
#include "SkyboxNode3d.h"

SkyboxNode3d::SkyboxNode3d(std::vector<std::string> faces) : m_skybox(faces) {}
SkyboxNode3d::SkyboxNode3d(const std::string &hdrPath) : m_skybox(hdrPath) {}
