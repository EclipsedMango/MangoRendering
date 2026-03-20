
#ifndef MANGORENDERING_SKYBOXNODE3D_H
#define MANGORENDERING_SKYBOXNODE3D_H

#include <vector>
#include <string>

#include "Node3d.h"
#include "Renderer/Skybox.h"

class SkyboxNode3d : public Node3d {
public:
    explicit SkyboxNode3d(std::vector<std::string> faces);
    explicit SkyboxNode3d(const std::string& hdrPath);
    ~SkyboxNode3d() override = default;

    [[nodiscard]] const Skybox& GetSkybox() const { return m_skybox; }

private:
    Skybox m_skybox;
};


#endif //MANGORENDERING_SKYBOXNODE3D_H