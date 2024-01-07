#include "Application.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Camera.h"
#include "Renderer.h"
#include "imgui.h"

#include <float.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>

using namespace Walnut;

class RayLayer : public Walnut::Layer {
public:
  RayLayer() : m_Camera(45.0f, 0.1f, 100.0f) {
    Material &pinkSphere = m_Scene.Materials.emplace_back();
    pinkSphere.Albedo = {1.0f, 0.0f, 1.0f};
    pinkSphere.Roughness = 0.0f;

    Material &blueSphere = m_Scene.Materials.emplace_back();
    blueSphere.Albedo = {0.2f, 0.3f, 1.0f};
    blueSphere.Roughness = 0.1f;

    Material &orangeSphere = m_Scene.Materials.emplace_back();
    orangeSphere.Albedo = {0.8f, 0.5f, 0.2f};
    orangeSphere.Roughness = 0.1f;
    orangeSphere.EmissionColor = orangeSphere.Albedo;
    orangeSphere.EmissionPower = 2.0f;

    {
      Sphere sphere;
      sphere.Position = {0.0f, 0.0f, 0.0f};
      sphere.Radius = 1.0f;
      sphere.MaterialIndex = 0;
      m_Scene.Spheres.push_back(sphere);
    }

    {
      Sphere sphere;
      sphere.Position = {2.0f, 0.0f, 0.0f};
      sphere.Radius = 1.0f;
      sphere.MaterialIndex = 2;
      m_Scene.Spheres.push_back(sphere);
    }

    {
      Sphere sphere;
      sphere.Position = {0.0f, -101.0f, 0.0f};
      sphere.Radius = 100.0f;
      sphere.MaterialIndex = 1;
      m_Scene.Spheres.push_back(sphere);
    }

    //
  }

  virtual void OnUpdate(float ts) override { m_Camera.OnUpdate(ts); }

  virtual void OnUIRender() override {
    ImGui::Begin("Settings");
    ImGui::Text("Last render: %.3fms", m_LastRenderTime);
    if (ImGui::Button("Render")) {
      Render();
    }
    ImGui::Checkbox("Accumuate", &m_Renderer.GetSettings().acumulate);
    if (ImGui::Button("Reset"))
      m_Renderer.ResetFrameIndex();
    ImGui::End();

    ImGui::Begin("Scene");
    for (size_t i = 0; i < m_Scene.Spheres.size(); i++) {
      ImGui::PushID(i);

      Sphere &sphere = m_Scene.Spheres[i];
      ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
      ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
      ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0,
                     (int)m_Scene.Materials.size() - 1);

      ImGui::Separator();

      ImGui::PopID();
    }

    ImGui::Separator();

    for (size_t i = 0; i < m_Scene.Materials.size(); i++) {
      ImGui::PushID(i);

      Material &material = m_Scene.Materials[i];
      ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
      ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
      ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);
      ImGui::ColorEdit3("Emission color",
                        glm::value_ptr(material.EmissionColor));
      ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f,
                       FLT_MAX);

      ImGui::Separator();

      ImGui::PopID();
    }

    ImGui::End();

    // Viewport to show Walnut Image in Imgui window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    m_ViewportWidth = ImGui::GetContentRegionAvail().x;
    m_ViewportHeight = ImGui::GetContentRegionAvail().y;

    // Call renderer
    auto image = m_Renderer.GetFinalImage();
    if (image) {
      ImGui::Image(image->GetTextureId(),
                   {(float)image->GetWidth(), (float)image->GetHeight()},
                   ImVec2(0, 1), ImVec2(1, 0));
    }

    ImGui::End();
    ImGui::PopStyleVar();

    Render();
  }

  void Render() {
    Timer timer;

    m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);

    // render procedure
    m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
    m_Renderer.Render(m_Scene, m_Camera);

    m_LastRenderTime = timer.ElapsedMillis();
  }

private:
  Renderer m_Renderer;
  Camera m_Camera;
  Scene m_Scene;
  uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

  float m_LastRenderTime = 0.0F;
};
