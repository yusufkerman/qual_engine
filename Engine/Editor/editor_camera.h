#pragma once

/**
 * @file editor_camera.h
 * @brief Editör sahne görünümü (Scene Viewport) serbest uçuş kamerasını tanımlar.
 */

#include <glm/glm.hpp>
#include "../Engine/fly_camera.h"

struct GLFWwindow;
struct InputState;

/**
 * @class EditorCamera
 * @brief Sahne düzenleme esnasında kullanıcının serbestçe dolaşmasını sağlayan uçuş kamerası sınıfı.
 */
class EditorCamera
{
public:
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };   ///< Kameranın dünya uzayındaki konumu.
    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };   ///< Kameranın Euler açıları (Pitch, Yaw, Roll).

    float fov = 45.0f;                        ///< Dikey görüş açısı (Field of View).
    float nearPlane = 0.1f;                   ///< Yakın kırpma düzlemi.
    float farPlane = 1000.0f;                 ///< Uzak kırpma düzlemi.
    float scrollStep = 0.0f;                  ///< Fare tekerleği yakınlaştırma adımı.

    bool sceneHovered = false;                ///< Fare imleci sahne penceresi üzerinde mi.
    bool sceneFocused = false;                ///< Sahne penceresi klavye/fare odağına sahip mi.

    fly_camera_state flyState;                ///< Uçuş kamerası girdi ve hız durumu.

    bool is2DMode = false;                    ///< 2B Ortografik mod aktif mi.
    float orthographicSize = 10.0f;           ///< 2B Ortografik görünüm dikey boyutu.
    glm::vec3 saved3DPosition{ 0.0f, 2.0f, 5.0f };
    glm::vec3 saved3DRotation{ 0.0f, 0.0f, 0.0f };

    /**
     * @brief 2B Ortografik mod ile 3B Perspektif mod arasında geçiş yapar.
     */
    void set2DMode(bool enabled);
    void toggle2DMode();

    /**
     * @brief Fare girdi durumunu ve kaydırma adımlarını sıfırlar.
     */
    void resetMouseState();

    /**
     * @brief Kullanıcı girdilerini işleyerek kamera konum ve yönelimini günceller.
     *
     * @param window GLFW pencere tanıtıcısı.
     * @param input Anlık klavye ve fare durumunu taşıyan girdi yapısı.
     * @param sensitivity Fare dönüş hassasiyeti.
     * @param moveSpeed Kamera hareket hızı.
     * @param hoverGate Fare girdilerinin yalnızca pencere üzerindeyken işlenmesini sağlayan bayrak.
     */
    void update(
        GLFWwindow* window,
        InputState& input,
        float sensitivity,
        float moveSpeed,
        bool hoverGate
    );

    /**
     * @brief Kameranın görünüm (View) matrisini hesaplar.
     */
    glm::mat4 getViewMatrix() const;

    /**
     * @brief Verilen en/boy oranına (aspect ratio) göre projeksiyon (Perspektif / Ortografik) matrisini hesaplar.
     */
    glm::mat4 getProjectionMatrix(float aspect) const;
};