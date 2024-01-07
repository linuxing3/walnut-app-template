#pragma once

#include "Walnut/Image.h"

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"
#include "glm/fwd.hpp"

#include <cstdint>
#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <memory>

class Renderer {
public:
  struct Settings {
    bool acumulate = true;
  };

  Renderer() = default;

  void OnResize(uint32_t width, uint32_t height);
  void Render(const Scene &scene, const Camera &camera);

  std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

  void ResetFrameIndex() { m_FrameIndex = 1; }

  Settings &GetSettings() { return m_Settings; }

private:
  // Hit point info
  struct HitPayload {
    float HitDistance;
    glm::vec3 WorldPosition;
    glm::vec3 WorldNormal;
    int ObjectIndex;

    float u;
    float v;

    static void set_sphere_uv(const glm::vec3 &p, float &u, float &v) {
      // p: a given point on the sphere of radius one, centered at the origin.
      // u: returned value [0,1] of angle around the Y axis from X=-1.
      // v: returned value [0,1] of angle from Y=-1 to Y=+1.
      //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
      //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
      //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

      auto theta = acos(-p.y);
      auto phi = atan2(-p.z, p.x) + 3.1415926f;

      u = phi / (2 * 3.1415926f);
      v = theta / 3.1415926f;
    }
  };

  // Ray gen perpixel color [light,material,reflect]
  // <- trace <- closestHit/miss
  glm::vec4 PerPixel(uint32_t x, uint32_t y);
  glm::vec4 PerPixelSolidColor(uint32_t x, uint32_t y);
  glm::vec4 PerPixelSimpleColor(uint32_t x, uint32_t y);

  HitPayload TraceRay(const Ray &ray);
  HitPayload ClosestHit(const Ray &ray, float hitDistance, int objectIndex);
  HitPayload Miss(const Ray &ray);

private:
  std::shared_ptr<Walnut::Image> m_FinalImage;

  const Scene *m_ActiveScene = nullptr;
  const Camera *m_ActiveCamera = nullptr;

  std::vector<uint32_t> m_VerticalIter;
  std::vector<uint32_t> m_HorizotalIter;

  Settings m_Settings;

  uint32_t *m_ImageData = nullptr;
  glm::vec4 *m_ImageAccumulatedData = nullptr;

  uint32_t m_FrameIndex = 1;
};
