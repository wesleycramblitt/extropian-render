#include <vector>
#include <string>
#include <functional>
namespace exd::render {
struct RenderPass { std::string name; int priority=0; };
class RenderGraph { public: void add_pass(const RenderPass&) {} void execute() {} };
}
