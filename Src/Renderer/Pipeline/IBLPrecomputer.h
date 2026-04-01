
#ifndef MANGORENDERING_IBLPRECOMPUTER_H
#define MANGORENDERING_IBLPRECOMPUTER_H

#include <memory>
#include "Renderer/Materials/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Meshes/Mesh.h"

class Texture;

class IBLPrecomputer {
public:
    struct Result {
        std::unique_ptr<Texture> irradiance; // 32x32 cubemap
        std::unique_ptr<Texture> prefiltered; // 128x128 cubemap w/ mips
    };

    static Result Compute(const Texture& envCubemap);
    static void Shutdown();

    static constexpr int IRRADIANCE_SIZE = 32;
    static constexpr int PREFILTER_SIZE = 128;
    static constexpr int PREFILTER_MIP_LEVELS = 5;
private:
    static std::unique_ptr<Texture> ComputeIrradiance(const Texture& env, GLuint captureFbo, GLuint captureRbo, const Mesh& cube);
    static std::unique_ptr<Texture> ComputePrefiltered(const Texture& env, GLuint captureFbo, GLuint captureRbo, const Mesh& cube);

    static std::shared_ptr<Shader> m_irradianceShader;
    static std::shared_ptr<Shader> m_prefilterShader;
};


#endif //MANGORENDERING_IBLPRECOMPUTER_H