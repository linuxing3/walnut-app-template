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

#include "ResourceManager.h"

#include "stb_image.h"
#include "tiny_obj_loader.h"
#include <cstring>
#include <memory>

#include <fstream>

using namespace wgpu;

// generate docs about loadShaderModule function
/**
 * @brief Load a shader from a WGSL file into a new shader module
 *
 * @param path the path to the shader file
 * @param device the device to create the shader module on
 * @return wgpu::ShaderModule a shader module
 */
ShaderModule ResourceManager::loadShaderModule(const path &path,
                                               Device device) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  std::string shaderSource(size, ' ');
  file.seekg(0);
  file.read(shaderSource.data(), size);

  ShaderModuleWGSLDescriptor shaderCodeDesc{};
  shaderCodeDesc.chain.next = nullptr;
  shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
  shaderCodeDesc.code = shaderSource.c_str();
  ShaderModuleDescriptor shaderDesc{};
  shaderDesc.hintCount = 0;
  shaderDesc.hints = nullptr;
  shaderDesc.nextInChain = &shaderCodeDesc.chain;
  return device.createShaderModule(shaderDesc);
}
// generate docs for loadGeometryFromObj function
/**
 * @brief Load an 3D mesh from a standard .obj file into a vertex data buffer
 *
 * @param path the path to the .obj file
 * @param vertexData the vertex data buffer
 * @return true if the mesh was loaded successfully
 * @return false if the mesh failed to load
 */
bool ResourceManager::loadGeometryFromObj(
    const path &path, std::vector<VertexAttributes> &vertexData) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;

  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                              path.string().c_str());

  if (!warn.empty()) {
    std::cout << warn << std::endl;
  }

  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  if (!ret) {
    return false;
  }

  // Fill in vertexData here
  vertexData.clear();
  // explain meaning of the following code in for loop
  // 1. offset is the current size of vertexData
  // 2. resize vertexData to the size of offset + the size of shape.mesh.indices
  // 3. for each index in shape.mesh.indices
  //    a. get the vertex_index, normal_index and texcoord_index from the index
  //    b. fill in the position, normal, color and uv of the vertexData[offset +
  //    i]
  for (const auto &shape : shapes) {
    size_t offset = vertexData.size();
    vertexData.resize(offset + shape.mesh.indices.size());

    for (int i = 0; i < vertexData.size(); ++i) {
      const tinyobj::index_t &idx = shape.mesh.indices[i];

      vertexData[offset + i].position = {
          attrib.vertices[3 * idx.vertex_index + 0],
          -attrib.vertices[3 * idx.vertex_index + 2],
          attrib.vertices[3 * idx.vertex_index + 1]};

      vertexData[offset + i].normal = {
          attrib.normals[3 * idx.normal_index + 0],
          -attrib.normals[3 * idx.normal_index + 2],
          attrib.normals[3 * idx.normal_index + 1]};

      vertexData[offset + i].color = {attrib.colors[3 * idx.vertex_index + 0],
                                      attrib.colors[3 * idx.vertex_index + 1],
                                      attrib.colors[3 * idx.vertex_index + 2]};

      // Fix the UV convention
      vertexData[offset + i].uv = {
          attrib.texcoords[2 * idx.texcoord_index + 0],
          1 - attrib.texcoords[2 * idx.texcoord_index + 1]};
    }
  }

  return true;
}

// Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m) {
  if (m == 0)
    return 0;
  else {
    uint32_t w = 0;
    while (m >>= 1)
      ++w;
    return w;
  }
}

// explain the meaning of the following function
/**
 * @brief Load an image from a standard image file into a new texture object
 *
 * @param path the path to the image file
 * @param device the device to create the texture on
 * @param pTextureView a pointer to a texture view to be filled with the view of
 * the texture
 * @return Texture a texture
 */
