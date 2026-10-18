#pragma once

/**
 * @file gameplay_ui.h
 * @brief Oyun içi scriptlerin (NativeScript) 2B UI elemanlarına (Text, Button, Image, RectTransform) erişimini ve manipülasyonunu sağlayan API.
 */

#include "gameplay_context.h"
#include "../ECS/components.h"
#include <string>
#include <functional>
#include <glm/glm.hpp>

namespace gameplay
{
    /**
     * @class ui
     * @brief UI elemanlarının özelliklerini (metin, renk, boyut, tıklama dinleyicisi) script üzerinden okumayı ve değiştirmeyi sağlayan statik yardımcı sınıf.
     */
    class ui
    {
    public:
        // ==========================================
        // TEXT API
        // ==========================================

        /**
         * @brief Bir UI Text bileşeninin metin içeriğini günceller.
         */
        static void set_text(entity e, const std::string& newText)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* txt = reg->get_component<ui_text_component>(e))
                {
                    txt->text = newText;
                }
            }
        }

        /**
         * @brief Bir UI Text bileşeninin metnini döner.
         */
        [[nodiscard]] static std::string get_text(entity e)
        {
            if (auto* reg = context::get_registry())
            {
                if (const auto* txt = reg->get_component<ui_text_component>(e))
                {
                    return txt->text;
                }
            }
            return "";
        }

        /**
         * @brief Bir UI Text bileşeninin rengini günceller.
         */
        static void set_text_color(entity e, const glm::vec4& color)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* txt = reg->get_component<ui_text_component>(e))
                {
                    txt->color = color;
                }
            }
        }

        // ==========================================
        // BUTTON API
        // ==========================================

        /**
         * @brief Bir UI butonuna tıklanma (onClick) geri çağrım fonksiyonu bağlar.
         */
        static void on_click(entity e, std::function<void()> callback)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* btn = reg->get_component<ui_button_component>(e))
                {
                    btn->on_click = std::move(callback);
                }
            }
        }

        /**
         * @brief Butonun etkileşime açık (interactable) olup olmadığını ayarlar.
         */
        static void set_interactable(entity e, bool interactable)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* btn = reg->get_component<ui_button_component>(e))
                {
                    btn->interactable = interactable;
                }
            }
        }

        // ==========================================
        // IMAGE API
        // ==========================================

        /**
         * @brief Bir UI Image bileşeninin rengini günceller.
         */
        static void set_image_color(entity e, const glm::vec4& color)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* img = reg->get_component<ui_image_component>(e))
                {
                    img->color = color;
                }
            }
        }

        // ==========================================
        // RECT TRANSFORM API
        // ==========================================

        /**
         * @brief Bir UI elemanının 2B anchored konumunu ayarlar.
         */
        static void set_anchored_position(entity e, const glm::vec2& pos)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    rt->anchored_position = pos;
                }
            }
        }

        /**
         * @brief Bir UI elemanının 2B anchored konumunu okur.
         */
        [[nodiscard]] static glm::vec2 get_anchored_position(entity e)
        {
            if (auto* reg = context::get_registry())
            {
                if (const auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    return rt->anchored_position;
                }
            }
            return glm::vec2(0.0f);
        }

        /**
         * @brief Bir UI elemanının 2B boyutunu ayarlar.
         */
        static void set_size(entity e, const glm::vec2& size)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    rt->size = size;
                }
            }
        }

        /**
         * @brief Bir UI elemanının 2B boyutunu okur.
         */
        [[nodiscard]] static glm::vec2 get_size(entity e)
        {
            if (auto* reg = context::get_registry())
            {
                if (const auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    return rt->size;
                }
            }
            return glm::vec2(0.0f);
        }

        /**
         * @brief UI elemanının görünürlüğünü açar veya kapatır.
         */
        static void set_visible(entity e, bool visible)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    rt->visible = visible;
                }
            }
        }

        /**
         * @brief UI elemanının görünür olup olmadığını döner.
         */
        [[nodiscard]] static bool is_visible(entity e)
        {
            if (auto* reg = context::get_registry())
            {
                if (const auto* rt = reg->get_component<rect_transform_component>(e))
                {
                    return rt->visible;
                }
            }
            return true;
        }

        /**
         * @brief Bir UI Text elemanının font boyutunu ayarlar.
         */
        static void set_font_size(entity e, float fontSize)
        {
            if (auto* reg = context::get_registry())
            {
                if (auto* txt = reg->get_component<ui_text_component>(e))
                {
                    txt->font_size = fontSize;
                }
            }
        }
    };
}


