#ifndef MANGORENDERING_GLTFLOADER_H
#define MANGORENDERING_GLTFLOADER_H

#include <string>
#include <vector>

#include "Nodes/MeshNode3d.h"
#include "Renderer/Shader.h"

class GltfLoader {
public:
    // loads the full scene hierarchy, caller owns the returned Node3d pointer
    static Node3d* Load(const std::string& path, std::shared_ptr<Shader> shader);

    // extracts just the first mesh found in the file (useful for the ResourceManager)
    static std::shared_ptr<Mesh> ExtractFirstMesh(const std::string& path);
};

#endif //MANGORENDERING_GLTFLOADER_H