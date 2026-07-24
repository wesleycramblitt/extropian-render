#pragma once

// Umbrella header for extropian-render.

// ── Graphics core ──────────────────────────────────────────
#include <exd/render/graphics/mesh.hpp>
#include <exd/render/graphics/mesh_gpu.hpp>
#include <exd/render/graphics/mesh_manager.hpp>
#include <exd/render/graphics/texture.hpp>
#include <exd/render/graphics/texture_manager.hpp>
#include <exd/render/graphics/shader_manager.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/uniform_value.hpp>
#include <exd/render/graphics/draw_data.hpp>

// ── Techniques ─────────────────────────────────────────────
#include <exd/render/graphics/techniques/lambertian_technique.hpp>
#include <exd/render/graphics/techniques/reflective_technique.hpp>
#include <exd/render/graphics/techniques/cubemap_technique.hpp>
#include <exd/render/graphics/techniques/particle_technique.hpp>
#include <exd/render/graphics/techniques/volume_technique.hpp>
#include <exd/render/graphics/techniques/highlight_technique.hpp>

// ── Components ─────────────────────────────────────────────
#include <exd/render/components/transform.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/material.hpp>
#include <exd/render/components/parent.hpp>
#include <exd/render/components/children.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/hovered.hpp>
#include <exd/render/components/render_technique_tags.hpp>

// ── Systems ────────────────────────────────────────────────
#include <exd/render/systems/hierarchy_system.hpp>
#include <exd/render/systems/render_system.hpp>
#include <exd/render/systems/camera_system.hpp>

// ── Interaction ────────────────────────────────────────────
#include <exd/render/interaction/picker.hpp>
#include <exd/render/interaction/selection.hpp>
#include <exd/render/interaction/ray.hpp>
#include <exd/render/interaction/gizmo.hpp>
#include <exd/render/interaction/gizmo_mesh.hpp>
