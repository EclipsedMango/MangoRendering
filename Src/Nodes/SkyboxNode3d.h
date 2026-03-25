
#ifndef MANGORENDERING_SKYBOXNODE3D_H
#define MANGORENDERING_SKYBOXNODE3D_H

#include <vector>
#include <string>

#include "Node3d.h"
#include "Renderer/Skybox.h"

class SkyboxNode3d : public Node3d {
public:
    SkyboxNode3d();
    explicit SkyboxNode3d(std::vector<std::string> faces);
    explicit SkyboxNode3d(const std::string& hdrPath);
    ~SkyboxNode3d() override = default;

    void SetHdrPath(const std::string& path);

    [[nodiscard]] std::string GetNodeType() const override { return "SkyboxNode3d"; }
    [[nodiscard]] Skybox* GetSkybox() const { return m_skybox.get(); }

private:
    void RegisterProperties();

    std::string m_hdrPath;
    std::unique_ptr<Skybox> m_skybox;
};

#endif //MANGORENDERING_SKYBOXNODE3D_H