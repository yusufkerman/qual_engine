#pragma once

/**
 * @file gameplay_input.h
 * @brief Betik yazarları için yüksek seviyeli, ergonomik ve güvenli girdi (input) API'si.
 */

#include <glm/glm.hpp>
#include "../Input/input_action.h"

namespace gameplay
{
    /**
     * @class input
     * @brief Ham girdi yapılarını soyutlayarak eylem, tuş ve fare durumlarını sorgulayan statik API.
     */
    class input
    {
    public:
        // ==========================================
        // EYLEM (ACTION) SORGULARI
        // ==========================================

        /**
         * @brief Belirtilen mantıksal eylemin basılı tutulup tutulmadığını sorgular.
         * @param action Sorgulanacak eylem (ör. MoveForward, Jump).
         * @return Basılıysa true döner.
         */
        [[nodiscard]] static bool is_action_pressed(InputAction action);

        /**
         * @brief Belirtilen eylemin bu karede yeni mi basıldığını sorgular (Tek seferlik tetikleme).
         * @param action Sorgulanacak eylem.
         * @return Bu karede basıldıysa true döner.
         */
        [[nodiscard]] static bool is_action_just_pressed(InputAction action);

        /**
         * @brief Belirtilen eylemin bu karede mi bırakıldığını sorgular.
         * @param action Sorgulanacak eylem.
         * @return Bu karede bırakıldıysa true döner.
         */
        [[nodiscard]] static bool is_action_just_released(InputAction action);

        // ==========================================
        // DONANIMSAL TUŞ (KEY) SORGULARI
        // ==========================================

        /**
         * @brief Belirtilen GLFW fiziksel tuşunun basılı olup olmadığını sorgular.
         * @param glfw_key GLFW tuş kodu (ör. GLFW_KEY_ESCAPE).
         */
        [[nodiscard]] static bool is_key_pressed(int glfw_key);

        /**
         * @brief Belirtilen tuşun bu karede yeni mi basıldığını sorgular.
         * @param glfw_key GLFW tuş kodu.
         */
        [[nodiscard]] static bool is_key_just_pressed(int glfw_key);

        /**
         * @brief Belirtilen tuşun bu karede mi bırakıldığını sorgular.
         * @param glfw_key GLFW tuş kodu.
         */
        [[nodiscard]] static bool is_key_just_released(int glfw_key);

        // ==========================================
        // EKSEN (AXIS) VE HAREKET KOLAYLAŞTIRICILARI
        // ==========================================

        /**
         * @brief İki zıt eylemden 1D eksen değeri (-1.0 ile +1.0 arası) üretir.
         * @param positive Pozitif yön eylemi (ör. MoveRight -> +1.0).
         * @param negative Negatif yön eylemi (ör. MoveLeft -> -1.0).
         * @return İkisi de basılıysa veya hiçbiri basılı değilse 0.0, aksi halde -1.0 veya +1.0.
         */
        [[nodiscard]] static float get_axis(InputAction positive, InputAction negative);

        /**
         * @brief Dört yön eyleminden normalize edilmiş 2D hareket vektörü üretir.
         * @param forward İleri eylemi (+Z).
         * @param backward Geri eylemi (-Z).
         * @param right Sağa hareket eylemi (+X).
         * @param left Sola hareket eylemi (-X).
         * @return Normalize edilmiş yön vektörü (Uzunluğu maksimum 1.0f).
         */
        [[nodiscard]] static glm::vec2 get_axis_2d(InputAction forward, InputAction backward, InputAction right, InputAction left);

        // ==========================================
        // FARE (MOUSE) SORGULARI
        // ==========================================

        /**
         * @brief Oyun ekranına göre ölçeklenmiş fare delta (yer değiştirme) vektörünü döner.
         */
        [[nodiscard]] static glm::vec2 get_mouse_delta();

        /**
         * @brief Farenin ekran üzerindeki anlık koordinatlarını döner.
         */
        [[nodiscard]] static glm::vec2 get_mouse_position();

        /**
         * @brief Fare imlecinin pencereye kilitli olup olmadığını döner.
         */
        [[nodiscard]] static bool is_cursor_locked();

        /**
         * @brief Fare imlecini pencereye kilitler veya serbest bırakır.
         * @param locked Kilitli yapmak için true.
         */
        static void set_cursor_locked(bool locked);
    };
}

