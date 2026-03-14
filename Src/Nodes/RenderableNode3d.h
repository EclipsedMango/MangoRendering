
#ifndef MANGORENDERING_RENDERABLENODE3D_H
#define MANGORENDERING_RENDERABLENODE3D_H

#include "Node3d.h"

class RenderApi;

class RenderableNode3d : public Node3d {
public:
    ~RenderableNode3d() override;
    virtual void SubmitToRenderer(RenderApi& renderer) = 0;
};


#endif //MANGORENDERING_RENDERABLENODE3D_H