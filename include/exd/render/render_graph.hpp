#pragma once

#include <vector>
#include <string>
#include <memory>

namespace exd::render {

/// DAG of render passes. Populated by the application, executed by IRenderer.
class RenderGraph {
public:
    struct Pass {
        std::string name;
        int priority = 0;
    };

    void add_pass(Pass p) { passes_.push_back(std::move(p)); }
    const auto& passes() const { return passes_; }

private:
    std::vector<Pass> passes_;
};

} // namespace exd::render
