
#include "Shader.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "glad/gl.h"

REGISTER_PROPERTY_TYPE(Shader)

uint64_t Shader::GenerateResourceId() {
    static std::atomic<uint64_t> s_nextId {1};
    return s_nextId.fetch_add(1, std::memory_order_relaxed);
}

Shader::Shader(const char *vertexPath, const char *fragmentPath) : m_resourceId(GenerateResourceId()) {
    m_vertexPath = vertexPath;
    m_fragmentPath = fragmentPath;
    Recompile();
}

Shader::Shader(const char *vertexPath, const char *fragmentPath, const char *geometryPath) : m_resourceId(GenerateResourceId()) {
    m_vertexPath = vertexPath;
    m_fragmentPath = fragmentPath;
    m_geometryPath = geometryPath;
    Recompile();
}

Shader::Shader(const char* computePath) : m_resourceId(GenerateResourceId()) {
    m_computePath = computePath;
    Recompile();
}

Shader::Shader() : m_resourceId(GenerateResourceId()) {
    AddProperty("name",
        [this]() -> PropertyValue { return GetName(); },
        [this](const PropertyValue& v) { SetName(std::get<std::string>(v)); }
    );

    AddProperty("vertex_path",
        [this]() -> PropertyValue { return GetVertexPath(); },
        [this](const PropertyValue& v) { SetVertexPath(std::get<std::string>(v)); }
    );

    AddProperty("fragment_path",
        [this]() -> PropertyValue { return GetFragmentPath(); },
        [this](const PropertyValue& v) { SetFragmentPath(std::get<std::string>(v)); }
    );

    AddProperty("geometry_path",
        [this]() -> PropertyValue { return GetGeometryPath(); },
        [this](const PropertyValue& v) { SetGeometryPath(std::get<std::string>(v)); }
    );

    AddProperty("compute_path",
        [this]() -> PropertyValue { return GetComputePath(); },
        [this](const PropertyValue& v) { SetComputePath(std::get<std::string>(v)); }
    );
}

Shader::~Shader() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
    }
}

void Shader::Bind() const {
    glUseProgram(m_id);

#ifndef NDEBUG
    GLint current = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    if (static_cast<GLuint>(current) != m_id) {
        std::cerr << "[Shader] ERROR: glUseProgram failed for " << m_id << "\n";
    }
#endif
}

void Shader::Unbind() {
    glUseProgram(0);
}

void Shader::Dispatch(const unsigned int x, const unsigned int y, const unsigned int z) const {
    Bind();
    glDispatchCompute(x, y, z);
    // Note: You usually need a glMemoryBarrier(...) after this in the main loop
}

void Shader::SetBool(const std::string &name, const bool value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform1i(loc, static_cast<int>(value));
    }
}

void Shader::SetInt(const std::string &name, const int value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform1i(loc, value);
    }
}

void Shader::SetUint(const std::string &name, const unsigned int value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform1ui(loc, value);
    }
}

void Shader::SetFloat(const std::string &name, const float value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform1f(loc, value);
    }
}

void Shader::SetVector2(const std::string &name, const glm::vec2 &value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform2fv(loc, 1, &value[0]);
    }
}

void Shader::SetVector3(const std::string &name, const glm::vec3 &value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform3fv(loc, 1, &value[0]);
    }
}

void Shader::SetVector4(const std::string &name, const glm::vec4 &value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniform4fv(loc, 1, &value[0]);
    }
}

void Shader::SetMatrix4(const std::string &name, const glm::mat4 &value) const {
    if (const int loc = GetUniformLocation(name); loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
    }
}

void Shader::Recompile() {
    if (m_id != 0) {
        glDeleteProgram(m_id);
        m_uniformCache.clear();
        m_id = 0;
    }

    if (!m_computePath.empty()) {
        const std::string computeCode = ReadFile(m_computePath.c_str());
        const unsigned int compute = CompileShader(GL_COMPUTE_SHADER, computeCode);

        m_id = glCreateProgram();
        glAttachShader(m_id, compute);
        glLinkProgram(m_id);

        CheckCompileErrors(m_id, "PROGRAM");
        glDeleteShader(compute);
        return;
    }

    if (!m_vertexPath.empty() && !m_fragmentPath.empty()) {
        const std::string vertexCode = ReadFile(m_vertexPath.c_str());
        const std::string fragmentCode = ReadFile(m_fragmentPath.c_str());

        const unsigned int vertex = CompileShader(GL_VERTEX_SHADER, vertexCode);
        const unsigned int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);
        unsigned int geometry = 0;

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, fragment);

        if (!m_geometryPath.empty()) {
            const std::string geometryCode = ReadFile(m_geometryPath.c_str());
            geometry = CompileShader(GL_GEOMETRY_SHADER, geometryCode);
            glAttachShader(m_id, geometry);
        }

        glLinkProgram(m_id);
        CheckCompileErrors(m_id, "PROGRAM");

        m_supportsInstancing = glGetUniformLocation(m_id, "u_UseInstancing") != -1;

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometry != 0) {
            glDeleteShader(geometry);
        }
    }
}

