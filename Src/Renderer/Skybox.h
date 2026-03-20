
#ifndef MANGORENDERING_SKYBOX_H
#define MANGORENDERING_SKYBOX_H
#include <string>
#include <vector>

#include "Shader.h"
#include "Materials/Texture.h"
#include "VertexArray.h"

class Skybox {
public:
    explicit Skybox(std::vector<std::string>& faces);
    explicit Skybox(const std::string& hdrPath, int faceSize = 512);
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void Draw(const glm::mat4& view, const glm::mat4& projection) const;
    [[nodiscard]] Texture& GetTexture() const { return *m_texture; }

private:
    Texture* m_texture = nullptr;
    Shader* m_shader = nullptr;
    VertexArray* m_vao = nullptr;
};


#endif //MANGORENDERING_SKYBOX_H