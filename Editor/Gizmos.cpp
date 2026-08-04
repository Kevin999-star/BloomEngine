#define GLM_ENABLE_EXPERIMENTAL
#include "Gizmos.h"
#include "../External/ImGui/imgui.h"
#include "../External/ImGui/ImGuizmo.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

GizmoMode Gizmos::s_Mode = GizmoMode::Translate;

void Gizmos::Draw(const glm::mat4& view, const glm::mat4& proj, glm::vec3& position)
{
    ImGuizmo::BeginFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);

    ImGuizmo::OPERATION op =
        (s_Mode == GizmoMode::Translate) ? ImGuizmo::TRANSLATE :
        (s_Mode == GizmoMode::Rotate) ? ImGuizmo::ROTATE :
        ImGuizmo::SCALE;

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(proj),
        op,
        ImGuizmo::LOCAL,
        glm::value_ptr(model)
    );

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 t, r, s;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model),
            &t.x, &r.x, &s.x);
        position = t;
    }
}