bool Shader::IsBound() const {
    GLint current = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    return static_cast<GLuint>(current) == m_id;
}

int Shader::GetUniformLocation(const std::string& name) const {
#ifndef NDEBUG
    if (!IsBound()) {
        GLint current = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current);
        std::cerr << "[Shader] WARNING: setting uniform '" << name << "' but shader program " << m_id << " is not bound (current=" << current << ")\n";
        return -1;
    }
#endif

    if (const auto it = m_uniformCache.find(name); it != m_uniformCache.end()) {
        return it->second; // may be -1
    }

    const int location = glGetUniformLocation(m_id, name.c_str());
    m_uniformCache[name] = location;

#ifndef NDEBUG
    if (location == -1) {
        std::cout << "Warning: Uniform '" << name << "' not found.\n";
    }
#endif

    return location;
}

std::string Shader::ReadFile(const char* path) {
    std::string code;
    std::ifstream file;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(path);

        std::stringstream stream;
        stream << file.rdbuf();

        std::string output = PreprocessSource(stream, path);

        file.close();
        code = output;
    } catch (std::ifstream::failure&) {
        throw std::runtime_error("Failed to read shader file: " + std::string(path));
    }

    return code;
}

std::string Shader::PreprocessSource(const std::stringstream& source, const char* path) {
    std::unordered_set<std::string> defines;
    return PreprocessSource(source, path, defines);
}

std::string Shader::PreprocessSource(const std::stringstream &source, const char *path, std::unordered_set<std::string> &defines) {
    std::string output;
    std::string line;
    std::istringstream iss(source.str());
    std::vector<bool> skipStack;

    auto isSkipping = [&] {
        return std::ranges::any_of(skipStack, [](const bool b) { return b; });
    };

    int lineNumber = 1;
    while (std::getline(iss, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        if (trimmed.starts_with("//")) {
            if (!isSkipping()) {
                output += line + "\n";
                lineNumber++;
            }

            continue;
        }

        if (trimmed.find("#ifndef") != std::string::npos) {
            std::string symbol = trimmed.substr(trimmed.find("#ifndef") + 8);
            symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
            symbol.erase(0, symbol.find_first_not_of(" \t\r\n"));
            skipStack.push_back(defines.contains(symbol));
            continue;
        }

        if (trimmed.find("#ifdef") != std::string::npos) {
            std::string symbol = trimmed.substr(trimmed.find("#ifdef") + 7);
            symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
            symbol.erase(0, symbol.find_first_not_of(" \t\r\n"));
            skipStack.push_back(!defines.contains(symbol));
            continue;
        }

        if (trimmed.find("#else") != std::string::npos) {
            bool outerSkipping = skipStack.size() > 1 && std::any_of(skipStack.begin(), skipStack.end() - 1, [](const bool b) { return b; });
            if (!outerSkipping) {
                skipStack.back() = !skipStack.back();
            }

            continue;
        }

        if (trimmed.find("#endif") != std::string::npos) {
            if (!skipStack.empty()) skipStack.pop_back();
            continue;
        }

        if (isSkipping()) {
            continue;
        }

        if (trimmed.find("#define") != std::string::npos) {
            std::string symbol = trimmed.substr(trimmed.find("#define") + 8);
            symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
            symbol.erase(0, symbol.find_first_not_of(" \t\r\n"));

            size_t space = symbol.find_first_of(" \t");
            if (space != std::string::npos) {
                symbol = symbol.substr(0, space);
            }

            defines.insert(symbol);
            output += line + "\n";
            lineNumber++;
            continue;
        }

        if (trimmed.find("#include") != std::string::npos) {
            std::string includePath = trimmed.substr(trimmed.find('"') + 1, trimmed.rfind('"') - trimmed.find('"') - 1);
            std::filesystem::path resolvedPath = std::filesystem::path(path).parent_path() / includePath;

            std::ifstream includeFile(resolvedPath);
            std::stringstream includeStream;
            includeStream << includeFile.rdbuf();

            output += PreprocessSource(includeStream, resolvedPath.string().c_str(), defines);
            output += "#line " + std::to_string(lineNumber + 1) + "\n";
            continue;
        }

        output += line + "\n";
        lineNumber++;
    }

    return output;
}

unsigned int Shader::CompileShader(const unsigned int type, const std::string& source) {
    const char* src = source.c_str();
    const unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    CheckCompileErrors(shader, "SHADER");
    return shader;
}

void Shader::CheckCompileErrors(const unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            throw std::runtime_error("Shader Compilation Failed");
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            throw std::runtime_error("Program Linking Failed");
        }
    }
}
