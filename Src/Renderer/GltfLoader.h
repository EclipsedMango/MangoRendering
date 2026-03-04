#ifndef MANGORENDERING_GLTFLOADER_H
#define MANGORENDERING_GLTFLOADER_H

#include <string>
#include <vector>
#include "Scene/Object.h"

class GltfLoader {
public:
    // returns all objects found in the file, caller owns the pointers
    static std::vector<Object*> Load(const std::string& path, Shader* shader);
};

#endif //MANGORENDERING_GLTFLOADER_H