#pragma once

#include "../Graphics/model.h"

class MeshRenderer
{
public:
    int UploadModel(const Model& model);
    void DrawMesh(int meshID);
    void DrawModel();
};