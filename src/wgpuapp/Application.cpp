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

#include "Application.h"
#include "ResourceManager.h"

#define WEBGPU_BACKEND_WGPU
#include "webgpu.h"
#include "webgpu.hpp"
#include "wgpu.h" // wgpuTextureViewDrop

#include "glfw3webgpu.h"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <sys/types.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include "imgui_impl_wgpu.h"
#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Emedded font
#include "ImGui/Roboto-Bold.embed"
#include "ImGui/Roboto-Italic.embed"
#include "ImGui/Roboto-Regular.embed"

#define RESOURCE_DIR "./wgpu"

// NOTE: Singleton design pattern
static Application *s_Instance;

constexpr float PI = 3.14159265358979323846f;

using namespace wgpu;
using VertexAttributes = ResourceManager::VertexAttributes;
using glm::mat4x4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

// GLFW callbacks
void onWindowResize(GLFWwindow *window, int width, int height) {
  (void)width;
  (void)height;
  auto pApp = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  if (pApp != nullptr)
    pApp->onResize();
}
void onWindowMouseMove(GLFWwindow *window, double xpos, double ypos) {
  auto pApp = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  if (pApp != nullptr)
    pApp->onMouseMove(xpos, ypos);
}
void onWindowMouseButton(GLFWwindow *window, int button, int action, int mods) {
  auto pApp = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  if (pApp != nullptr)
    pApp->onMouseButton(button, action, mods);
}
void onWindowScroll(GLFWwindow *window, double xoffset, double yoffset) {
  auto pApp = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
  if (pApp != nullptr)
    pApp->onScroll(xoffset, yoffset);
}

void Application::buildWindow() {

  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
  }

  // Create window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  m_window = glfwCreateWindow(1920, 1050, "Learn WebGPU", nullptr, nullptr);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
  }

  // Add window callbacks
  glfwSetWindowUserPointer(m_window, this);
  glfwSetFramebufferSizeCallback(m_window, onWindowResize);
  glfwSetCursorPosCallback(m_window, onWindowMouseMove);
  glfwSetMouseButtonCallback(m_window, onWindowMouseButton);
  glfwSetScrollCallback(m_window, onWindowScroll);
}

void Application::buildDeviceObject() {

  // Create instance
  m_instance = createInstance(InstanceDescriptor{});
  if (!m_instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
  }

  // Create surface and adapter
  std::cout << "Requesting adapter..." << std::endl;
  m_surface = glfwGetWGPUSurface(m_instance, m_window);
  RequestAdapterOptions adapterOpts{};
  adapterOpts.compatibleSurface = m_surface;
  Adapter adapter = m_instance.requestAdapter(adapterOpts);
  std::cout << "Got adapter: " << adapter << std::endl;

  std::cout << "Requesting device..." << std::endl;
  SupportedLimits supportedLimits;
  adapter.getLimits(&supportedLimits);
  RequiredLimits requiredLimits = Default;
  requiredLimits.limits.maxVertexAttributes = 4;
  requiredLimits.limits.maxVertexBuffers = 1;
  requiredLimits.limits.maxBindGroups = 2;
  requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
  requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
  requiredLimits.limits.maxComputeWorkgroupSizeY = 256;
  requiredLimits.limits.maxComputeWorkgroupSizeY = 256;
  requiredLimits.limits.maxComputeWorkgroupSizeZ = 64;
  requiredLimits.limits.maxComputeInvocationsPerWorkgroup = 256;
  requiredLimits.limits.maxComputeWorkgroupsPerDimension = 256;
  requiredLimits.limits.minStorageBufferOffsetAlignment = 256;

  // Create device
  DeviceDescriptor deviceDesc{};
  deviceDesc.label = "My Device";
  deviceDesc.requiredFeaturesCount = 0;
  deviceDesc.requiredLimits = &requiredLimits;
  deviceDesc.defaultQueue.label = "The default queue";
  m_device = adapter.requestDevice(deviceDesc);
  std::cout << "Got device: " << m_device << std::endl;

  // Add an error callback for more debug info
  m_uncapturedErrorCallback = m_device.setUncapturedErrorCallback(
      [](ErrorType type, char const *message) {
        std::cout << "Device error: type " << type;
        if (message)
          std::cout << " (message: " << message << ")";
        std::cout << std::endl;
      });
  // Create swapchain
  m_swapChainFormat = m_surface.getPreferredFormat(adapter);
  buildSwapChain();
}

