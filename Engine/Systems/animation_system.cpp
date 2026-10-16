/**
 * @file animation_system.cpp
 * @brief İskelet animasyonlarının oynatılması, anahtar kare enterpolasyonu (LERP/SLERP),
 *        kemik hiyerarşisi değerlendirmesi ve GPU matris paleti üretim uygulaması.
 */

#include "animation_system.h"
#include "../ECS/transform_utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <algorithm>

void animation_system::update(registry& reg, aria::system* ariaSystem, float deltaTime)
{
    if (!ariaSystem)
    {
        return;
    }

    // 1. Animatörleri güncelle (Zaman ilerletme, anahtar kare enterpolasyonu, crossfade)
    update_animators(reg, ariaSystem, deltaTime);

    // 2. İskelet hiyerarşisini çöz (Local poses -> Global poses -> Skinning Palette)
    evaluate_skeletons(reg, ariaSystem);

    // 3. Kemik soketlerini güncelle (Bone attachments -> Transform components)
    update_bone_attachments(reg, ariaSystem);
}

void animation_system::update_animators(registry& reg, aria::system* ariaSystem, float deltaTime)
{
    for (auto& [e, animator] : reg.get_all<animator_component>())
    {
        auto* skeleton = reg.get_component<skeleton_component>(e);
        if (!skeleton || !skeleton->skeleton_handle.is_valid())
        {
            continue;
        }

        const auto* skelAsset = ariaSystem->get_skeleton_data(skeleton->skeleton_handle);
        if (!skelAsset)
        {
            continue;
        }

        const size_t boneCount = skelAsset->raw_data.bones.size();
        if (skeleton->local_pose.size() != boneCount)
        {
            skeleton->local_pose.resize(boneCount);
            skeleton->is_dirty = true;
        }
        if (skeleton->bone_animated.size() != boneCount)
        {
            skeleton->bone_animated.assign(boneCount, 0);
            skeleton->is_dirty = true;
        }

        if (!animator.current_clip.is_valid())
        {
            std::fill(skeleton->bone_animated.begin(), skeleton->bone_animated.end(), 0);
            continue;
        }

        const auto* clipAsset = ariaSystem->get_animation_data(animator.current_clip);
        if (!clipAsset)
        {
            std::fill(skeleton->bone_animated.begin(), skeleton->bone_animated.end(), 0);
            continue;
        }

        // Zamanı ilerlet
        if (animator.is_playing)
        {
            animator.playback_time += deltaTime * animator.playback_speed;
        }

        float ticksPerSec = clipAsset->raw_data.ticks_per_second;
        if (ticksPerSec <= 0.0f) ticksPerSec = 25.0f;
        const float durationTicks = clipAsset->raw_data.duration;
        const float durationSec = durationTicks / ticksPerSec;

        if (durationSec > 0.0f)
        {
            if (animator.is_looping)
            {
                if (animator.playback_time > durationSec)
                {
                    animator.playback_time = std::fmod(animator.playback_time, durationSec);
                }
                else if (animator.playback_time < 0.0f)
                {
                    animator.playback_time = durationSec + std::fmod(animator.playback_time, durationSec);
                }
            }
            else
            {
                if (animator.playback_time >= durationSec)
                {
                    animator.playback_time = durationSec;
                    animator.is_playing = false;
                }
            }
        }

        const float currentTicks = animator.playback_time * ticksPerSec;

        // Crossfade geçiş kontrolü
        const aria::animation_clip_asset* blendClipAsset = nullptr;
        float blendTicks = 0.0f;
        float blendWeight = 0.0f;

        if (animator.blend_state.is_blending && animator.blend_state.target_clip.is_valid())
        {
            blendClipAsset = ariaSystem->get_animation_data(animator.blend_state.target_clip);
            if (blendClipAsset)
            {
                animator.blend_state.fade_timer += deltaTime;
                blendWeight = glm::clamp(animator.blend_state.fade_timer / animator.blend_state.fade_duration, 0.0f, 1.0f);
                animator.blend_state.current_weight = blendWeight;

                float blendTicksPerSec = blendClipAsset->raw_data.ticks_per_second;
                if (blendTicksPerSec <= 0.0f) blendTicksPerSec = 25.0f;

                animator.blend_state.target_time += deltaTime * animator.playback_speed;
                blendTicks = animator.blend_state.target_time * blendTicksPerSec;
            }
        }

        // Her kemik için animasyon pozunu hesapla
        for (size_t b = 0; b < boneCount; ++b)
        {
            const std::string& boneName = skelAsset->raw_data.bones[b].name;
            const auto* channel = clipAsset->raw_data.find_channel(boneName);
            const auto* blendChannel = (blendClipAsset && blendWeight > 0.0f) ? blendClipAsset->raw_data.find_channel(boneName) : nullptr;

            if (!channel && !blendChannel)
            {
                skeleton->bone_animated[b] = 0;
                continue;
            }

            skeleton->bone_animated[b] = 1;

            bone_transform poseA;
            if (channel)
            {
                poseA.position = interpolate_position(channel->positions, currentTicks);
                poseA.rotation = interpolate_rotation(channel->rotations, currentTicks);
                poseA.scale    = interpolate_scale(channel->scalings, currentTicks);
            }
            else if (blendChannel)
            {
                poseA = skeleton->local_pose[b];
            }

            if (blendChannel)
            {
                bone_transform poseB;
                poseB.position = interpolate_position(blendChannel->positions, blendTicks);
                poseB.rotation = interpolate_rotation(blendChannel->rotations, blendTicks);
                poseB.scale    = interpolate_scale(blendChannel->scalings, blendTicks);

                skeleton->local_pose[b].position = glm::mix(poseA.position, poseB.position, blendWeight);
                skeleton->local_pose[b].rotation = glm::normalize(glm::slerp(poseA.rotation, poseB.rotation, blendWeight));
                skeleton->local_pose[b].scale    = glm::mix(poseA.scale, poseB.scale, blendWeight);
            }
            else
            {
                skeleton->local_pose[b] = poseA;
            }
        }

        if (blendClipAsset && blendWeight >= 1.0f)
        {
            animator.current_clip = animator.blend_state.target_clip;
            animator.playback_time = animator.blend_state.target_time;
            animator.blend_state.is_blending = false;
        }

        skeleton->is_dirty = true;
    }
}

