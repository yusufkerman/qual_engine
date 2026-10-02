#pragma once

/**
 * @file fly_camera.h
 * @brief Editör ve oyun içi serbest dolaşım (fly) kamerasının durumlarını ve işlevlerini tanımlar.
 */

#include <glm/glm.hpp>

struct GLFWwindow;
struct InputState;

/**
 * @struct fly_camera_state
 * @brief Serbest kameranın anlık durum (state) verilerini tutar.
 */
struct fly_camera_state
{
    bool capturingMouse = false; ///< Farenin şu an yakalanıp (capture) yakalanmadığını belirtir.
    bool firstMouse = true;      ///< Kamera hareketine başlandığındaki ilk fare hareketini tespit etmek için kullanılır.
    bool prevRightHeld = false;  ///< Bir önceki karede sağ tık tuşunun basılı olup olmadığını tutar.
    double lastX = 0.0;          ///< Farenin x eksenindeki son konumu.
    double lastY = 0.0;          ///< Farenin y eksenindeki son konumu.
};

/**
 * @brief Verilen Euler dönüş (rotation) açılarına göre ileri (forward) yön vektörünü hesaplar.
 *
 * @param rotation Kameranın rotasyon değerleri (x: Pitch, y: Yaw, z: Roll).
 * @return glm::vec3 Normalize edilmiş ileri (forward) yön vektörü.
 */
glm::vec3 calc_forward(const glm::vec3& rotation);

/**
 * @brief Serbest kameranın konum ve rotasyonunu kullanıcı girdilerine göre günceller.
 *
 * @param window GLFW pencere işaretçisi.
 * @param input Motorun genel girdi durum nesnesi (Not: Şu an içeride aktif kullanılmıyor).
 * @param state Kameranın anlık durum verilerini tutan yapı.
 * @param position Kameranın dünyadaki güncellenecek konumu (referans).
 * @param rotation Kameranın güncellenecek rotasyonu (referans).
 * @param sensitivity Fare hassasiyeti çarpanı.
 * @param moveSpeed Kameranın temel hareket hızı.
 * @param hoverGate Kameranın çalışması için imlecin geçerli bir alan (viewport) üzerinde olup olmadığını belirten kapı/şart.
 * @param scrollStep Fare tekerleği kaydırma değeri (varsayılan: nullptr). Yakınlaştırma/İleri atılma için kullanılır.
 */
void update_fly_camera(
    GLFWwindow* window,
    InputState& input,
    fly_camera_state& state,
    glm::vec3& position,
    glm::vec3& rotation,
    float sensitivity,
    float moveSpeed,
    bool hoverGate,
    float* scrollStep = nullptr
);