void Application::buildRenderPipeline() {

  Queue queue = m_device.getQueue();
  // Create pipeline

  // [GPU] shader
  std::cout << "Creating shader module..." << std::endl;
  ShaderModule shaderModule =
      ResourceManager::loadShaderModule(RESOURCE_DIR "/shader.wsl", m_device);
  std::cout << "Shader module: " << shaderModule << std::endl;

  // [CPU] load geometry
  bool success = ResourceManager::loadGeometryFromObj(
      RESOURCE_DIR "/fourareen.obj", m_vertexData);
  if (!success) {
    std::cerr << "Could not load geometry!" << std::endl;
  }

  // [GPU] Create vertex buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = m_vertexData.size() * sizeof(VertexAttributes);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  m_vertexBuffer = m_device.createBuffer(bufferDesc);
  queue.writeBuffer(m_vertexBuffer, 0, m_vertexData.data(), bufferDesc.size);

  m_indexCount = static_cast<int>(m_vertexData.size());

  // [GPU] Create uniform buffer
  bufferDesc.size = sizeof(MyUniforms);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.mappedAtCreation = false;
  m_uniformBuffer = m_device.createBuffer(bufferDesc);

  // Upload the initial value of the uniforms
  m_uniforms.time = 1.0f;
  m_uniforms.color = {0.0f, 1.0f, 0.4f, 1.0f};

  // Matrices
  m_uniforms.modelMatrix = mat4x4(1.0);
  m_uniforms.viewMatrix =
      glm::lookAt(vec3(-2.0f, -3.0f, 2.0f), vec3(0.0f), vec3(0, 0, 1));
  m_uniforms.projectionMatrix =
      glm::perspective(45 * PI / 180, 640.0f / 480.0f, 0.01f, 100.0f);

  queue.writeBuffer(m_uniformBuffer, 0, &m_uniforms, sizeof(MyUniforms));
  updateViewMatrix();

  // [GPU] Create depth buffer
  buildDepthBuffer();

  // [GPU] Create a sampler
  SamplerDescriptor samplerDesc;
  samplerDesc.addressModeU = AddressMode::Repeat;
  samplerDesc.addressModeV = AddressMode::Repeat;
  samplerDesc.addressModeW = AddressMode::Repeat;
  samplerDesc.magFilter = FilterMode::Linear;
  samplerDesc.minFilter = FilterMode::Linear;
  samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
  samplerDesc.lodMinClamp = 0.0f;
  samplerDesc.lodMaxClamp = 32.0f;
  samplerDesc.compare = CompareFunction::Undefined;
  samplerDesc.maxAnisotropy = 1;
  Sampler sampler = m_device.createSampler(samplerDesc);

  // ---------------------------------------------------------//
  // [GPU] Create binding layout
  m_bindingLayoutEntries.resize(2, Default);

  BindGroupLayoutEntry &bindingLayout = m_bindingLayoutEntries[0];
  bindingLayout.binding = 0;
  bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;
  bindingLayout.buffer.type = BufferBindingType::Uniform;
  bindingLayout.buffer.minBindingSize = sizeof(MyUniforms); // from CPU

  BindGroupLayoutEntry &samplerBindingLayout = m_bindingLayoutEntries[1];
  samplerBindingLayout.binding = 1;
  samplerBindingLayout.visibility = ShaderStage::Fragment;
  samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

  // ---------------------------------------------------------//
  // [GPU] Create bindgroup entries
  m_bindingEntries.resize(2);

  /* @group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms; */
  m_bindingEntries[0].binding = 0;
  m_bindingEntries[0].buffer = m_uniformBuffer;
  m_bindingEntries[0].offset = 0;
  m_bindingEntries[0].size = sizeof(MyUniforms);

  /* @group(0) @binding(1) var textureSampler : sampler; */
  m_bindingEntries[1].binding = 1;
  m_bindingEntries[1].sampler = sampler;

  /* @group(0) @binding(2) var baseColorTexture: texture_2d<f32>; */
  if (!initTexture(RESOURCE_DIR "/fourareen2K_albedo.jpg"))
    cout << "Failed to init texture";

  /* @group(0) @binding(3) var<uniform> uLighting: LightingUniforms; */
  initLighting();

  std::cout << "Creating render pipeline..." << std::endl;
  RenderPipelineDescriptor pipelineDesc{};

  // Vertex stage
  std::vector<VertexAttribute> vertexAttribs(4);

  // Position attribute
  vertexAttribs[0].shaderLocation = 0;
  vertexAttribs[0].format = VertexFormat::Float32x3;
  vertexAttribs[0].offset = offsetof(VertexAttributes, position);

  // Normal attribute
  vertexAttribs[1].shaderLocation = 1;
  vertexAttribs[1].format = VertexFormat::Float32x3;
  vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

  // Color attribute
  vertexAttribs[2].shaderLocation = 2;
  vertexAttribs[2].format = VertexFormat::Float32x3;
  vertexAttribs[2].offset = offsetof(VertexAttributes, color);

  // UV attribute
  vertexAttribs[3].shaderLocation = 3;
  vertexAttribs[3].format = VertexFormat::Float32x2;
  vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

  VertexBufferLayout vertexBufferLayout;
  vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
  vertexBufferLayout.attributes = vertexAttribs.data();
  vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
  vertexBufferLayout.stepMode = VertexStepMode::Vertex;

  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;

  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vs_main";
  pipelineDesc.vertex.constantCount = 0;
  pipelineDesc.vertex.constants = nullptr;

  pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
  pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
  pipelineDesc.primitive.frontFace = FrontFace::CCW;
  pipelineDesc.primitive.cullMode = CullMode::None;

  FragmentState fragmentState{};
  pipelineDesc.fragment = &fragmentState;
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = "fs_main";
  fragmentState.constantCount = 0;
  fragmentState.constants = nullptr;

  BlendState blendState{};
  blendState.color.srcFactor = BlendFactor::SrcAlpha;
  blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
  blendState.color.operation = BlendOperation::Add;
  blendState.alpha.srcFactor = BlendFactor::Zero;
  blendState.alpha.dstFactor = BlendFactor::One;
  blendState.alpha.operation = BlendOperation::Add;

  ColorTargetState colorTarget{};
  colorTarget.format = m_swapChainFormat;
  colorTarget.blend = &blendState;
  colorTarget.writeMask = ColorWriteMask::All;

  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;

  DepthStencilState depthStencilState = Default;
  depthStencilState.depthCompare = CompareFunction::Less;
  depthStencilState.depthWriteEnabled = true;
  depthStencilState.format = m_depthTextureFormat;
  depthStencilState.stencilReadMask = 0;
  depthStencilState.stencilWriteMask = 0;
  pipelineDesc.depthStencil = &depthStencilState;

  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = false;

  // ---------------------------------------------------------//
  // [GPU] Create a bind group layout
  BindGroupLayoutDescriptor bindGroupLayoutDesc{};
  bindGroupLayoutDesc.entryCount = (uint32_t)m_bindingLayoutEntries.size();
  bindGroupLayoutDesc.entries = m_bindingLayoutEntries.data();
  BindGroupLayout bindGroupLayout =
      m_device.createBindGroupLayout(bindGroupLayoutDesc);

  // ---------------------------------------------------------//
  // [GPU] Create the pipeline layout
  PipelineLayoutDescriptor layoutDesc{};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout *)&bindGroupLayout;
  PipelineLayout pipelineLayout = m_device.createPipelineLayout(layoutDesc);
  pipelineDesc.layout = pipelineLayout;

  // ---------------------------------------------------------//
  // [GPU] Create the pipeline
  m_pipeline = m_device.createRenderPipeline(pipelineDesc);
  std::cout << "Render pipeline: " << m_pipeline << std::endl;

  // ---------------------------------------------------------//
  // [GPU] Create bind group
  BindGroupDescriptor bindGroupDesc{};
  bindGroupDesc.layout = bindGroupLayout;
  bindGroupDesc.entryCount = (uint32_t)m_bindingEntries.size();
  bindGroupDesc.entries = m_bindingEntries.data();
  m_bindGroup = m_device.createBindGroup(bindGroupDesc);
}