void animation_system::evaluate_skeletons(registry& reg, aria::system* ariaSystem)
{
    for (auto& [e, skeleton] : reg.get_all<skeleton_component>())
    {
        if (!skeleton.skeleton_handle.is_valid())
        {
            continue;
        }

        const auto* skelAsset = ariaSystem->get_skeleton_data(skeleton.skeleton_handle);
        if (!skelAsset || skelAsset->raw_data.bones.empty())
        {
            continue;
        }

        const size_t boneCount = skelAsset->raw_data.bones.size();

        if (skeleton.global_pose.size() != boneCount)
        {
            skeleton.global_pose.resize(boneCount, glm::mat4(1.0f));
            skeleton.is_dirty = true;
        }
        if (skeleton.skinning_palette.size() != boneCount)
        {
            skeleton.skinning_palette.resize(boneCount, glm::mat4(1.0f));
            skeleton.is_dirty = true;
        }
        if (skeleton.local_pose.size() != boneCount)
        {
            skeleton.local_pose.resize(boneCount);
            skeleton.is_dirty = true;
        }
        if (skeleton.bone_animated.size() != boneCount)
        {
            skeleton.bone_animated.assign(boneCount, 0);
            skeleton.is_dirty = true;
        }

        if (skeleton.is_dirty)
        {
            evaluate_skeleton_hierarchy(
                skelAsset->raw_data.root_node,
                glm::mat4(1.0f),
                skelAsset->raw_data,
                skeleton.local_pose,
                skeleton.bone_animated,
                skeleton.global_pose,
                skeleton.skinning_palette
            );
            skeleton.is_dirty = false;
        }
    }
}

void animation_system::evaluate_skeleton_hierarchy(
    const aria::skeleton_node& node,
    const glm::mat4& parentTransform,
    const aria::raw_skeleton_data& skeletonData,
    const std::vector<bone_transform>& localPoses,
    const std::vector<uint8_t>& boneAnimated,
    std::vector<glm::mat4>& outGlobalPoses,
    std::vector<glm::mat4>& outSkinningPalette
)
{
    glm::mat4 nodeLocal;
    if (node.bone_index >= 0 &&
        static_cast<size_t>(node.bone_index) < localPoses.size() &&
        static_cast<size_t>(node.bone_index) < boneAnimated.size() &&
        boneAnimated[node.bone_index])
    {
        nodeLocal = localPoses[node.bone_index].to_matrix();
    }
    else
    {
        nodeLocal = node.local_transform;
    }

    const glm::mat4 nodeGlobal = parentTransform * nodeLocal;

    if (node.bone_index >= 0 && static_cast<size_t>(node.bone_index) < skeletonData.bones.size())
    {
        outGlobalPoses[node.bone_index] = nodeGlobal;
        outSkinningPalette[node.bone_index] =
            skeletonData.global_inverse_transform *
            nodeGlobal *
            skeletonData.bones[node.bone_index].offset_matrix;
    }

    for (const auto& child : node.children)
    {
        evaluate_skeleton_hierarchy(
            child,
            nodeGlobal,
            skeletonData,
            localPoses,
            boneAnimated,
            outGlobalPoses,
            outSkinningPalette
        );
    }
}

