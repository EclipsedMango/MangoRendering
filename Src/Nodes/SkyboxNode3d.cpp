
#include "SkyboxNode3d.h"

SkyboxNode3d::SkyboxNode3d(std::vector<std::string> faces) : m_skybox(faces) {
    SetName("SkyboxNode3d");
}

SkyboxNode3d::SkyboxNode3d(const std::string &hdrPath) : m_skybox(hdrPath) {
    SetName("SkyboxNode3d");
}
