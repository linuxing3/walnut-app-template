/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "Layer.h"
#include "ResourceManager.h"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>
#include <webgpu.hpp>

#include <glm/glm.hpp>

#include <array>
#include <filesystem>

using namespace wgpu;
using namespace std;

class Application {
public:
  Application();
  static Application *Get();
  // A function called only once at the beginning. Returns false is init failed.
  bool onInit();

  void onCompute();
  // A function called at each frame, guarantied never to be called before
  // `onInit`.
  void onFrame();

  // A function called only once at the very end.
  void onFinish();

  // A function called when the window is resized.
  void onResize();

  void onMouseMove(double xpos, double ypos);
  // Mouse events
  void onMouseButton(int button, int action, int mods);
  void onScroll(double xoffset, double yoffset);

  // A function that tells if the application is still running.
  bool isRunning();

  Device GetDevice() { return m_device; };
  Queue GetQueue() { return m_device.getQueue(); };

  GLFWwindow *GetWindowHandle() { return m_window; };

  TextureView GetCurrentTextureView() { return m_currentTextureView; }
  TextureView GetCurrentDepthView() { return m_depthTextureView; }
  vector<BindGroupEntry> GetBindings() { return m_bindingEntries; };

  static CommandBuffer
  RunSingleCommand(function<void(RenderPassEncoder renderPass)> &&renderFunc);
  static CommandBuffer RunSingleCommand(function<void()> &&prepareFunc);

private:
  void buildWindow();
  void buildDeviceObject();
  void buildSwapChain();
  void buildRenderPipeline();
  void buildComputePipeline();
  void buildDepthBuffer();
  void updateViewMatrix();
  void updateDragInertia();

  void initGui();                               // called in onInit
  void updateGui(RenderPassEncoder renderPass); // called in onFrame

  bool initTexture(const filesystem::path &path);

  void initLighting();
  void updateLighting();
  void initComputeBindGroup();
  void terminateBindGroup();

  void initComputeBindGroupLayout();
  void terminateBindGroupLayout();

  void initComputePipeline();
  void terminateComputePipeline();

  void initComputeBuffers();
  void terminateBuffers();

private:
  using vec2 = glm::vec2;
  using vec3 = glm::vec3;
  using vec4 = glm::vec4;
  using mat4x4 = glm::mat4x4;
  TextureFormat m_swapChainFormat = TextureFormat::RGBA8UnormSrgb;
  TextureFormat m_depthTextureFormat = TextureFormat::Depth32Float;

  // Everything that is initialized in `onInit` and needed in `onFrame`.
  GLFWwindow *m_window = nullptr;
  Instance m_instance = nullptr;
  Surface m_surface = nullptr;
  Device m_device = nullptr;

  SwapChainDescriptor m_swapChainDesc{};
  SwapChain m_swapChain = nullptr;

  RenderPipeline m_pipeline = nullptr;
  ComputePipeline m_computePipeline = nullptr;
  PipelineLayout m_pipelineLayout = nullptr;

  // --------------------------------------------------------
  // BindGroup = BindGroupLayoutEntry + BindingGroupEntry
  // --------------------------------------------------------
  BindGroup m_bindGroup = nullptr;
  vector<BindGroupEntry> m_bindingEntries;
  vector<BindGroupLayoutEntry> m_bindingLayoutEntries;

  BindGroupLayout m_bindGroupLayout = nullptr;
  BindGroup m_computeBindGroup = nullptr;
  vector<BindGroupEntry> m_computeBindingEntries;
  vector<BindGroupLayoutEntry> m_computeBindingLayoutEntries;
  uint32_t m_bufferSize = 1000;
  wgpu::Buffer m_inputBuffer = nullptr;
  wgpu::Buffer m_outputBuffer = nullptr;
  wgpu::Buffer m_mapBuffer = nullptr;
  // --------------------------------------------------------
  // Vertex part
  vector<ResourceManager::VertexAttributes> m_vertexData;
  Buffer m_vertexBuffer = nullptr;
  int m_indexCount;

  // @group(0) @binding(0) var<uniform> uMyUniforms : MyUniforms;
  struct MyUniforms {
    mat4x4 projectionMatrix;
    mat4x4 viewMatrix;
    mat4x4 modelMatrix;
    vec4 color;
    vec3 cameraWorldPosition;
    float time;
  };
  static_assert(sizeof(MyUniforms) % 16 == 0);

  Buffer m_uniformBuffer = nullptr;
  MyUniforms m_uniforms;

  // --------------------------------------------------------
  // Fragement part
  // --------------------------------------------------------
  // @group(0) @binding(1) var textureSampler : sampler;

  // @group(0) @binding(2) var baseColorTexture : texture_2d<f32>;
  vector<Texture> m_textures;
  TextureView m_currentTextureView = nullptr;
  Texture m_depthTexture = nullptr;
  TextureView m_depthTextureView = nullptr;

  // @group(0) @binding(3) var<uniform> uLighting : LightingUniforms;
  struct LightingUniforms {
    array<vec4, 2> directions;
    array<vec4, 2> colors;
    float hardness;
    float kd;
    float ks;
    float _pad;
  };
  static_assert(sizeof(LightingUniforms) % 16 == 0);
  LightingUniforms m_lightingUniforms;
  Buffer m_lightingUniformBuffer = nullptr;
  bool m_lightingUniformsChanged = false;
  // --------------------------------------------------------

  // --------------------------------------------------------
  // Camera & Animation control
  // --------------------------------------------------------
  struct CameraState {
    // angles.x is the rotation of the camera around the global vertical axis,
    // affected by mouse.x angles.y is the rotation of the camera around its
    // local horizontal axis, affected by mouse.y
    vec2 angles = {0.8f, 0.5f};
    // zoom is the position of the camera along its local forward axis, affected
    // by the scroll wheel
    float zoom = -1.2f;
  };

  struct DragState {
    // Whether a drag action is ongoing (i.e., we are between mouse press and
    // mouse release)
    bool active = false;
    // The position of the mouse at the beginning of the drag action
    vec2 startMouse;
    // The camera state at the beginning of the drag action
    CameraState startCameraState;

    // Constant settings
    float sensitivity = 0.01f;
    float scrollSensitivity = 0.1f;

    // Inertia
    vec2 velocity = {0.0, 0.0};
    vec2 previousDelta;
    float intertia = 0.9f;
  };

  CameraState m_cameraState;
  DragState m_drag;
  // --------------------------------------------------------

  unique_ptr<ErrorCallback> m_uncapturedErrorCallback;

public:
  vector<shared_ptr<Walnut::Layer>> m_LayerStack;

  template <typename T> void PushLayer() {
    static_assert(is_base_of<Walnut::Layer, T>::value,
                  "Pushed type is not subclass of Layer!");
    m_LayerStack.emplace_back(make_shared<T>())->OnAttach();
  }

  void PushLayer(const shared_ptr<Walnut::Layer> &layer) {
    m_LayerStack.emplace_back(layer);
    layer->OnAttach();
  }
};
