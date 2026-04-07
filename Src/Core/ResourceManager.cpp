
#include "ResourceManager.h"

#include <iostream>
#include <fstream>
#include <fkYAML/node.hpp>
#include "SDL3/SDL_filesystem.h"

#include "Renderer/Materials/Texture.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Meshes/PrimitiveMesh.h"
#include "Renderer/Shader.h"
#include "Renderer/Meshes/GltfLoader.h"

template<> ResourceManager::Cache<Texture>& ResourceManager::GetCache<Texture>() { return m_textures; }
template<> ResourceManager::Cache<Material>& ResourceManager::GetCache<Material>() { return m_materials; }
template<> ResourceManager::Cache<Mesh>& ResourceManager::GetCache<Mesh>() { return m_meshes; }
template<> ResourceManager::Cache<Shader>& ResourceManager::GetCache<Shader>() { return m_shaders; }

// texture loading
template<>
std::shared_ptr<Texture> ResourceManager::Load<Texture>(const std::string& filepath, const CacheMode mode) {
    if (mode == CacheMode::Reuse) {
        if (auto cached = Get<Texture>(filepath)) return cached;
    }

    std::string fullPath = SearchAssetFile(filepath);
    if (fullPath.empty()) {
        std::cerr << "[ResourceManager] Failed to load Texture: " << filepath << std::endl;
        return nullptr;
    }

    auto tex = std::make_shared<Texture>(fullPath);
    Register<Texture>(filepath, tex);
    return tex;
}

// mesh loading
template<>
std::shared_ptr<Mesh> ResourceManager::Load<Mesh>(const std::string& filepath, CacheMode mode) {
    if (filepath.empty() || filepath == "None") return nullptr;
    if (mode == CacheMode::Reuse) {
        if (auto cached = Get<Mesh>(filepath)) return cached;
    }

    std::shared_ptr<Mesh> mesh = nullptr;

    if (filepath == "Cube") mesh = std::make_shared<CubeMesh>();
    else if (filepath == "Sphere") mesh = std::make_shared<SphereMesh>();
    else if (filepath == "Plane") mesh = std::make_shared<PlaneMesh>();
    else if (filepath == "Quad") mesh = std::make_shared<QuadMesh>();
    else if (filepath == "Cylinder") mesh = std::make_shared<CylinderMesh>();
    else if (filepath == "Capsule") mesh = std::make_shared<CapsuleMesh>();
    else {
        const size_t hashPos = filepath.find_last_of('#');
        if (hashPos != std::string::npos && (filepath.find(".gltf") != std::string::npos || filepath.find(".glb") != std::string::npos)) {
            const std::string filename = filepath.substr(0, hashPos);
            std::string indices = filepath.substr(hashPos + 1);
            const size_t underscore = indices.find('_');

            if (underscore != std::string::npos) {
                const int meshIdx = std::stoi(indices.substr(0, underscore));
                const int primIdx = std::stoi(indices.substr(underscore + 1));

                const std::string fullPath = SearchAssetFile(filename);
                if (!fullPath.empty()) {
                    mesh = GltfLoader::ExtractMesh(fullPath, meshIdx, primIdx);
                }
            }
        }
    }

    if (!mesh) {
        std::cerr << "[ResourceManager] Unknown mesh, primitive, or file missing: " << filepath << std::endl;
        return nullptr;
    }

    Register<Mesh>(filepath, mesh);
    return mesh;
}

// material loading
template<>
std::shared_ptr<Material> ResourceManager::Load<Material>(const std::string& filepath, CacheMode mode) {
    if (mode == CacheMode::Reuse) {
        if (auto cached = Get<Material>(filepath)) return cached;
    }

    std::string fullPath = SearchAssetFile(filepath);
    if (fullPath.empty()) {
        auto mat = std::make_shared<Material>();
        mat->SetFilePath(fullPath);
        Register<Material>(filepath, mat);
        return mat;
    }

    std::ifstream file(fullPath);
    auto yaml = fkyaml::node::deserialize(file);

    auto mat = std::make_shared<Material>();
    mat->SetFilePath(fullPath);

    if (yaml.contains("properties")) {
        for (auto& [key, val] : yaml["properties"].get_value<fkyaml::ordered_map<std::string, fkyaml::node>>()) {
            PropertyValue propVal = PropertyHolder::DeserializePropertyValue(val);
            try {
                mat->Set(key, propVal);
            } catch (...) {
                mat->AddProperty(key, [propVal]{ return propVal; }, {});
            }
        }
    }

    Register<Material>(filepath, mat);
    return mat;
}