void Application::buildSwapChain() {
  int width, height;
  glfwGetFramebufferSize(m_window, &width, &height);

  std::cout << "Creating swapchain..." << std::endl;
  /* m_swapChainDesc{}; */
  m_swapChainDesc.width = (uint32_t)width;
  m_swapChainDesc.height = (uint32_t)height;
  m_swapChainDesc.usage =
      TextureUsage::RenderAttachment | TextureUsage::TextureBinding;
  m_swapChainDesc.format = m_swapChainFormat;
  m_swapChainDesc.presentMode = PresentMode::Fifo;
  m_swapChain = m_device.createSwapChain(m_surface, m_swapChainDesc);
  std::cout << "Swapchain: " << m_swapChain << std::endl;
}

void Application::buildDepthBuffer() {
  // Destroy previously allocated texture
  if (m_depthTexture != nullptr)
    m_depthTexture.destroy();

  std::cout << "Creating depth texture..." << std::endl;
  // Create the depth texture
  TextureDescriptor depthTextureDesc;
  depthTextureDesc.dimension = TextureDimension::_2D;
  depthTextureDesc.format = m_depthTextureFormat;
  depthTextureDesc.mipLevelCount = 1;
  depthTextureDesc.sampleCount = 1;
  depthTextureDesc.size = {m_swapChainDesc.width, m_swapChainDesc.height, 1};
  depthTextureDesc.usage = TextureUsage::RenderAttachment;
  depthTextureDesc.viewFormatCount = 1;
  depthTextureDesc.viewFormats = (WGPUTextureFormat *)&m_depthTextureFormat;
  m_depthTexture = m_device.createTexture(depthTextureDesc);
  std::cout << "Depth texture: " << m_depthTexture << std::endl;

  // Create the view of the depth texture manipulated by the rasterizer
  TextureViewDescriptor depthTextureViewDesc;
  depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
  depthTextureViewDesc.baseArrayLayer = 0;
  depthTextureViewDesc.arrayLayerCount = 1;
  depthTextureViewDesc.baseMipLevel = 0;
  depthTextureViewDesc.mipLevelCount = 1;
  depthTextureViewDesc.dimension = TextureViewDimension::_2D;
  depthTextureViewDesc.format = m_depthTextureFormat;
  m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);
}

