
#ifndef MANGORENDERING_EQUIRECTTOCUBEMAP_H
#define MANGORENDERING_EQUIRECTTOCUBEMAP_H

#include "Renderer/Materials/Texture.h"
#include "Renderer/Shader.h"

class EquirectToCubemap {
public:
    static Texture* Convert(const std::string& hdrPath, int faceSize = 512);
};


#endif //MANGORENDERING_EQUIRECTTOCUBEMAP_H