template<>
std::shared_ptr<Shader> ResourceManager::Load<Shader>(const std::string& filepath, CacheMode mode) {
    if (mode == CacheMode::Reuse) {
        if (auto cached = Get<Shader>(filepath)) return cached;
    }

    const std::string vertPath = SearchAssetFile(filepath + ".vert");
    const std::string fragPath = SearchAssetFile(filepath + ".frag");

    if (vertPath.empty() || fragPath.empty()) {
        std::cerr << "[ResourceManager] Failed to find .vert or .frag for: " << filepath << std::endl;
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(vertPath.c_str(), fragPath.c_str());
    Register<Shader>(filepath, shader);
    return shader;
}

template<>
void ResourceManager::Save(const std::shared_ptr<Material> &mat, const std::string &filePath) {
    if (!mat) return;

    fkyaml::node yaml = fkyaml::node::mapping();

    if (mat->GetShader()) {
        yaml["shader_name"] = mat->GetShader()->GetName();
    }

    fkyaml::node properties = fkyaml::node::mapping();
    for (const auto& [key, prop] : mat->GetProperties()) {
        properties[key] = PropertyHolder::SerializeProperty(prop.getter());
    }
    yaml["properties"] = properties;

    std::ofstream file(filePath);
    file << fkyaml::node::serialize(yaml);

    mat->SetFilePath(filePath);
    Register<Material>(filePath, mat);
}

ResourceManager::ResourceManager() {
    const char* basePathStr = SDL_GetBasePath();
    fs::path searchPath = basePathStr ? fs::path(basePathStr) : fs::current_path();

    bool foundEngineAssets = false;
    while (searchPath.has_parent_path()) {
        fs::path potentialEngineAssets = searchPath / "EngineAssets";
        if (fs::exists(potentialEngineAssets) && fs::is_directory(potentialEngineAssets)) {
            m_engineAssetsPath = potentialEngineAssets;
            foundEngineAssets = true;
            break;
        }
        searchPath = searchPath.parent_path();
    }

    if (!foundEngineAssets) {
        std::cerr << "[ResourceManager] FATAL: Could not find EngineAssets folder!" << std::endl;
        m_engineAssetsPath = fs::current_path() / "EngineAssets";
    }

    RefreshAssetRegistry();
}

ResourceManager::~ResourceManager() {
    ReleaseAll();
}

void ResourceManager::ScanDirectoryIntoRegistry(const fs::path& directory) {
    if (fs::exists(directory) && fs::is_directory(directory)) {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                m_fileRegistry[entry.path().filename().string()] = entry.path().string();
            }
        }
    }
}

std::string ResourceManager::ManualSearch(const fs::path& directory, const std::string& filename) {
    if (fs::exists(directory) && fs::is_directory(directory)) {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().filename().string() == filename) {
                m_fileRegistry[filename] = entry.path().string();
                return entry.path().string();
            }
        }
    }
    return "";
}

std::string ResourceManager::SearchAssetFile(const std::string& filename) {
    if (fs::exists(filename)) return filename;
    if (const auto it = m_fileRegistry.find(filename); it != m_fileRegistry.end()) {
        if (fs::exists(it->second)) return it->second;
    }

    std::string foundPath;
    if (!m_userAssetsPath.empty()) foundPath = ManualSearch(m_userAssetsPath, filename);
    if (foundPath.empty()) foundPath = ManualSearch(m_engineAssetsPath, filename);
    return foundPath;
}

