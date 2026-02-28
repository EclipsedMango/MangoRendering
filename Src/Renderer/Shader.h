#ifndef MANGORENDERING_SHADER_H
#define MANGORENDERING_SHADER_H

#include <string>
#include <unordered_map>

#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"

class Shader {
public:
    unsigned int m_id;

    // graphics pipeline
    Shader(const char* vertexPath, const char* fragmentPath);

    // compute pipeline
    explicit Shader(const char* computePath);

    ~Shader();

    void Bind() const;

    static void Unbind();

    void Dispatch(unsigned int x, unsigned int y, unsigned int z) const;

    void SetBool(const std::string &name, bool value) const;
    void SetInt(const std::string &name, int value) const;
    void SetUint(const std::string &name, unsigned int value) const;
    void SetFloat(const std::string &name, float value) const;
    void SetVector2(const std::string& name, const glm::vec2& value) const;
    void SetVector3(const std::string& name, const glm::vec3& value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

private:
    mutable std::unordered_map<std::string, int> m_uniformCache;

    int GetUniformLocation(const std::string& name) const;

    static std::string ReadFile(const char* path);
    static unsigned int CompileShader(unsigned int type, const std::string& source);
    static void CheckCompileErrors(unsigned int shader, const std::string& type);
};


#endif //MANGORENDERING_SHADER_H