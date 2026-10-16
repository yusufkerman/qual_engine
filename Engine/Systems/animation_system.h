#pragma once

/**
 * @file animation_system.h
 * @brief İskelet animasyonlarının oynatılması, anahtar kare enterpolasyonu (LERP/SLERP),
 *        kemik hiyerarşisi değerlendirmesi ve GPU matris paleti üretimini yöneten sistem.
 */

#include "../ECS/registry.h"
#include "../ECS/components.h"
#include "../Engine/Assets/aria.h"

class animation_system
{
public:
    animation_system() = default;
    ~animation_system() = default;

    /**
     * @brief Canlı sahnedeki tüm animatör ve iskelet bileşenlerini günceller.
     * @param reg Sahneye ait ECS kayıt defteri.
     * @param ariaSystem ARIA varlık sistemi referansı.
     * @param deltaTime Geçen kare süresi (saniye cinsinden).
     */
    void update(registry& reg, aria::system* ariaSystem, float deltaTime);

private:
    void update_animators(registry& reg, aria::system* ariaSystem, float deltaTime);
    void evaluate_skeletons(registry& reg, aria::system* ariaSystem);
    void update_bone_attachments(registry& reg, aria::system* ariaSystem);

    static glm::vec3 interpolate_position(const std::vector<aria::vector_keyframe>& keys, float time);
    static glm::quat interpolate_rotation(const std::vector<aria::quat_keyframe>& keys, float time);
    static glm::vec3 interpolate_scale(const std::vector<aria::vector_keyframe>& keys, float time);

    static void evaluate_skeleton_hierarchy(
        const aria::skeleton_node& node,
        const glm::mat4& parentTransform,
        const aria::raw_skeleton_data& skeletonData,
        const std::vector<bone_transform>& localPoses,
        const std::vector<uint8_t>& boneAnimated,
        std::vector<glm::mat4>& outGlobalPoses,
        std::vector<glm::mat4>& outSkinningPalette
    );
};

