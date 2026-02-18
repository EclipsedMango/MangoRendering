#ifndef MANGORENDERING_SHADER_H
#define MANGORENDERING_SHADER_H
#include <string>
#include <unordered_map>

#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"


class Shader {
public:
    unsigned int m_id;

    Shader(const char* vertexPath, const char* fragmentPath);
    void Bind() const;

    void SetBool(const std::string &name, bool value) const;
    void SetInt(const std::string &name, int value) const;
    void SetFloat(const std::string &name, float value) const;
    void SetVector3(const std::string& name, const glm::vec3& value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

    int GetUniformLocation(const std::string& name) const;

private:
    mutable std::unordered_map<std::string, int> m_uniformCache;
};


#endif //MANGORENDERING_SHADER_H