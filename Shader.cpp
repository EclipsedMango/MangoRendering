
#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "glad/gl.h"

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
    const std::string vertexCode = ReadFile(vertexPath);
    const std::string fragmentCode = ReadFile(fragmentPath);

    const unsigned int vertex = CompileShader(GL_VERTEX_SHADER, vertexCode);
    const unsigned int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);

    CheckCompileErrors(m_id, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(const char* computePath) {
    const std::string computeCode = ReadFile(computePath);
    const unsigned int compute = CompileShader(GL_COMPUTE_SHADER, computeCode);

    m_id = glCreateProgram();
    glAttachShader(m_id, compute);
    glLinkProgram(m_id);

    CheckCompileErrors(m_id, "PROGRAM");

    glDeleteShader(compute);
}

Shader::~Shader() {
    glDeleteProgram(m_id);
}

void Shader::Bind() const {
    glUseProgram(m_id);
}

void Shader::Unbind() {
    glUseProgram(0);
}

void Shader::Dispatch(const unsigned int x, const unsigned int y, const unsigned int z) const {
    Bind();
    glDispatchCompute(x, y, z);
    // Note: You usually need a glMemoryBarrier(...) after this in the main loop
    // but that depends on what you do next, so we leave it out of here.
}

void Shader::SetBool(const std::string &name, const bool value) const {
    glUniform1i(GetUniformLocation(name), static_cast<int>(value));
}

void Shader::SetInt(const std::string &name, const int value) const {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string &name, const float value) const {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVector3(const std::string &name, const glm::vec3 &value) const {
	glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::SetMatrix4(const std::string &name, const glm::mat4 &value) const {
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

int Shader::GetUniformLocation(const std::string& name) const {
    if (m_uniformCache.contains(name)) {
        return m_uniformCache[name];
    }

    const int location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        std::cout << "Warning: Uniform '" << name << "' not found." << std::endl;
    }

    m_uniformCache[name] = location;
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

        file.close();
        code = stream.str();
    } catch (std::ifstream::failure&) {
        throw std::runtime_error("Failed to read shader file: " + std::string(path));
    }

    return code;
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