void ResourceManager::SetUserDirectory(const std::filesystem::path& userPath) {
    m_userAssetsPath = userPath;
    RefreshAssetRegistry();
}

const std::filesystem::path& ResourceManager::GetEngineDirectory() const { return m_engineAssetsPath; }
const std::filesystem::path& ResourceManager::GetUserDirectory() const { return m_userAssetsPath; }

void ResourceManager::RefreshAssetRegistry() {
    m_fileRegistry.clear();
    ScanDirectoryIntoRegistry(m_engineAssetsPath);
    if (!m_userAssetsPath.empty()) {
        ScanDirectoryIntoRegistry(m_userAssetsPath);
    }
}

std::string ResourceManager::ResolveAssetPath(const std::string& filename) {
    return SearchAssetFile(filename);
}

void ResourceManager::ReleaseAll() {
    m_textures.clear();
    m_materials.clear();
    m_meshes.clear();
    m_shaders.clear();
}

void ResourceManager::CollectGarbage() {
    EraseExpired(m_textures);
    EraseExpired(m_materials);
    EraseExpired(m_meshes);
    EraseExpired(m_shaders);
}

template<typename T>
void ResourceManager::EraseExpired(Cache<T>& cache) {
    for (auto it = cache.begin(); it != cache.end();) {
        it = it->second.expired() ? cache.erase(it) : ++it;
    }
}

void ResourceManager::InitializeDefaultResources() {
    m_defaultShader = LoadShader("default", "test.vert", "test.frag");
}

void ResourceManager::ClearDefaultResources() {
    m_defaultShader.reset();
}

std::shared_ptr<Texture> ResourceManager::LoadTextureFromMemory(const std::string& key, const unsigned char* data, int width, int height, int channels) {
    if (key.empty()) return nullptr;
    if (auto tex = Get<Texture>(key)) return tex;

    auto tex = std::make_shared<Texture>(data, width, height, channels, key);
    Register<Texture>(key, tex);
    return tex;
}

std::shared_ptr<Material> ResourceManager::DuplicateMaterial(const std::string& sourceName, const std::string& newName) {
    if (const auto source = Get<Material>(sourceName)) {
        auto mat = std::make_shared<Material>(*source);
        Register<Material>(newName, mat);
        return mat;
    }
    return nullptr;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, const std::string& vertFilename, const std::string& fragFilename) {
    if (auto shader = Get<Shader>(name)) return shader;

    const std::string vertFullPath = SearchAssetFile(vertFilename);
    const std::string fragFullPath = SearchAssetFile(fragFilename);

    if (vertFullPath.empty() || fragFullPath.empty()) {
        std::cerr << "[ResourceManager] Failed to load Shader files for: " << name << std::endl;
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(vertFullPath.c_str(), fragFullPath.c_str());
    Register<Shader>(name, shader);
    return shader;
}

std::shared_ptr<Shader> ResourceManager::LoadShaderWithGeom(const std::string& name, const std::string& vertFilename, const std::string& fragFilename, const std::string& geomFileName) {
    if (auto shader = Get<Shader>(name)) return shader;

    const std::string vertFullPath = SearchAssetFile(vertFilename);
    const std::string fragFullPath = SearchAssetFile(fragFilename);
    const std::string geomFullPath = SearchAssetFile(geomFileName);

    if (vertFullPath.empty() || fragFullPath.empty() || geomFullPath.empty()) {
        std::cerr << "[ResourceManager] Failed to load Shader files for: " << name << std::endl;
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(vertFullPath.c_str(), fragFullPath.c_str(), geomFullPath.c_str());
    Register<Shader>(name, shader);
    return shader;
}

std::shared_ptr<Shader> ResourceManager::LoadComputeShader(const std::string& name, const std::string& computeFilename) {
    if (auto shader = Get<Shader>(name)) return shader;

    const std::string compFullPath = SearchAssetFile(computeFilename);
    if (compFullPath.empty()) {
        std::cerr << "[ResourceManager] Failed to load Compute Shader: " << computeFilename << std::endl;
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(compFullPath.c_str());
    Register<Shader>(name, shader);
    return shader;
}