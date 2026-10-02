#pragma once

/**
 * @file engine_context.h
 * @brief Oyun motorunun tüm temel alt sistemlerini, ayarlarını ve global durumlarını barındıran merkezi bağlam (context) yapısı.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "../Editor/editor_layer.h"
#include "../Renderer/render_target.h"
#include "../Editor/ui_manager.h"
#include "../Scene/scene_session.h"
#include "../Input/input_map.h"
#include "../Input/input_state.h"
#include "../Input/input_system.h"
#include "../Renderer/MeshRenderer.h"
#include "../Renderer/Debug/grid_renderer.h"
#include "../shader_manager.h"
#include "../Renderer/material_manager.h"
#include "../Renderer/texture_manager.h"
#include "Assets/asset_manager.h"
#include "../Systems/physics_system.h"
#include "../Systems/render_system.h"
#include "../Systems/camera_system.h"
#include "../Importers/ModelLoader.h"
#include "../Renderer/shadow_map_fbo.h"
#include "../Renderer/Debug/gizmo_system.h"
#include "../Editor/Vesta/vesta.h"
#include "Assets/aria.h"
#include "../Editor/engine_mode.h"
#include "fly_camera.h"

struct GLFWwindow;

namespace sera
{
    class sera;
}

/**
 * @struct EngineContext
 * @brief Motorun yaşam döngüsü boyunca ihtiyaç duyulan tüm çekirdek sistemlerin örneklerini (instance) tutar.
 */
struct EngineContext
{
    // ==========================================
    // PENCERE VE GÖRÜNTÜ AYARLARI
    // ==========================================
    GLFWwindow* window = nullptr;       ///< GLFW Pencere işaretçisi.
    int screenW = 1280;                 ///< Ekran genişliği (piksel).
    int screenH = 720;                  ///< Ekran yüksekliği (piksel).

    float sensitivity = 0.002f;         ///< Kamera fare hassasiyeti.
    float moveSpeed = 0.10f;            ///< Kamera temel hareket hızı.
    bool isEditorBuild = true;          ///< Motorun editör modunda çalışıp çalışmadığını belirler.
    EngineMode prevMode = EngineMode::EDITOR; ///< Bir önceki motor modu durumu (Play/Editor geçişleri için).

    // ==========================================
    // ÇEKİRDEK (CORE) SİSTEMLER
    // ==========================================
    InputState inputState;              ///< Anlık girdi durumları (tuş basımları vb.).
    InputMap inputMap;                  ///< Fiziksel tuşların mantıksal eylemlere bağlandığı harita.
    InputSystem inputSystem;            ///< Girdi olaylarını işleyen sistem.
    scene_session session;              ///< Aktif sahne oturumu (oyun ve editör sahnelerini barındırır).
    sera::sera* sera = nullptr;         ///< Sera alt sistemi (Oyun motoruna özel serileştirme vb. işlemler için).
    fly_camera_state runtimeCameraState;///< Oyun içi serbest kamera (fly camera) durumu.

    // ==========================================
    // EDİTÖR SİSTEMLERİ
    // ==========================================
    ui_manager uiManager;               ///< Editör arayüz (UI) yöneticisi.
    RenderTarget editorTarget;          ///< Editör kamerasının çizildiği render hedefi.
    RenderTarget gameTarget;            ///< Oyun kamerasının çizildiği render hedefi.
    vesta::vesta_system vestaSys;       ///< Vesta arayüz/pencere yönetim sistemi.
    gizmo_system gizmoSystem;           ///< Editör içi manipülasyon araçları (transform, rotate, scale gizmo).

    // ==========================================
    // RENDER VE VARLIK (ASSET) SİSTEMLERİ
    // ==========================================
    MeshRenderer renderer;              ///< Temel mesh çizim sistemi.
    GridRenderer grid;                  ///< Editör içi referans ızgara çizim sistemi.

    shader_manager shaderManager;       ///< Eski nesil (legacy) shader yöneticisi.
    material_manager materialManager;   ///< Eski nesil materyal yöneticisi.
    texture_manager textureManager;     ///< Eski nesil doku yöneticisi.
    asset_manager assetManager;         ///< Eski nesil varlık yöneticisi.

    render_system renderSystem;         ///< Genel render yönetim sistemi.
    camera_system cameraSystem;         ///< Kamera yaşam döngüsü ve hesaplama sistemi.
    physics_system physicsSystem;       ///< Fizik motoru köprü sistemi (Jolt, PhysX vb. sarıcı).

    ModelLoader modelLoader;            ///< 3D model yükleyici servisi.
    shadow_map_fbo shadowFBO;           ///< Gölge haritalama için çerçeve arabelleği nesnesi (FBO).

    aria::system* aria_system = nullptr;///< Modern Aria varlık (asset) ve bellek yönetim sistemi.

    // ==========================================
    // ÇALIŞMA ZAMANI (RUNTIME) DOKU VE SHADER REFERANSLARI
    // ==========================================
    unsigned int skyboxTexture = 0;                     ///< Gökyüzü küp haritasına (cubemap) ait OpenGL kimliği.

    aria::handle<aria::shader_asset> gridShaderHandle;  ///< Izgara (grid) çizimi için shader referansı.
    aria::handle<aria::shader_asset> skyboxShaderHandle;///< Gökyüzü (skybox) çizimi için shader referansı.
    aria::handle<aria::shader_asset> defaultShaderHandle; ///< Varsayılan materyal shader referansı.
    aria::handle<aria::shader_asset> gizmoShaderHandle; ///< Gizmo çizimi için shader referansı.
    aria::handle<aria::shader_asset> shadowShaderHandle;///< Gölge hesaplamaları için shader referansı.
};