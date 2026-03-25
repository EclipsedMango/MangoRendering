
#ifndef MANGORENDERING_PRIMITIVEMESH_H
#define MANGORENDERING_PRIMITIVEMESH_H

#include "Mesh.h"

static constexpr float PRIM_PI = 3.14159265358979323846f;

// ----- base -----
class PrimitiveMesh : public Mesh {
public:
    ~PrimitiveMesh() override = default;

protected:
    PrimitiveMesh() = default;
    virtual void BuildGeometry() = 0;
    void Rebuild(); // call after changing params
};

// ----- cube mesh -----
class CubeMesh : public PrimitiveMesh {
public:
    explicit CubeMesh(float size = 1.0f);

    void  SetSize(const float size) { m_size = size; Rebuild(); }
    [[nodiscard]] float GetSize() const { return m_size; }

protected:
    void BuildGeometry() override;

private:
    float m_size = 1.0f;
};

// ----- plane mesh -----
class PlaneMesh : public PrimitiveMesh {
public:
    explicit PlaneMesh(float width = 1.0f, float depth = 1.0f, int subdivisionsX = 1, int subdivisionsZ = 1);

    void  SetWidth(const float w)         { m_width = w;          Rebuild(); }
    void  SetDepth(const float d)         { m_depth = d;          Rebuild(); }
    void  SetSubdivisionsX(const int s)   { m_subdivisionsX = s;  Rebuild(); }
    void  SetSubdivisionsZ(const int s)   { m_subdivisionsZ = s;  Rebuild(); }

    [[nodiscard]] float GetWidth()        const { return m_width; }
    [[nodiscard]] float GetDepth()        const { return m_depth; }
    [[nodiscard]] int   GetSubdivisionsX()const { return m_subdivisionsX; }
    [[nodiscard]] int   GetSubdivisionsZ()const { return m_subdivisionsZ; }

protected:
    void BuildGeometry() override;

private:
    float m_width        = 1.0f;
    float m_depth        = 1.0f;
    int   m_subdivisionsX = 1;
    int   m_subdivisionsZ = 1;
};

// ----- quad mesh -----
class QuadMesh : public PrimitiveMesh {
public:
    explicit QuadMesh(float width = 1.0f, float height = 1.0f);

    void  SetWidth(const float w)  { m_width  = w; Rebuild(); }
    void  SetHeight(const float h) { m_height = h; Rebuild(); }

    [[nodiscard]] float GetWidth()  const { return m_width; }
    [[nodiscard]] float GetHeight() const { return m_height; }

protected:
    void BuildGeometry() override;

private:
    float m_width  = 1.0f;
    float m_height = 1.0f;
};

// ----- sphere mesh -----
class SphereMesh : public PrimitiveMesh {
public:
    explicit SphereMesh(float radius = 0.5f, int rings = 16, int sectors = 16);

    void  SetRadius(const float r)  { m_radius  = r; Rebuild(); }
    void  SetRings(const int r)     { m_rings   = r; Rebuild(); }
    void  SetSectors(const int s)   { m_sectors = s; Rebuild(); }

    [[nodiscard]] float GetRadius()  const { return m_radius; }
    [[nodiscard]] int   GetRings()   const { return m_rings; }
    [[nodiscard]] int   GetSectors() const { return m_sectors; }

protected:
    void BuildGeometry() override;

private:
    float m_radius  = 0.5f;
    int   m_rings   = 16;
    int   m_sectors = 16;
};

// ----- cylinder mesh -----
class CylinderMesh : public PrimitiveMesh {
public:
    explicit CylinderMesh(float radius = 0.5f, float height = 1.0f, int sectors = 16);

    void  SetRadius(const float r)  { m_radius  = r; Rebuild(); }
    void  SetHeight(const float h)  { m_height  = h; Rebuild(); }
    void  SetSectors(const int s)   { m_sectors = s; Rebuild(); }

    [[nodiscard]] float GetRadius()  const { return m_radius; }
    [[nodiscard]] float GetHeight()  const { return m_height; }
    [[nodiscard]] int   GetSectors() const { return m_sectors; }

protected:
    void BuildGeometry() override;

private:
    float m_radius  = 0.5f;
    float m_height  = 1.0f;
    int   m_sectors = 16;
};

// ----- capsule mesh -----
class CapsuleMesh : public PrimitiveMesh {
public:
    explicit CapsuleMesh(float radius = 0.5f, float height = 2.0f, int rings = 8, int sectors = 16);

    void  SetRadius(const float r)  { m_radius  = r; Rebuild(); }
    void  SetHeight(const float h)  { m_height  = h; Rebuild(); }
    void  SetRings(const int r)     { m_rings   = r; Rebuild(); }
    void  SetSectors(const int s)   { m_sectors = s; Rebuild(); }

    [[nodiscard]] float GetRadius()  const { return m_radius; }
    [[nodiscard]] float GetHeight()  const { return m_height; }
    [[nodiscard]] int   GetRings()   const { return m_rings; }
    [[nodiscard]] int   GetSectors() const { return m_sectors; }

protected:
    void BuildGeometry() override;

private:
    float m_radius  = 0.5f;
    float m_height  = 2.0f;
    int   m_rings   = 8;
    int   m_sectors = 16;
};

#endif //MANGORENDERING_PRIMITIVEMESH_H