void animation_system::update_bone_attachments(registry& reg, aria::system* ariaSystem)
{
    for (auto& [e, attachment] : reg.get_all<bone_attachment_component>())
    {
        const entity targetE = attachment.target_skeleton_entity;
        if (targetE == 0)
        {
            continue;
        }

        const auto* skel = reg.get_component<skeleton_component>(targetE);
        auto* targetTransform = reg.get_component<transform_component>(e);

        if (!skel || !targetTransform || !skel->skeleton_handle.is_valid())
        {
            continue;
        }

        if (attachment.cached_bone_index < 0 && !attachment.bone_name.empty())
        {
            const auto* skelAsset = ariaSystem->get_skeleton_data(skel->skeleton_handle);
            if (skelAsset)
            {
                attachment.cached_bone_index = skelAsset->raw_data.find_bone(attachment.bone_name);
            }
        }

        if (attachment.cached_bone_index >= 0 && static_cast<size_t>(attachment.cached_bone_index) < skel->global_pose.size())
        {
            const glm::mat4 parentWorld = ecs_transform::get_world_matrix(targetE, reg);
            const glm::mat4 boneGlobal = skel->global_pose[attachment.cached_bone_index];

            glm::mat4 offsetMat = glm::translate(glm::mat4(1.0f), attachment.local_offset);
            offsetMat = glm::rotate(offsetMat, glm::radians(attachment.local_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            offsetMat = glm::rotate(offsetMat, glm::radians(attachment.local_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            offsetMat = glm::rotate(offsetMat, glm::radians(attachment.local_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            offsetMat = glm::scale(offsetMat, attachment.local_scale);

            const glm::mat4 finalWorld = parentWorld * boneGlobal * offsetMat;
            ecs_transform::apply_world_matrix_as_local(*targetTransform, finalWorld);
        }
    }
}

glm::vec3 animation_system::interpolate_position(const std::vector<aria::vector_keyframe>& keys, float time)
{
    if (keys.empty()) return glm::vec3(0.0f);
    if (keys.size() == 1 || time <= keys.front().time_stamp) return keys.front().value;
    if (time >= keys.back().time_stamp) return keys.back().value;

    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time < keys[i + 1].time_stamp)
        {
            const float delta = keys[i + 1].time_stamp - keys[i].time_stamp;
            const float factor = delta > 0.00001f ? (time - keys[i].time_stamp) / delta : 0.0f;
            return glm::mix(keys[i].value, keys[i + 1].value, factor);
        }
    }

    return keys.back().value;
}

glm::quat animation_system::interpolate_rotation(const std::vector<aria::quat_keyframe>& keys, float time)
{
    if (keys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (keys.size() == 1 || time <= keys.front().time_stamp) return keys.front().value;
    if (time >= keys.back().time_stamp) return keys.back().value;

    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time < keys[i + 1].time_stamp)
        {
            const float delta = keys[i + 1].time_stamp - keys[i].time_stamp;
            const float factor = delta > 0.00001f ? (time - keys[i].time_stamp) / delta : 0.0f;
            return glm::slerp(keys[i].value, keys[i + 1].value, factor);
        }
    }

    return keys.back().value;
}

glm::vec3 animation_system::interpolate_scale(const std::vector<aria::vector_keyframe>& keys, float time)
{
    if (keys.empty()) return glm::vec3(1.0f);
    if (keys.size() == 1 || time <= keys.front().time_stamp) return keys.front().value;
    if (time >= keys.back().time_stamp) return keys.back().value;

    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time < keys[i + 1].time_stamp)
        {
            const float delta = keys[i + 1].time_stamp - keys[i].time_stamp;
            const float factor = delta > 0.00001f ? (time - keys[i].time_stamp) / delta : 0.0f;
            return glm::mix(keys[i].value, keys[i + 1].value, factor);
        }
    }

    return keys.back().value;
}