bool Application::initTexture(const std::filesystem::path &path) {
  // Create a texture
  TextureView textureView = nullptr;
  Texture texture = ResourceManager::loadTexture(path, m_device, &textureView);
  if (!texture) {
    std::cerr << "Could not load texture!" << std::endl;
    return false;
  }
  m_textures.push_back(texture);

  // Setup binding
  uint32_t bindingIndex = (uint32_t)m_bindingLayoutEntries.size();
  BindGroupLayoutEntry bindingLayout = Default;
  bindingLayout.binding = bindingIndex;
  bindingLayout.visibility = ShaderStage::Fragment;
  bindingLayout.texture.sampleType = TextureSampleType::Float;
  bindingLayout.texture.viewDimension = TextureViewDimension::_2D;
  m_bindingLayoutEntries.push_back(bindingLayout);

  BindGroupEntry binding = Default;
  binding.binding = bindingIndex;
  binding.textureView = textureView;
  m_bindingEntries.push_back(binding);

  return true;
}

void Application::initLighting() {
  Queue queue = m_device.getQueue();

  // Create uniform buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = sizeof(LightingUniforms);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.mappedAtCreation = false;
  m_lightingUniformBuffer = m_device.createBuffer(bufferDesc);

  // Upload the initial value of the uniforms
  m_lightingUniforms.directions = {vec4{0.5, -0.9, 0.1, 0.0},
                                   vec4{0.2, 0.4, 0.3, 0.0}};
  m_lightingUniforms.colors = {vec4{1.0, 0.9, 0.6, 1.0},
                               vec4{0.6, 0.9, 1.0, 1.0}};
  m_lightingUniforms.hardness = 16.0f;
  m_lightingUniforms.kd = 1.0f;
  m_lightingUniforms.ks = 0.5f;

  queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms,
                    sizeof(LightingUniforms));

  // Setup binding
  auto bindingIndex = (uint32_t)m_bindingLayoutEntries.size();
  BindGroupLayoutEntry bindingLayout = Default;
  bindingLayout.binding = bindingIndex;
  bindingLayout.visibility = ShaderStage::Fragment;
  bindingLayout.buffer.type = BufferBindingType::Uniform;
  bindingLayout.buffer.minBindingSize = sizeof(LightingUniforms);
  m_bindingLayoutEntries.push_back(bindingLayout);

  BindGroupEntry binding = Default;
  binding.binding = bindingIndex;
  binding.buffer = m_lightingUniformBuffer;
  binding.offset = 0;
  binding.size = sizeof(LightingUniforms);
  m_bindingEntries.push_back(binding);
}

void Application::updateLighting() {
  if (m_lightingUniformsChanged) {
    Queue queue = m_device.getQueue();
    queue.writeBuffer(m_lightingUniformBuffer, 0, &m_lightingUniforms,
                      sizeof(LightingUniforms));
  }
}

void Application::onFrame() {
  glfwPollEvents();
  Queue queue = m_device.getQueue();

  // Update Lighting, drag, uniform buffer
  updateLighting();
  updateDragInertia();
  m_uniforms.time = static_cast<float>(glfwGetTime());
  queue.writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time),
                    &m_uniforms.time, sizeof(MyUniforms::time));

  {
    TextureView nextTexture = s_Instance->m_swapChain.getCurrentTextureView();
    if (!nextTexture) {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
    }
    s_Instance->Get()->m_currentTextureView = nextTexture;

    auto command = RunSingleCommand([&](RenderPassEncoder renderPass) {
      renderPass.setPipeline(m_pipeline);
      renderPass.setVertexBuffer(
          0, m_vertexBuffer, 0, m_vertexData.size() * sizeof(VertexAttributes));
      renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);
      renderPass.draw(m_indexCount, 1, 0, 0);

      for (auto layer : m_LayerStack) {
        layer->OnUpdate(m_uniforms.time);
      }

      updateGui(renderPass);
      renderPass.end();
    });

    // submit and present
    wgpuTextureViewDrop(nextTexture);
    queue.submit(command);
    m_swapChain.present();
  }
}

