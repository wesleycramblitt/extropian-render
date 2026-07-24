#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <string>

namespace exd::render {

class EnvironmentSystem {
public:
    EnvironmentSystem(exd::ecs::Registry& registry, GraphicsContext& ctx);

    void load(const std::string& name,
              const std::string& base_path = "assets/environments");

    /// Destroy all entities spawned by every known EnvironmentComponent.
    /// Call before load() to replace the current environment cleanly.
    void unload_all();

    void update(exd::ecs::Registry& registry, double dt);

private:
    exd::ecs::Registry& registry_;
    GraphicsContext& ctx_;
    void load_impl(const std::string& full_dir, const std::string& name);
};

} // namespace exd::render
