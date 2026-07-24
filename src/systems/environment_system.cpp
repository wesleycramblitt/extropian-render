#include <exd/render/systems/environment_system.hpp>
#include <exd/render/components/environment.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/material.hpp>
#include <exd/render/components/mesh_asset.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/math/quat.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>

namespace exd::render {

EnvironmentSystem::EnvironmentSystem(exd::ecs::Registry& registry, GraphicsContext& ctx)
    : registry_(registry), ctx_(ctx) {}

void EnvironmentSystem::load(const std::string& name, const std::string& base_path) {
    std::string full_dir = base_path + "/" + name;
    load_impl(full_dir, name);
}

void EnvironmentSystem::unload_all() {
    for (auto e : registry_.view<EnvironmentComponent>()) {
        auto& env = registry_.get<EnvironmentComponent>(e);
        for (auto& ent : env.spawned_entities) {
            if (registry_.valid(ent))
                registry_.destroy(ent);
        }
        env.spawned_entities.clear();
        registry_.destroy(e);  // also destroy the hub entity itself
    }
}

void EnvironmentSystem::load_impl(const std::string& full_dir, const std::string& name) {
    std::string json_path = full_dir + "/env.json";
    std::ifstream f(json_path);
    if (!f.is_open()) {
        std::fprintf(stderr, "[Environment] Cannot open %s\n", json_path.c_str());
        return;
    }

    nlohmann::json cfg;
    try { cfg = nlohmann::json::parse(f); }
    catch (const std::exception& ex) {
        std::fprintf(stderr, "[Environment] JSON parse error in %s: %s\n",
                     json_path.c_str(), ex.what());
        return;
    }

    std::printf("[Environment] Loading %s from %s\n", name.c_str(), json_path.c_str());

    // ── create the environment hub entity ──────────────────────
    auto env_entity = registry_.create(name + "_env");
    auto& env_c = registry_.emplace<EnvironmentComponent>(env_entity);
    env_c.name = name;
    auto& spawned = env_c.spawned_entities;

    // ── skybox ────────────────────────────────────────────────
    std::string skybox_path = full_dir + "/" + cfg.value("skybox", "skybox/cross.png");
    {
        auto sky_e = registry_.create(name + "_sky");
        spawned.push_back(sky_e);
        auto& cm = registry_.emplace<CubeMapComponent>(sky_e);
        cm.custom_path = skybox_path;
        cm.cross_layout = true;
        registry_.emplace<RenderTechnique_CubeMap>(sky_e);
    }

    // ── terrain ───────────────────────────────────────────────
    if (cfg.contains("terrain") && cfg["terrain"].contains("mesh")) {
        std::string terrain_path = full_dir + "/" + cfg["terrain"]["mesh"].get<std::string>();
        auto terr_e = registry_.create(name + "_terrain");
        spawned.push_back(terr_e);
        registry_.emplace<MeshAssetComponent>(terr_e, terrain_path);
        registry_.emplace<RenderTechnique_Lambertian>(terr_e);
        registry_.emplace<Transform>(terr_e);
        if (cfg.contains("terrain_color")) {
            auto& tc = cfg["terrain_color"];
            registry_.emplace<Material>(terr_e, math::Quat{
                tc[3].get<float>(), tc[0].get<float>(), tc[1].get<float>(), tc[2].get<float>()});
        }
    }

    // ── props ─────────────────────────────────────────────────
    if (cfg.contains("props")) {
        int prop_idx = 0;
        for (const auto& p : cfg["props"]) {
            std::string prop_path = full_dir + "/" + p["mesh"].get<std::string>();
            auto prop_e = registry_.create(name + "_prop_" + std::to_string(prop_idx++));
            spawned.push_back(prop_e);
            registry_.emplace<MeshAssetComponent>(prop_e, prop_path);
            registry_.emplace<RenderTechnique_Lambertian>(prop_e);

            auto& xform = registry_.emplace<Transform>(prop_e);
            if (cfg.contains("prop_color")) {
                auto& pc = cfg["prop_color"];
                registry_.emplace<Material>(prop_e, math::Quat{
                    pc[3].get<float>(), pc[0].get<float>(), pc[1].get<float>(), pc[2].get<float>()});
            }
            if (p.contains("position")) {
                auto& pos = p["position"];
                xform.position = {pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>()};
            }
            if (p.contains("scale")) {
                auto& scl = p["scale"];
                xform.scale = {scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>()};
            }
        }
    }

    // ── scene-wide fog ────────────────────────────────────────
    {
        auto fog_e = registry_.create(name + "_fog");
        spawned.push_back(fog_e);
        auto& fc = registry_.emplace<FogComponent>(fog_e);
        if (cfg.contains("lighting")) {
            auto& L = cfg["lighting"];
            if (L.contains("fog_color")) {
                auto& fcj = L["fog_color"];
                fc.color = {fcj[0].get<float>(), fcj[1].get<float>(), fcj[2].get<float>()};
            }
            if (L.contains("fog_density"))
                fc.density = L["fog_density"].get<float>();
        }
    }

    // ── scene-wide lighting ───────────────────────────────────
    {
        auto light_e = registry_.create(name + "_light");
        spawned.push_back(light_e);
        auto& sl = registry_.emplace<SceneLighting>(light_e);
        if (cfg.contains("lighting")) {
            auto& L = cfg["lighting"];
            if (L.contains("ambient")) {
                auto& aj = L["ambient"];
                sl.ambient = {aj[0].get<float>(), aj[1].get<float>(), aj[2].get<float>()};
            }
            if (L.contains("sun_direction")) {
                auto& sd = L["sun_direction"];
                sl.sun_direction = {sd[0].get<float>(), sd[1].get<float>(), sd[2].get<float>()};
            }
            if (L.contains("sun_color")) {
                auto& sc = L["sun_color"];
                sl.sun_color = {sc[0].get<float>(), sc[1].get<float>(), sc[2].get<float>()};
            }
        }
    }

    env_c.loaded = true;
    // also track the hub entity itself
    // (unload_all iterates EnvironmentComponent view, so the hub is destroyed there)
    std::printf("[Environment] %s loaded successfully\n", name.c_str());
}

void EnvironmentSystem::update(exd::ecs::Registry&, double) {
}

} // namespace exd::render