void Application::onCompute() {
  Queue queue = m_device.getQueue();

  // Fill in input buffer
  std::vector<float> input(m_bufferSize / sizeof(float));
  for (int i = 0; i < input.size(); ++i) {
    input[i] = 0.1f * i;
  }
  queue.writeBuffer(m_inputBuffer, 0, input.data(),
                    input.size() * sizeof(float));

  // Initialize a command encoder
  CommandEncoderDescriptor encoderDesc = Default;
  CommandEncoder encoder = m_device.createCommandEncoder(encoderDesc);

  // Create compute pass
  ComputePassDescriptor computePassDesc;
  computePassDesc.timestampWriteCount = 0;
  computePassDesc.timestampWrites = nullptr;
  ComputePassEncoder computePass = encoder.beginComputePass(computePassDesc);

  // Use compute pass
  computePass.setPipeline(m_computePipeline);
  computePass.setBindGroup(0, m_bindGroup, 0, nullptr);

  uint32_t invocationCount = m_bufferSize / sizeof(uint32_t);
  uint32_t workgroupSize = 32;
  // This ceils invocationCount / workgroupSize
  uint32_t workgroupCount =
      (invocationCount + workgroupSize - 1) / workgroupSize;
  computePass.dispatchWorkgroups(workgroupCount, 1, 1);

  // Finalize compute pass
  computePass.end();

  // Before encoder.finish
  encoder.copyBufferToBuffer(m_outputBuffer, 0, m_mapBuffer, 0, m_bufferSize);

  // Encode and submit the GPU commands
  CommandBuffer commands = encoder.finish(CommandBufferDescriptor{});
  queue.submit(commands);

  // Print output
  bool done = false;
  auto handle = m_mapBuffer.mapAsync(
      MapMode::Read, 0, m_bufferSize, [&](BufferMapAsyncStatus status) {
        if (status == BufferMapAsyncStatus::Success) {
          const float *output =
              (const float *)m_mapBuffer.getMappedRange(0, m_bufferSize);
          for (int i = 0; i < input.size(); ++i) {
            std::cout << "input " << input[i] << " became " << output[i]
                      << std::endl;
          }
          m_mapBuffer.unmap();
        }
        done = true;
      });

  while (!done) {
    // Checks for ongoing asynchronous operations and call their callbacks if
    // needed
#ifdef WEBGPU_BACKEND_WGPU
    queue.submit(0, nullptr);
#else
#endif
  }
}

bool Application::onInit() {
  m_bufferSize = 64 * sizeof(float);
  buildWindow();
  buildDeviceObject();
/* #define WEBGPU_COMPUTE */
#ifdef WEBGPU_COMPUTE
  initComputeBindGroupLayout();
  initComputePipeline();
  initComputeBuffers();
  initComputeBindGroup();
  /* buildComputePipeline(); */
#else
  buildRenderPipeline();
  initGui();
#endif // DEBUG
  return true;
}

void Application::onFinish() {
  for (auto texture : m_textures) {
    texture.destroy();
  }
  for (auto layer : m_LayerStack) {
    layer->OnDetach();
  }
  m_depthTexture.destroy();

  terminateBindGroup();
  terminateBuffers();
  terminateComputePipeline();
  terminateBindGroupLayout();
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Application::onResize() {
  buildSwapChain();
  buildDepthBuffer();

  float ratio = m_swapChainDesc.width / (float)m_swapChainDesc.height;
  m_uniforms.projectionMatrix =
      glm::perspective(45 * PI / 180, ratio, 0.01f, 100.0f);
  m_device.getQueue().writeBuffer(
      m_uniformBuffer, offsetof(MyUniforms, projectionMatrix),
      &m_uniforms.projectionMatrix, sizeof(MyUniforms::projectionMatrix));
}

void Application::onMouseMove(double xpos, double ypos) {
  if (m_drag.active) {
    vec2 currentMouse = vec2(-(float)xpos, (float)ypos);
    vec2 delta = (currentMouse - m_drag.startMouse) * m_drag.sensitivity;
    m_cameraState.angles = m_drag.startCameraState.angles + delta;
    m_cameraState.angles.y =
        glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
    updateViewMatrix();

    // Inertia
    m_drag.velocity = delta - m_drag.previousDelta;
    m_drag.previousDelta = delta;
  }
}

void Application::onMouseButton(int button, int action, int mods) {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  if (io.WantCaptureMouse) {
    return;
  }

  (void)mods;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    switch (action) {
    case GLFW_PRESS:
      m_drag.active = true;
      double xpos, ypos;
      glfwGetCursorPos(m_window, &xpos, &ypos);
      m_drag.startMouse = vec2(-(float)xpos, (float)ypos);
      m_drag.startCameraState = m_cameraState;
      break;
    case GLFW_RELEASE:
      m_drag.active = false;
      break;
    }
  }
}

void Application::onScroll(double xoffset, double yoffset) {
  (void)xoffset;
  m_cameraState.zoom += m_drag.scrollSensitivity * (float)yoffset;
  m_cameraState.zoom = glm::clamp(m_cameraState.zoom, -2.0f, 2.0f);
  updateViewMatrix();
}

bool Application::isRunning() { return !glfwWindowShouldClose(m_window); }

void Application::updateViewMatrix() {
  float cx = cos(m_cameraState.angles.x);
  float sx = sin(m_cameraState.angles.x);
  float cy = cos(m_cameraState.angles.y);
  float sy = sin(m_cameraState.angles.y);
  vec3 position = vec3(cx * cy, sx * cy, sy) * std::exp(-m_cameraState.zoom);
  m_uniforms.viewMatrix = glm::lookAt(position, vec3(0.0f), vec3(0, 0, 1));
  m_device.getQueue().writeBuffer(
      m_uniformBuffer, offsetof(MyUniforms, viewMatrix), &m_uniforms.viewMatrix,
      sizeof(MyUniforms::viewMatrix));

  m_uniforms.cameraWorldPosition = position;
  m_device.getQueue().writeBuffer(
      m_uniformBuffer, offsetof(MyUniforms, cameraWorldPosition),
      &m_uniforms.cameraWorldPosition, sizeof(MyUniforms::cameraWorldPosition));
}

