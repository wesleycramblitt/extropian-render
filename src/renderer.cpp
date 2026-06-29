#include <exd/render/renderer.hpp>
#include <cstdio>
namespace exd::render {
std::unique_ptr<IRenderer> IRenderer::create(Backend backend) {
    std::fprintf(stderr, "[IRenderer] Backend %d requested\n", (int)backend);
    return nullptr;
}
}
