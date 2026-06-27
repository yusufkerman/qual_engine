#pragma once

#include "../Graphics/model.h"
#include "../Renderer/render_item.h"
#include "../Renderer/texture_manager.h"

enum RenderConstants : int
{
    MAX_LIGHT_COUNT = 64,
    DIFFUSE_TEXTURE_SLOT = 0,
    NORMAL_TEXTURE_SLOT = 1
};
class MeshRenderer
{
public:
    int UploadModel(const Model& model);
    //void DrawMesh(int meshID);
    void DrawModel();

    void Draw(const render_item& item, texture_manager& textureManager);

    void Clear();

};