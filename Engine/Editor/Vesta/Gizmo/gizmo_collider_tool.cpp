/**
 * @file gizmo_collider_tool.cpp
 * @brief Sahne içi etkileşimli Box ve Sphere çarpıştırıcı (Collider) boyutlandırma tutamaçları uygulaması.
 */

#include "gizmo_collider_tool.h"
#include "gizmo_math_utils.h"
#include "../../../Engine/engine_context.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include "../../../Systems/mesh_collider_baker.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace vesta
{
    void gizmo_collider_tool::reset_interaction()
    {
        m_ActiveHandle = -1;
        m_HoveredHandle = -1;
    }

    void gizmo_collider_tool::render(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& reg = engineCtx.session.editor_scene().get_registry();
        const int selectedID = engineCtx.vestaSys.selection().get_first_selected_entity();
        if (selectedID == -1)
        {
            reset_interaction();
            return;
        }

        const entity e = static_cast<entity>(selectedID);
        auto* col = reg.get_component<collider_component>(e);
        auto* meshCol = reg.get_component<mesh_collider_component>(e);
        const auto* transform = reg.get_component<transform_component>(e);
        if ((!col && !meshCol) || !transform)
        {
            reset_interaction();
            return;
        }

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);
        const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
        const glm::vec3 worldPos = glm::vec3(worldMat[3]);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // ====================================================================
        // MESH COLLIDER (AG CARPISTIRICI) TEL KAFES CIZIMI
        // ====================================================================
        // Mesh Collider tel kafes cizimi 1:1 tam cozunurluk ve 3B derinlik testiyle gizmo_pass::render icerisinde GPU VBO uzerinden yapilir.
        if (meshCol && (meshCol->is_dirty || meshCol->cooked_vertices.empty()))
        {
            qual::physics::mesh_collider_baker::bake(e, reg, engineCtx.aria_system, *meshCol);
        }
        if (false && meshCol)
        {
            if (meshCol->is_dirty || meshCol->cooked_vertices.empty())
            {
                qual::physics::mesh_collider_baker::bake(e, reg, engineCtx.aria_system, *meshCol);
            }

            const size_t numVerts = meshCol->cooked_vertices.size();
            const size_t numIndices = meshCol->cooked_indices.size();

            if (numVerts > 0 && numIndices >= 3)
            {
                const ImU32 wireCol = IM_COL32(45, 235, 95, 200);
                std::vector<ImVec2> screenPts(numVerts);
                std::vector<uint8_t> valid(numVerts, 0);

                for (size_t i = 0; i < numVerts; ++i)
                {
                    const glm::vec3 wPt = glm::vec3(worldMat * glm::vec4(meshCol->cooked_vertices[i], 1.0f));
                    ImVec2 sp;
                    if (gizmo_math::project_world_to_screen(wPt, viewProj, viewportPos, viewportSize, sp))
                    {
                        screenPts[i] = sp;
                        valid[i] = 1;
                    }
                }

                // Cok yogun modellerde editorde 60 FPS'i korumak icin adimlama uygula
                const size_t numTris = numIndices / 3;
                const size_t step = (numTris > 6000) ? (numTris / 6000) : 1;

                for (size_t t = 0; t < numTris; t += step)
                {
                    const uint32_t i0 = meshCol->cooked_indices[t * 3 + 0];
                    const uint32_t i1 = meshCol->cooked_indices[t * 3 + 1];
                    const uint32_t i2 = meshCol->cooked_indices[t * 3 + 2];

                    if (i0 < numVerts && i1 < numVerts && valid[i0] && valid[i1])
                        dl->AddLine(screenPts[i0], screenPts[i1], wireCol, 1.2f);
                    if (i1 < numVerts && i2 < numVerts && valid[i1] && valid[i2])
                        dl->AddLine(screenPts[i1], screenPts[i2], wireCol, 1.2f);
                    if (i2 < numVerts && i0 < numVerts && valid[i2] && valid[i0])
                        dl->AddLine(screenPts[i2], screenPts[i0], wireCol, 1.2f);
                }
            }
        }

        if (!col)
        {
            reset_interaction();
            return;
        }

        // Yonelim (Rotation) ve Olcek (Scale) vektorlerini dunya matrisinden ayristir
        glm::vec3 worldScale;
        worldScale.x = glm::length(glm::vec3(worldMat[0]));
        worldScale.y = glm::length(glm::vec3(worldMat[1]));
        worldScale.z = glm::length(glm::vec3(worldMat[2]));
        const glm::vec3 effScale = glm::max(worldScale, glm::vec3(0.001f));

        glm::mat3 rotMat;
        rotMat[0] = glm::vec3(worldMat[0]) / effScale.x;
        rotMat[1] = glm::vec3(worldMat[1]) / effScale.y;
        rotMat[2] = glm::vec3(worldMat[2]) / effScale.z;

        // Carpistirici yerel ofsetinin dunya uzayindaki merkezi
        const glm::vec3 colliderCenter = worldPos + rotMat * (col->offset * effScale);

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (!mouseDown)
        {
            m_ActiveHandle = -1;
        }

        const gizmo_math::Ray3D mouseRay = gizmo_math::screen_to_world_ray(
            mousePos, invViewProj, viewportPos, viewportSize);

        // ====================================================================
        // 1. KUTU ÇARPIŞTIRICI (BOX COLLIDER)
        // ====================================================================
        if (col->type == collider_type::box)
        {
            // Ölçekle orantılı etkin yarım boyutlar (Half-extents)
            const glm::vec3 effHalf = col->extents * effScale;

            // 8 Köşe Noktası
            glm::vec3 corners[8];
            for (int i = 0; i < 8; ++i)
            {
                const glm::vec3 localCorner(
                    (i & 1) ? effHalf.x : -effHalf.x,
                    (i & 2) ? effHalf.y : -effHalf.y,
                    (i & 4) ? effHalf.z : -effHalf.z
                );
                corners[i] = colliderCenter + rotMat * localCorner;
            }

            // 12 Çerçeve Kenarı (Wireframe)
            const int edges[12][2] = {
                {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Alt yüz
                {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Üst yüz
                {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Dikey sütunlar
            };

            const ImU32 wireCol = IM_COL32(45, 235, 95, 240);
            for (int i = 0; i < 12; ++i)
            {
                ImVec2 sp1, sp2;
                if (gizmo_math::project_world_to_screen(corners[edges[i][0]], viewProj, viewportPos, viewportSize, sp1) &&
                    gizmo_math::project_world_to_screen(corners[edges[i][1]], viewProj, viewportPos, viewportSize, sp2))
                {
                    dl->AddLine(sp1, sp2, wireCol, 1.8f);
                }
            }

            // 6 Yüz Tutamacı (+X, -X, +Y, -Y, +Z, -Z)
            struct ColliderHandlePoint
            {
                int id;
                glm::vec3 worldPt;
                glm::vec3 axisDir;
                int axisIndex; // 0: X, 1: Y, 2: Z
            };

            std::vector<ColliderHandlePoint> handles;
            handles.push_back({ 0, colliderCenter + rotMat * glm::vec3( effHalf.x, 0, 0),  rotMat[0], 0 });
            handles.push_back({ 1, colliderCenter + rotMat * glm::vec3(-effHalf.x, 0, 0), -rotMat[0], 0 });
            handles.push_back({ 2, colliderCenter + rotMat * glm::vec3(0,  effHalf.y, 0),  rotMat[1], 1 });
            handles.push_back({ 3, colliderCenter + rotMat * glm::vec3(0, -effHalf.y, 0), -rotMat[1], 1 });
            handles.push_back({ 4, colliderCenter + rotMat * glm::vec3(0, 0,  effHalf.z),  rotMat[2], 2 });
            handles.push_back({ 5, colliderCenter + rotMat * glm::vec3(0, 0, -effHalf.z), -rotMat[2], 2 });

            // Hover Tespiti
            if (m_ActiveHandle == -1)
            {
                m_HoveredHandle = -1;
                float bestDist = 12.0f;
                for (const auto& h : handles)
                {
                    ImVec2 sp;
                    if (gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                    {
                        const float d = std::hypot(mousePos.x - sp.x, mousePos.y - sp.y);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            m_HoveredHandle = h.id;
                        }
                    }
                }

                if (m_HoveredHandle != -1 && mouseClicked)
                {
                    m_ActiveHandle = m_HoveredHandle;
                    m_DragStartExtents = col->extents;
                }
            }

            // Aktif Sürükleme ile Boyutlandırma
            if (m_ActiveHandle != -1 && mouseDown)
            {
                for (const auto& h : handles)
                {
                    if (h.id != m_ActiveHandle) continue;

                    const float param = gizmo_math::ray_axis_closest_param(mouseRay, colliderCenter, h.axisDir);
                    const float newWorldDist = std::max(0.01f, param);

                    col->extents[h.axisIndex] = std::max(0.01f, newWorldDist / effScale[h.axisIndex]);

                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "Box Collider Size: X: %.2f  Y: %.2f  Z: %.2f",
                        col->extents.x * 2.0f, col->extents.y * 2.0f, col->extents.z * 2.0f);
                    ImGui::SetTooltip("%s", buf);
                    break;
                }
            }

            // Tutamaçları Çiz (Unity tarzı yeşil/sarı kareler)
            for (const auto& h : handles)
            {
                ImVec2 sp;
                if (!gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                    continue;

                const bool isHot = (m_HoveredHandle == h.id) || (m_ActiveHandle == h.id);
                const float halfSz = isHot ? 5.5f : 3.8f;

                const ImU32 fillCol = isHot ? IM_COL32(255, 235, 50, 255) : IM_COL32(45, 240, 95, 240);
                const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

                dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
                dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
            }
        }
        // ====================================================================
        // 2. KÜRE ÇARPIŞTIRICI (SPHERE COLLIDER)
        // ====================================================================
        else if (col->type == collider_type::sphere)
        {
            const float maxScale = std::max({ effScale.x, effScale.y, effScale.z });
            const float effRadius = std::max(0.01f, col->extents.x * maxScale);

            // 3 Eksen Çemberi Çizimi (XY, XZ, YZ)
            const int segs = 32;
            const ImU32 wireCol = IM_COL32(45, 235, 95, 240);

            auto drawCircle = [&](const glm::vec3& axis1, const glm::vec3& axis2)
            {
                ImVec2 prevSp;
                bool hasPrev = false;
                for (int i = 0; i <= segs; ++i)
                {
                    const float angle = (static_cast<float>(i) / static_cast<float>(segs)) * 6.2831853f;
                    const glm::vec3 pt = colliderCenter + (axis1 * std::cos(angle) + axis2 * std::sin(angle)) * effRadius;
                    ImVec2 sp;
                    if (gizmo_math::project_world_to_screen(pt, viewProj, viewportPos, viewportSize, sp))
                    {
                        if (hasPrev)
                        {
                            dl->AddLine(prevSp, sp, wireCol, 1.6f);
                        }
                        prevSp = sp;
                        hasPrev = true;
                    }
                    else
                    {
                        hasPrev = false;
                    }
                }
            };

            drawCircle(rotMat[0], rotMat[1]); // XY
            drawCircle(rotMat[0], rotMat[2]); // XZ
            drawCircle(rotMat[1], rotMat[2]); // YZ

            // 6 Yarıçap Tutamacı
            struct SphereHandlePoint
            {
                int id;
                glm::vec3 worldPt;
                glm::vec3 axisDir;
            };

            std::vector<SphereHandlePoint> handles;
            handles.push_back({ 0, colliderCenter + rotMat[0] * effRadius,  rotMat[0] });
            handles.push_back({ 1, colliderCenter - rotMat[0] * effRadius, -rotMat[0] });
            handles.push_back({ 2, colliderCenter + rotMat[1] * effRadius,  rotMat[1] });
            handles.push_back({ 3, colliderCenter - rotMat[1] * effRadius, -rotMat[1] });
            handles.push_back({ 4, colliderCenter + rotMat[2] * effRadius,  rotMat[2] });
            handles.push_back({ 5, colliderCenter - rotMat[2] * effRadius, -rotMat[2] });

            // Hover Tespiti
            if (m_ActiveHandle == -1)
            {
                m_HoveredHandle = -1;
                float bestDist = 12.0f;
                for (const auto& h : handles)
                {
                    ImVec2 sp;
                    if (gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                    {
                        const float d = std::hypot(mousePos.x - sp.x, mousePos.y - sp.y);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            m_HoveredHandle = h.id;
                        }
                    }
                }

                if (m_HoveredHandle != -1 && mouseClicked)
                {
                    m_ActiveHandle = m_HoveredHandle;
                    m_DragStartExtents = col->extents;
                }
            }

            // Aktif Sürükleme ile Yarıçap Güncellemesi
            if (m_ActiveHandle != -1 && mouseDown)
            {
                for (const auto& h : handles)
                {
                    if (h.id != m_ActiveHandle) continue;

                    const float param = gizmo_math::ray_axis_closest_param(mouseRay, colliderCenter, h.axisDir);
                    const float newWorldDist = std::max(0.01f, param);

                    col->extents.x = std::max(0.01f, newWorldDist / maxScale);
                    col->extents.y = col->extents.x;
                    col->extents.z = col->extents.x;

                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Sphere Collider Radius: %.2f m", col->extents.x);
                    ImGui::SetTooltip("%s", buf);
                    break;
                }
            }

            // Tutamaçları Çiz
            for (const auto& h : handles)
            {
                ImVec2 sp;
                if (!gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                    continue;

                const bool isHot = (m_HoveredHandle == h.id) || (m_ActiveHandle == h.id);
                const float halfSz = isHot ? 5.5f : 3.8f;

                const ImU32 fillCol = isHot ? IM_COL32(255, 235, 50, 255) : IM_COL32(45, 240, 95, 240);
                const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

                dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
                dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
            }
        }
    }
} // namespace vesta

