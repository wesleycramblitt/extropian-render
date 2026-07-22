#include <exd/render/renderer.hpp>
#include <cstdint>
#include <string_view>

namespace exd::render {

class NullRenderer : public IRenderer {
public:
    void initialize(void*) override {}
    void shutdown() override {}
    void resize(uint32_t, uint32_t) override {}
    void begin_frame() override {}
    void execute(const RenderGraph&, const Camera&) override {}
    void end_frame() override {}
    std::string_view backend_name() const override { return "Null"; }
    std::string_view renderer_info() const override { return "Headless renderer"; }
};

} // namespace exd::render
