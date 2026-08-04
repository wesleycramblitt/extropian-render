#pragma once

namespace exd::render {

// Render technique tags (marker components)
struct RenderTechnique_Lambertian {};
struct RenderTechnique_Mirror {};
    struct RenderTechnique_CubeMap {};
    struct RenderTechnique_Equirect {};
    struct RenderTechnique_Lit {};

} // namespace exd::render