void Application::updateDragInertia() {
  constexpr float eps = 1e-4f;
  if (!m_drag.active) {
    if (std::abs(m_drag.velocity.x) < eps &&
        std::abs(m_drag.velocity.y) < eps) {
      return;
    }
    m_cameraState.angles += m_drag.velocity;
    m_cameraState.angles.y =
        glm::clamp(m_cameraState.angles.y, -PI / 2 + 1e-5f, PI / 2 - 1e-5f);
    m_drag.velocity *= m_drag.intertia;
    updateViewMatrix();
  }
}

void Application::initGui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Load default font
  ImFontConfig fontConfig;
  fontConfig.FontDataOwnedByAtlas = false;
  ImFont *robotoFont = io.Fonts->AddFontFromMemoryTTF(
      (void *)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
  io.FontDefault = robotoFont;
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(m_window, true);
  ImGui_ImplWGPU_Init(m_device, 3, m_swapChainFormat, m_depthTextureFormat);
}

namespace ImGui {
bool DragDirection(const char *label, vec4 &direction) {
  vec2 angles = glm::degrees(glm::polar(vec3(direction)));
  bool changed = ImGui::DragFloat2(label, glm::value_ptr(angles));
  direction = vec4(glm::euclidean(glm::radians(angles)), direction.w);
  return changed;
}
} // namespace ImGui

