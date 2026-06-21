#pragma once

#include "../Graphics/model.h"
#include "../Renderer/render_item.h"


class MeshRenderer
{
public:
    int UploadModel(const Model& model);
    //void DrawMesh(int meshID);
    void DrawModel();

    void Draw(const render_item& item);

    void Clear();

};