Texture ResourceManager::loadTexture(const path &path, Device device,
                                     TextureView *pTextureView) {
  int width, height, channels;
  unsigned char *pixelData = stbi_load(path.string().c_str(), &width, &height,
                                       &channels, 4 /* force 4 channels */);
  if (nullptr == pixelData)
    return nullptr;

  TextureDescriptor textureDesc;
  textureDesc.dimension = TextureDimension::_2D;
  textureDesc.format = TextureFormat::RGBA8Unorm;
  textureDesc.size = {(unsigned int)width, (unsigned int)height, 1};
  textureDesc.mipLevelCount =
      bit_width(std::max(textureDesc.size.width, textureDesc.size.height));
  ;
  textureDesc.sampleCount = 1;
  textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;
  Texture texture = device.createTexture(textureDesc);

  // Arguments telling which part of the texture we upload to
  ImageCopyTexture destination;
  destination.texture = texture;
  destination.origin = {0, 0, 0};
  destination.aspect = TextureAspect::All;

  // Arguments telling how the C++ side pixel memory is laid out
  TextureDataLayout source;
  source.offset = 0;

  // Create image data
  Extent3D mipLevelSize = textureDesc.size;
  std::vector<uint8_t> previousLevelPixels;
  Extent3D previousMipLevelSize;

  // explain the meaning of the following loop
  // 1. for each level in the texture
  //    a. create a vector of pixels
  //    b. if the level is 0, copy the pixelData into the vector
  //    c. else
  //       i. for each pixel in the vector
  //          1. get the 4 pixels from the previous level
  //          2. average the 4 pixels and set the pixel in the current level
  //          to the average
  //    d. upload the data to the texture
  //    e. set the previousLevelPixels to the pixels
  //    f. set the previousMipLevelSize to the mipLevelSize
  //    g. divide the mipLevelSize by 2
  for (uint32_t level = 0; level < textureDesc.mipLevelCount; ++level) {
    std::vector<uint8_t> pixels(4 * mipLevelSize.width * mipLevelSize.height);
    if (level == 0) {
      // We cannot really avoid this copy since we need this
      // in previousLevelPixels at the next iteration
      memcpy(pixels.data(), pixelData, pixels.size());
    } else {
      // Create mip level data
      for (uint32_t i = 0; i < mipLevelSize.width; ++i) {
        for (uint32_t j = 0; j < mipLevelSize.height; ++j) {
          uint8_t *p = &pixels[4 * (j * mipLevelSize.width + i)];
          // Get the corresponding 4 pixels from the previous level
          uint8_t *p00 =
              &previousLevelPixels[4 *
                                   ((2 * j + 0) * previousMipLevelSize.width +
                                    (2 * i + 0))];
          uint8_t *p01 =
              &previousLevelPixels[4 *
                                   ((2 * j + 0) * previousMipLevelSize.width +
                                    (2 * i + 1))];
          uint8_t *p10 =
              &previousLevelPixels[4 *
                                   ((2 * j + 1) * previousMipLevelSize.width +
                                    (2 * i + 0))];
          uint8_t *p11 =
              &previousLevelPixels[4 *
                                   ((2 * j + 1) * previousMipLevelSize.width +
                                    (2 * i + 1))];
          // Average
          p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
          p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
          p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
          p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
        }
      }
    }

    // Upload data to the GPU texture
    destination.mipLevel = level;
    source.bytesPerRow = 4 * mipLevelSize.width;
    source.rowsPerImage = mipLevelSize.height;
    device.getQueue().writeTexture(destination, pixels.data(), pixels.size(),
                                   source, mipLevelSize);

    previousLevelPixels = std::move(pixels);
    previousMipLevelSize = mipLevelSize;
    mipLevelSize.width /= 2;
    mipLevelSize.height /= 2;
  }

  stbi_image_free(pixelData);

  if (pTextureView) {
    TextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = TextureAspect::All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
    textureViewDesc.dimension = TextureViewDimension::_2D;
    textureViewDesc.format = textureDesc.format;
    *pTextureView = texture.createView(textureViewDesc);
  }

  return texture;
}


