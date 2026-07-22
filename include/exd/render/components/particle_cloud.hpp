#pragma once

#include <vector>

namespace exd::render {

struct ParticleCloudComponent {
    std::vector<float> positions;
    std::vector<float> colors;
    int particle_count = 0;
    int max_particles = 100000;
};

} // namespace exd::render