void Application::updateGui(RenderPassEncoder renderPass) {
  // Start the Dear ImGui frame
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  bool changed = false;
  ImGui::Begin("Lighting");
  changed = ImGui::ColorEdit3("Color #0",
                              glm::value_ptr(m_lightingUniforms.colors[0])) ||
            changed;
  changed =
      ImGui::DragDirection("Direction #0", m_lightingUniforms.directions[0]) ||
      changed;
  changed = ImGui::ColorEdit3("Color #1",
                              glm::value_ptr(m_lightingUniforms.colors[1])) ||
            changed;
  changed =
      ImGui::DragDirection("Direction #1", m_lightingUniforms.directions[1]) ||
      changed;
  changed = ImGui::SliderFloat("Hardness", &m_lightingUniforms.hardness, 0.01f,
                               128.0f) ||
            changed;
  changed =
      ImGui::SliderFloat("K Diffuse", &m_lightingUniforms.kd, 0.0f, 2.0f) ||
      changed;
  changed =
      ImGui::SliderFloat("K Specular", &m_lightingUniforms.ks, 0.0f, 2.0f) ||
      changed;
  ImGui::End();
  m_lightingUniformsChanged = changed;

  for (auto layer : m_LayerStack) {
    layer->OnUIRender();
  }
  ImGui::Render();
  ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

// NOTE: Set Application instance as static instance, not global variable
// here this = *Application
Application::Application() { s_Instance = this; };

// NOTE: Singleton design pattern
// so other code can access it with static method Application::Get()
Application *Application::Get() { return s_Instance; };

wgpu::CommandBuffer
Application::RunSingleCommand(std::function<void()> &&prepareFunc) {

  CommandEncoderDescriptor commandEncoderDesc{};
  commandEncoderDesc.label = "Command Encoder";
  CommandEncoder encoder =
      s_Instance->GetDevice().createCommandEncoder(commandEncoderDesc);

  prepareFunc();

  CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
  return command;
}

wgpu::CommandBuffer Application::RunSingleCommand(
    std::function<void(RenderPassEncoder renderPass)> &&renderFunc) {

  CommandEncoderDescriptor commandEncoderDesc{};
  commandEncoderDesc.label = "Command Encoder";
  CommandEncoder encoder =
      s_Instance->GetDevice().createCommandEncoder(commandEncoderDesc);

  RenderPassDescriptor renderPassDesc{};

  RenderPassColorAttachment colorAttachment;
  colorAttachment.view = s_Instance->Get()->m_currentTextureView;
  colorAttachment.resolveTarget = nullptr;
  colorAttachment.loadOp = LoadOp::Clear;
  colorAttachment.storeOp = StoreOp::Store;
  colorAttachment.clearValue = Color{0.05, 0.05, 0.05, 1.0};
  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &colorAttachment;

  RenderPassDepthStencilAttachment depthStencilAttachment;
  depthStencilAttachment.view = s_Instance->m_depthTextureView;
  depthStencilAttachment.depthClearValue = 100.0f;
  depthStencilAttachment.depthLoadOp = LoadOp::Clear;
  depthStencilAttachment.depthStoreOp = StoreOp::Store;
  depthStencilAttachment.depthReadOnly = false;
  depthStencilAttachment.stencilClearValue = 0;
  depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
  depthStencilAttachment.stencilStoreOp = StoreOp::Store;
  depthStencilAttachment.stencilReadOnly = true;

  renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
  renderPassDesc.timestampWriteCount = 0;
  renderPassDesc.timestampWrites = nullptr;

  RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
  renderFunc(renderPass);

  CommandBuffer command = encoder.finish(CommandBufferDescriptor{});
  return command;
};

void Application::initComputeBindGroup() {
  // Create compute bind group
  std::vector<BindGroupEntry> entries(2, Default);

  // Input buffer
  entries[0].binding = 0;
  entries[0].buffer = m_inputBuffer;
  entries[0].offset = 0;
  entries[0].size = m_bufferSize;

  // Output buffer
  entries[1].binding = 1;
  entries[1].buffer = m_outputBuffer;
  entries[1].offset = 0;
  entries[1].size = m_bufferSize;

  BindGroupDescriptor bindGroupDesc;
  bindGroupDesc.layout = m_bindGroupLayout;
  bindGroupDesc.entryCount = (uint32_t)entries.size();
  bindGroupDesc.entries = (WGPUBindGroupEntry *)entries.data();
  m_bindGroup = m_device.createBindGroup(bindGroupDesc);
}

void Application::terminateBindGroup() {
  /* wgpuBindGroupRelease(m_bindGroup);  */
}

void Application::initComputeBindGroupLayout() {
  // Create bind group layout
  std::vector<BindGroupLayoutEntry> bindings(2, Default);

  // Input buffer
  bindings[0].binding = 0;
  bindings[0].buffer.type = BufferBindingType::ReadOnlyStorage;
  bindings[0].visibility = ShaderStage::Compute;

  // Output buffer
  bindings[1].binding = 1;
  bindings[1].buffer.type = BufferBindingType::Storage;
  bindings[1].visibility = ShaderStage::Compute;

  BindGroupLayoutDescriptor bindGroupLayoutDesc;
  bindGroupLayoutDesc.entryCount = (uint32_t)bindings.size();
  bindGroupLayoutDesc.entries = bindings.data();
  m_bindGroupLayout = m_device.createBindGroupLayout(bindGroupLayoutDesc);
}

void Application::terminateBindGroupLayout() {
  /* wgpuBindGroupLayoutRelease(m_bindGroupLayout); */
}

void Application::initComputePipeline() {
  // Load compute shader
  ShaderModule computeShaderModule = ResourceManager::loadShaderModule(
      RESOURCE_DIR "/compute_shader.wsl", m_device);

  // Create compute pipeline layout
  PipelineLayoutDescriptor pipelineLayoutDesc;
  pipelineLayoutDesc.bindGroupLayoutCount = 1;
  pipelineLayoutDesc.bindGroupLayouts =
      (WGPUBindGroupLayout *)&m_bindGroupLayout;
  m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutDesc);

  // Create compute pipeline
  ComputePipelineDescriptor computePipelineDesc;
  computePipelineDesc.compute.constantCount = 0;
  computePipelineDesc.compute.constants = nullptr;
  computePipelineDesc.compute.entryPoint = "computeStuff";
  computePipelineDesc.compute.module = computeShaderModule;
  computePipelineDesc.layout = m_pipelineLayout;
  m_computePipeline = m_device.createComputePipeline(computePipelineDesc);
}

void Application::terminateComputePipeline() {
  /* wgpuComputePipelineRelease(m_pipeline); */
  /* wgpuPipelineLayoutRelease(m_pipelineLayout); */
}

void Application::initComputeBuffers() {
  // Create input/output buffers
  BufferDescriptor bufferDesc;
  bufferDesc.mappedAtCreation = false;
  bufferDesc.size = m_bufferSize;

  bufferDesc.usage = BufferUsage::Storage | BufferUsage::CopyDst;
  m_inputBuffer = m_device.createBuffer(bufferDesc);

  bufferDesc.usage = BufferUsage::Storage | BufferUsage::CopySrc;
  m_outputBuffer = m_device.createBuffer(bufferDesc);

  // Create an intermediary buffer to which we copy the output and that can be
  // used for reading into the CPU memory.
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::MapRead;
  m_mapBuffer = m_device.createBuffer(bufferDesc);
}

void Application::terminateBuffers() {
  m_inputBuffer.destroy();
  /* wgpuBufferRelease(m_inputBuffer); */

  m_outputBuffer.destroy();
  /* wgpuBufferRelease(m_outputBuffer); */

  m_mapBuffer.destroy();
  /* wgpuBufferRelease(m_mapBuffer); */
}

