#ifndef MANGORENDERING_SHADER_H
#define MANGORENDERING_SHADER_H

#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "Core/PropertyHolder.h"

class Shader : public PropertyHolder {
public:
    unsigned int m_id {};

    Shader(const char* vertexPath, const char* fragmentPath);
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath);
    explicit Shader(const char* computePath);
    Shader();
    ~Shader() override;

    void Bind() const;
    static void Unbind();

    void Dispatch(unsigned int x, unsigned int y, unsigned int z) const;

    void SetBool(const std::string &name, bool value) const;
    void SetInt(const std::string &name, int value) const;
    void SetUint(const std::string &name, unsigned int value) const;
    void SetFloat(const std::string &name, float value) const;
    void SetVector2(const std::string& name, const glm::vec2& value) const;
    void SetVector3(const std::string& name, const glm::vec3& value) const;
    void SetVector4(const std::string& name, const glm::vec4& value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

    [[nodiscard]] std::string GetPropertyHolderType() const override { return "Shader"; }
    [[nodiscard]] std::string GetName() const { return m_name; }
    [[nodiscard]] std::string GetVertexPath() const { return m_vertexPath; }
    [[nodiscard]] std::string GetFragmentPath() const { return m_fragmentPath; }
    [[nodiscard]] std::string GetGeometryPath() const { return m_geometryPath; }
    [[nodiscard]] std::string GetComputePath() const { return m_computePath; }
    [[nodiscard]] bool IsCompute() const { return !m_computePath.empty(); }
    [[nodiscard]] bool SupportsInstancing() const { return m_supportsInstancing; }
    [[nodiscard]] uint64_t GetResourceId() const { return m_resourceId; }

    void SetName(const std::string& name) { m_name = name; }
    void SetVertexPath(const std::string& path) { m_vertexPath = path; }
    void SetFragmentPath(const std::string& path) { m_fragmentPath = path; }
    void SetGeometryPath(const std::string& path) { m_geometryPath = path; }
    void SetComputePath(const std::string& path) { m_computePath = path; }

    void Recompile();
private:
    static uint64_t GenerateResourceId();

    std::string m_name = "New Shader";
    std::string m_vertexPath;
    std::string m_fragmentPath;
    std::string m_geometryPath;
    std::string m_computePath;
    bool m_isCompute = false;
    bool m_supportsInstancing = false;
    uint64_t m_resourceId = 0;

    bool IsBound() const;

    mutable std::unordered_map<std::string, int> m_uniformCache;

    int GetUniformLocation(const std::string& name) const;

    static std::string ReadFile(const char* path);
    static std::string PreprocessSource(const std::stringstream& source, const char* path);
    static std::string PreprocessSource(const std::stringstream &source, const char *path, std::unordered_set<std::string> &defines);

    static unsigned int CompileShader(unsigned int type, const std::string& source);
    static void CheckCompileErrors(unsigned int shader, const std::string& type);
};


#endif //MANGORENDERING_SHADER_H
