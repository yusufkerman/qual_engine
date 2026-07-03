// MeshRenderer.h
#pragma once
#include "../Renderer/render_item.h"

// İleri bildirim (Aria sistemine erişebilmek için)
namespace aria { class system; }

enum RenderConstants : int
{
    MAX_LIGHT_COUNT = 64,
    DIFFUSE_TEXTURE_SLOT = 0,
    NORMAL_TEXTURE_SLOT = 1,
    SPECULAR_TEXTURE_SLOT = 3 // İhtiyacın olursa diye slot 3
};

class MeshRenderer
{
public:
    // Artık parametre olarak textureManager yerine doğrudan aria::system alıyoruz!
    void Draw(const render_item& item, aria::system* ariaSystem);

    void SetViewport(int x, int y, int width, int height);
    void BindFramebuffer(unsigned int fboID);
    void BindTextureSlot(unsigned int slot, unsigned int textureID);

    // Not: Eski UploadModel ve static vektörler Aria'nın 'gpu_resource_cache' 
    // veya 'opengl_backend' tarafına taşınacağı için burası zamanla tamamen temizlenecektir.
};