// NOTE: Personal implmentation works, but so hard
void Application::buildComputePipeline() {
  BufferDescriptor storgeBufferDesc{};
  storgeBufferDesc.size = 1000;
  storgeBufferDesc.usage = WGPUBufferUsage::WGPUBufferUsage_Storage |
                           WGPUBufferUsage::WGPUBufferUsage_CopyDst |
                           WGPUBufferUsage::WGPUBufferUsage_CopySrc;
  auto storageBuffer = m_device.createBuffer(storgeBufferDesc);

  BufferDescriptor stagingBufferDesc{};
  stagingBufferDesc.size = 1000;
  stagingBufferDesc.usage = WGPUBufferUsage::WGPUBufferUsage_Storage |
                            WGPUBufferUsage::WGPUBufferUsage_CopyDst;
  auto stagingBuffer = m_device.createBuffer(stagingBufferDesc);

  // Fill in input buffer
  std::vector<uint32_t> input(1000 / sizeof(uint32_t));
  for (int i = 0; i < input.size(); ++i) {
    input[i] = 0.1f * i;
  }
  s_Instance->GetQueue().writeBuffer(storageBuffer, 0, input.data(), 1000);

  // [GPU] shader
  std::cout << "Creating compute shader module..." << std::endl;
  ShaderModule computerShaderModule = ResourceManager::loadShaderModule(
      RESOURCE_DIR "/compute_shader.wsl", m_device);
  std::cout << "Shader module: " << computerShaderModule << std::endl;

  std::cout << "Creating compute pipeline..." << std::endl;
  ComputePipelineDescriptor computePipelineDes{};
  computePipelineDes.compute.module = computerShaderModule;
  computePipelineDes.compute.entryPoint = "main";
  computePipelineDes.label = "compute pipeline 1";
  // ---------------------------------------------------------//
  // [GPU] Create binding layout
  m_computeBindingLayoutEntries.resize(2, Default);
  std::cout << "computed Binding Layout Entries: "
            << m_computeBindingLayoutEntries.size() << "\n";
  m_computeBindingLayoutEntries[0].binding = 0;
  m_computeBindingLayoutEntries[0].visibility = ShaderStage::Compute;
  m_computeBindingLayoutEntries[0].buffer.type = BufferBindingType::Storage;
  m_computeBindingLayoutEntries[0].buffer.minBindingSize = sizeof(uint32_t);
  m_computeBindingLayoutEntries[1].binding = 1;
  m_computeBindingLayoutEntries[1].visibility = ShaderStage::Compute;
  m_computeBindingLayoutEntries[1].buffer.type = BufferBindingType::Storage;
  m_computeBindingLayoutEntries[1].buffer.minBindingSize = sizeof(uint32_t);

  // [GPU] Create bindgroup entries
  m_computeBindingEntries.resize(2);
  std::cout << "computed Binding Entries: " << m_computeBindingEntries.size()
            << "\n";
  /* var<storage, read_write> v_indices: array<u32>; */
  m_computeBindingEntries[0].binding = 0;
  m_computeBindingEntries[0].buffer = storageBuffer;
  m_computeBindingEntries[0].offset = 0;
  m_computeBindingEntries[0].size = sizeof(uint32_t); // BUG: u32 is 4 bytes
  m_computeBindingEntries[1].binding = 1;
  m_computeBindingEntries[1].buffer = stagingBuffer;
  m_computeBindingEntries[1].offset = 0;
  m_computeBindingEntries[1].size = sizeof(uint32_t); // BUG: u32 is 4 bytes
  // ---------------------------------------------------------//
  // [GPU] Create a compute bind group layout
  BindGroupLayoutDescriptor computeBindGroupLayoutDesc{};
  computeBindGroupLayoutDesc.entryCount =
      (uint32_t)m_computeBindingLayoutEntries.size();
  computeBindGroupLayoutDesc.entries = m_computeBindingLayoutEntries.data();
  BindGroupLayout computeBindGroupLayout =
      m_device.createBindGroupLayout(computeBindGroupLayoutDesc);
  // ---------------------------------------------------------//
  // [GPU] Create the pipeline layout
  PipelineLayoutDescriptor computeLayoutDesc{};
  computeLayoutDesc.bindGroupLayoutCount = 1;
  computeLayoutDesc.bindGroupLayouts =
      (WGPUBindGroupLayout *)&computeBindGroupLayout;
  PipelineLayout computePipelineLayout =
      m_device.createPipelineLayout(computeLayoutDesc);
  computePipelineDes.layout = computePipelineLayout;

  // ---------------------------------------------------------//
  // [GPU] Create the pipeline
  m_computePipeline = m_device.createComputePipeline(computePipelineDes);
  std::cout << "Compute pipeline: " << m_computePipeline << "\n";

  // ---------------------------------------------------------//
  // [GPU] Create bindgroup
  BindGroupDescriptor bindGroupDesc{};
  bindGroupDesc.layout = computeBindGroupLayout;
  bindGroupDesc.entries = m_computeBindingEntries.data();
  bindGroupDesc.entryCount = (uint32_t)m_computeBindingEntries.size();
  m_computeBindGroup = m_device.createBindGroup(bindGroupDesc);
  std::cout << "Compute group: " << m_computeBindGroup << "\n";
}
