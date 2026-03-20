
#ifndef MANGORENDERING_CUBEGEOMETRY_H
#define MANGORENDERING_CUBEGEOMETRY_H

#include "Renderer/VertexArray.h"

namespace CubeGeometry {
    static constexpr float vertices[] = {
        -1, -1,  1,   1, -1,  1,   1,  1,  1,  -1,  1,  1,  // +Z
        -1, -1, -1,  -1,  1, -1,   1,  1, -1,   1, -1, -1,  // -Z
        -1,  1, -1,  -1,  1,  1,   1,  1,  1,   1,  1, -1,  // +Y
        -1, -1, -1,   1, -1, -1,   1, -1,  1,  -1, -1,  1,  // -Y
         1, -1, -1,   1,  1, -1,   1,  1,  1,   1, -1,  1,  // +X
        -1, -1, -1,  -1, -1,  1,  -1,  1,  1,  -1,  1, -1,  // -X
    };

    static constexpr uint32_t indices[] = {
        0,  1,  2,   2,  3,  0,
        4,  5,  6,   6,  7,  4,
        8,  9, 10,  10, 11,  8,
       12, 13, 14,  14, 15, 12,
       16, 17, 18,  18, 19, 16,
       20, 21, 22,  22, 23, 20,
   };

    inline VertexArray* CreateVao() {
        std::vector<Vertex> verts;
        verts.reserve(24);
        for (int i = 0; i < 24; i++) {
            Vertex v{};
            v.position = { vertices[i*3], vertices[i*3+1], vertices[i*3+2] };
            verts.push_back(v);
        }
        const std::vector idx(std::begin(indices), std::end(indices));
        return new VertexArray(verts, idx);
    }

} // namespace CubeGeometry

#endif //MANGORENDERING_CUBEGEOMETRY_H