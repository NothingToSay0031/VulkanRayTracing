
#include "graphics/renderer.hpp"

#include "graphics/command_buffer.hpp"

#include "foundation/memory.hpp"
#include "foundation/file.hpp"

#include "external/imgui/imgui.h"
#include "external/vk_mem_alloc.h"

#include <mutex>

namespace hydra {

std::mutex texture_update_mutex;

// GpuTechniqueCreation ///////////////////////////////////////////////////
GpuTechniqueCreation& GpuTechniqueCreation::reset() {
  num_creations = 0;
  name = nullptr;
  return *this;
}

GpuTechniqueCreation& GpuTechniqueCreation::add_pipeline(const PipelineCreation& pipeline) {
  RASSERT(num_creations < 16);
  creations[num_creations++] = pipeline;
  return *this;
}

GpuTechniqueCreation& GpuTechniqueCreation::set_name(cstring name_) {
  name = name_;
  return *this;
}

// MaterialCreation ///////////////////////////////////////////////////////
MaterialCreation& MaterialCreation::reset() {
  technique = nullptr;
  name = nullptr;
  render_index = ~0u;
  return *this;
}

MaterialCreation& MaterialCreation::set_technique(GpuTechnique* technique_) {
  technique = technique_;
  return *this;
}

MaterialCreation& MaterialCreation::set_render_index(u32 render_index_) {
  render_index = render_index_;
  return *this;
}

MaterialCreation& MaterialCreation::set_name(cstring name_) {
  name = name_;
  return *this;
}

// Renderer /////////////////////////////////////////////////////////////////////

u64 TextureResource::k_type_hash = 0;
u64 BufferResource::k_type_hash = 0;
u64 SamplerResource::k_type_hash = 0;
u64 Material::k_type_hash = 0;
u64 GpuTechnique::k_type_hash = 0;

static Renderer s_renderer;

Renderer* Renderer::instance() { return &s_renderer; }

void Renderer::init(const RendererCreation& creation) {
  rprint("Renderer init\n");

  gpu = creation.gpu;
  resident_allocator = creation.allocator;
  temporary_allocator.init(rkilo(8));

  width = gpu->swapchain_width;
  height = gpu->swapchain_height;

  const RendererResourcePoolCreation& pool_creation = creation.resource_pool_creation;
  textures.init(creation.allocator, pool_creation.textures);
  buffers.init(creation.allocator, pool_creation.buffers);
  samplers.init(creation.allocator, pool_creation.samplers);
  materials.init(creation.allocator, pool_creation.materials);
  techniques.init(creation.allocator, pool_creation.techniques);

  resource_cache.init(creation.allocator);

  // Init resource hashes
  TextureResource::k_type_hash = hash_calculate(TextureResource::k_type);
  BufferResource::k_type_hash = hash_calculate(BufferResource::k_type);
  SamplerResource::k_type_hash = hash_calculate(SamplerResource::k_type);
  Material::k_type_hash = hash_calculate(Material::k_type);
  GpuTechnique::k_type_hash = hash_calculate(GpuTechnique::k_type);

  const u32 gpu_heap_counts = gpu->get_memory_heap_count();
  gpu_heap_budgets.init(resident_allocator, gpu_heap_counts, gpu_heap_counts);
}

void Renderer::shutdown() {
  temporary_allocator.shutdown();

  resource_cache.shutdown(this);
  gpu_heap_budgets.shutdown();

  textures.shutdown();
  buffers.shutdown();
  samplers.shutdown();
  materials.shutdown();
  techniques.shutdown();

  rprint("Renderer shutdown\n");

  gpu->shutdown();
}

void Renderer::set_loaders(hydra::ResourceManager* manager) {}

static void pool_imgui_draw(const ResourcePool& resource_pool, cstring resource_name) {
  ImGui::Text("Pool %s, indices used %u, allocated %u", resource_name, resource_pool.used_indices,
              resource_pool.pool_size);
}

void Renderer::imgui_draw() {
  ImGui::Text("GPU used: %s", gpu->get_gpu_name());
  // Print memory stats
  vmaGetHeapBudgets(gpu->vma_allocator, gpu_heap_budgets.data);

  sizet memory_used = 0;
  sizet memory_allocated = 0;
  for (u32 i = 0; i < gpu->get_memory_heap_count(); ++i) {
    memory_used += gpu_heap_budgets[i].usage;
    memory_allocated += gpu_heap_budgets[i].budget;
  }

  ImGui::Text("GPU Memory Used: %lluMB, Total: %lluMB", memory_used / (1024 * 1024),
              memory_allocated / (1024 * 1024));

  // Resorce pools
  ImGui::Separator();
  pool_imgui_draw(gpu->buffers, "Buffers");
  pool_imgui_draw(gpu->textures, "Textures");
  pool_imgui_draw(gpu->pipelines, "Pipelines");
  pool_imgui_draw(gpu->samplers, "Samplers");
  pool_imgui_draw(gpu->descriptor_sets, "DescriptorSets");
  pool_imgui_draw(gpu->descriptor_set_layouts, "DescriptorSetLayouts");
  pool_imgui_draw(gpu->framebuffers, "Framebuffers");
  pool_imgui_draw(gpu->render_passes, "RenderPasses");
  pool_imgui_draw(gpu->shaders, "Shaders");
}

void Renderer::set_presentation_mode(PresentMode::Enum value) {
  gpu->set_present_mode(value);
  gpu->resize_swapchain();
}

void Renderer::resize_swapchain(u32 width_, u32 height_) {
  gpu->resize((u16)width_, (u16)height_);

  width = gpu->swapchain_width;
  height = gpu->swapchain_height;
}

f32 Renderer::aspect_ratio() const { return gpu->swapchain_width * 1.f / gpu->swapchain_height; }

BufferResource* Renderer::create_buffer(const BufferCreation& creation) {
  BufferResource* buffer = buffers.obtain();
  if (buffer) {
    BufferHandle handle = gpu->create_buffer(creation);
    buffer->handle = handle;
    buffer->name = creation.name;
    gpu->query_buffer(handle, buffer->desc);

    if (creation.name != nullptr) {
      resource_cache.buffers.insert(hash_calculate(creation.name), buffer);
    }

    buffer->references = 1;

    return buffer;
  }
  return nullptr;
}

BufferResource* Renderer::create_buffer(VkBufferUsageFlags type, ResourceUsageType::Enum usage,
                                        u32 size, void* data, cstring name) {
  BufferCreation creation{type, usage, size, 0, 0, data, name};
  return create_buffer(creation);
}

TextureResource* Renderer::create_texture(const TextureCreation& creation) {
  TextureResource* texture = textures.obtain();

  if (texture) {
    TextureHandle handle = gpu->create_texture(creation);
    texture->handle = handle;
    texture->name = creation.name;
    gpu->query_texture(handle, texture->desc);

    if (creation.name != nullptr) {
      resource_cache.textures.insert(hash_calculate(creation.name), texture);
    }

    texture->references = 1;

    return texture;
  }
  return nullptr;
}

SamplerResource* Renderer::create_sampler(const SamplerCreation& creation) {
  SamplerResource* sampler = samplers.obtain();
  if (sampler) {
    SamplerHandle handle = gpu->create_sampler(creation);
    sampler->handle = handle;
    sampler->name = creation.name;
    gpu->query_sampler(handle, sampler->desc);

    if (creation.name != nullptr) {
      resource_cache.samplers.insert(hash_calculate(creation.name), sampler);
    }

    sampler->references = 1;

    return sampler;
  }
  return nullptr;
}

GpuTechnique* Renderer::create_technique(const GpuTechniqueCreation& creation) {
  GpuTechnique* technique = techniques.obtain();
  if (technique) {
    technique->passes.init(resident_allocator, creation.num_creations, creation.num_creations);
    technique->name_hash_to_index.init(resident_allocator, creation.num_creations);
    technique->name_hash_to_index.set_default_value(u16_max);
    technique->name = creation.name;

    temporary_allocator.clear();

    StringBuffer pipeline_cache_path;
    pipeline_cache_path.init(2048, &temporary_allocator);

    for (u32 i = 0; i < creation.num_creations; ++i) {
      GpuTechniquePass& pass = technique->passes[i];
      const PipelineCreation& pass_creation = creation.creations[i];
      if (pass_creation.name != nullptr) {
        char* cache_path = pipeline_cache_path.append_use_f(
            "%s/%s.cache", resource_cache.binary_data_folder, pass_creation.name);

        pass.pipeline = gpu->create_pipeline(pass_creation, cache_path);
      } else {
        pass.pipeline = gpu->create_pipeline(pass_creation);
      }

      pass.name_hash_to_descriptor_index.init(resident_allocator, 16);
      pass.name_hash_to_descriptor_index.set_default_value(u16_max);

      // Cache names of each pass descriptor
      Pipeline* pipeline = gpu->access_pipeline(pass.pipeline);

      for (u32 i = 0; i < pipeline->num_active_layouts; ++i) {
        const DescriptorSetLayout* descriptor_set_layout = pipeline->descriptor_set_layout[i];
        // First global layout is null
        if (descriptor_set_layout == nullptr) {
          continue;
        }

        for (u32 b = 0; b < descriptor_set_layout->num_bindings; ++b) {
          const DescriptorBinding& binding = descriptor_set_layout->bindings[b];

          pass.name_hash_to_descriptor_index.insert(hash_calculate(binding.name),
                                                    (u16)binding.index);
        }
      }

      RASSERT(pass_creation.name);
      technique->name_hash_to_index.insert(hash_calculate(pass_creation.name), (u32)i);
    }

    temporary_allocator.clear();

    if (creation.name != nullptr) {
      resource_cache.techniques.insert(hash_calculate(creation.name), technique);
    }

    technique->references = 1;
  }
  return technique;
}

Material* Renderer::create_material(const MaterialCreation& creation) {
  Material* material = materials.obtain();
  if (material) {
    material->technique = creation.technique;
    material->name = creation.name;
    material->render_index = creation.render_index;

    if (creation.name != nullptr) {
      resource_cache.materials.insert(hash_calculate(creation.name), material);
    }

    material->references = 1;

    return material;
  }
  return nullptr;
}

Material* Renderer::create_material(GpuTechnique* technique, cstring name) {
  MaterialCreation creation{technique, name};
  return create_material(creation);
}

PipelineHandle Renderer::get_pipeline(Material* material, u32 pass_index) {
  RASSERT(material != nullptr);

  return material->technique->passes[pass_index].pipeline;
}

DescriptorSetHandle Renderer::create_descriptor_set(CommandBuffer* gpu_commands, Material* material,
                                                    DescriptorSetCreation& ds_creation) {
  RASSERT(material != nullptr);

  // TODO:
  DescriptorSetLayoutHandle set_layout =
      gpu->get_descriptor_set_layout(material->technique->passes[0].pipeline, 1);

  ds_creation.set_layout(set_layout);

  return gpu_commands->create_descriptor_set(ds_creation);
}

void Renderer::destroy_buffer(BufferResource* buffer) {
  if (!buffer) {
    return;
  }

  buffer->remove_reference();
  if (buffer->references) {
    return;
  }

  if (buffer->desc.name) {
    resource_cache.buffers.remove(hash_calculate(buffer->desc.name));
  }

  gpu->destroy_buffer(buffer->handle);
  buffers.release(buffer);
}

void Renderer::destroy_texture(TextureResource* texture) {
  if (!texture) {
    return;
  }

  texture->remove_reference();
  if (texture->references) {
    return;
  }

  if (texture->desc.name) {
    resource_cache.textures.remove(hash_calculate(texture->desc.name));
  }

  gpu->destroy_texture(texture->handle);
  textures.release(texture);
}

void Renderer::destroy_sampler(SamplerResource* sampler) {
  if (!sampler) {
    return;
  }

  sampler->remove_reference();
  if (sampler->references) {
    return;
  }

  if (sampler->desc.name) {
    resource_cache.samplers.remove(hash_calculate(sampler->desc.name));
  }

  gpu->destroy_sampler(sampler->handle);
  samplers.release(sampler);
}

void Renderer::destroy_material(Material* material) {
  if (!material) {
    return;
  }

  material->remove_reference();
  if (material->references) {
    return;
  }

  resource_cache.materials.remove(hash_calculate(material->name));
  materials.release(material);
}

void Renderer::destroy_technique(GpuTechnique* technique) {
  if (!technique) {
    return;
  }

  technique->remove_reference();
  if (technique->references) {
    return;
  }

  for (u32 i = 0; i < technique->passes.size; ++i) {
    gpu->destroy_pipeline(technique->passes[i].pipeline);
    technique->passes[i].name_hash_to_descriptor_index.shutdown();
  }

  technique->passes.shutdown();
  technique->name_hash_to_index.shutdown();

  resource_cache.techniques.remove(hash_calculate(technique->name));
  techniques.release(technique);
}

void* Renderer::map_buffer(BufferResource* buffer, u32 offset, u32 size) {
  MapBufferParameters cb_map = {buffer->handle, offset, size};
  return gpu->map_buffer(cb_map);
}

void Renderer::unmap_buffer(BufferResource* buffer) {
  if (buffer->desc.parent_handle.index == k_invalid_index) {
    MapBufferParameters cb_map = {buffer->handle, 0, 0};
    gpu->unmap_buffer(cb_map);
  }
}

void Renderer::add_texture_to_update(hydra::TextureHandle texture) {
  std::lock_guard<std::mutex> guard(texture_update_mutex);

  textures_to_update[num_textures_to_update++] = texture;
}

// TODO:
static void generate_mipmaps(hydra::Texture* texture, hydra::CommandBuffer* cb,
                             bool from_transfer_queue) {
  using namespace hydra;

  if (texture->mip_level_count > 1) {
    util_add_image_barrier(
        cb->gpu_device, cb->vk_command_buffer, texture->vk_image,
        from_transfer_queue ? RESOURCE_STATE_COPY_SOURCE : RESOURCE_STATE_COPY_SOURCE,
        RESOURCE_STATE_COPY_SOURCE, 0, 1, false);
  }

  i32 w = texture->width;
  i32 h = texture->height;

  for (int mip_index = 1; mip_index < texture->mip_level_count; ++mip_index) {
    util_add_image_barrier(cb->gpu_device, cb->vk_command_buffer, texture->vk_image,
                           RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_COPY_DEST, mip_index, 1, false);

    VkImageBlit blit_region{};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = mip_index - 1;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;

    blit_region.srcOffsets[0] = {0, 0, 0};
    blit_region.srcOffsets[1] = {w, h, 1};

    w /= 2;
    h /= 2;

    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = mip_index;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;

    blit_region.dstOffsets[0] = {0, 0, 0};
    blit_region.dstOffsets[1] = {w, h, 1};

    vkCmdBlitImage(cb->vk_command_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region,
                   VK_FILTER_LINEAR);

    // Prepare current mip for next level
    util_add_image_barrier(cb->gpu_device, cb->vk_command_buffer, texture->vk_image,
                           RESOURCE_STATE_COPY_DEST, RESOURCE_STATE_COPY_SOURCE, mip_index, 1,
                           false);
  }

  // Transition
  if (from_transfer_queue) {
    util_add_image_barrier(
        cb->gpu_device, cb->vk_command_buffer, texture->vk_image,
        (texture->mip_level_count > 1) ? RESOURCE_STATE_COPY_SOURCE : RESOURCE_STATE_COPY_DEST,
        RESOURCE_STATE_SHADER_RESOURCE, 0, texture->mip_level_count, false);
  } else {
    util_add_image_barrier(cb->gpu_device, cb->vk_command_buffer, texture->vk_image,
                           RESOURCE_STATE_UNDEFINED, RESOURCE_STATE_SHADER_RESOURCE, 0,
                           texture->mip_level_count, false);
  }
}

void Renderer::add_texture_update_commands(u32 thread_id) {
  std::lock_guard<std::mutex> guard(texture_update_mutex);

  if (num_textures_to_update == 0) {
    return;
  }

  CommandBuffer* cb = gpu->get_command_buffer(thread_id, gpu->current_frame, false);
  cb->begin();

  for (u32 i = 0; i < num_textures_to_update; ++i) {
    Texture* texture = gpu->access_texture(textures_to_update[i]);

    util_add_image_barrier_ext(
        cb->gpu_device, cb->vk_command_buffer, texture->vk_image, RESOURCE_STATE_COPY_DEST,
        RESOURCE_STATE_COPY_SOURCE, 0, 1, 0, 1, false, gpu->vulkan_transfer_queue_family,
        gpu->vulkan_main_queue_family, QueueType::CopyTransfer, QueueType::Graphics);

    generate_mipmaps(texture, cb, true);
  }

  // TODO: this is done before submitting to the queue in the device.
  // cb->end();
  gpu->queue_command_buffer(cb);

  num_textures_to_update = 0;
}

// ResourceCache
void ResourceCache::init(Allocator* allocator) {
  // Init resources caching
  textures.init(allocator, 16);
  buffers.init(allocator, 16);
  samplers.init(allocator, 16);
  materials.init(allocator, 16);
  techniques.init(allocator, 16);
}

void ResourceCache::shutdown(Renderer* renderer) {
  hydra::FlatHashMapIterator it = textures.iterator_begin();

  while (it.is_valid()) {
    hydra::TextureResource* texture = textures.get(it);
    renderer->destroy_texture(texture);

    textures.iterator_advance(it);
  }

  it = buffers.iterator_begin();

  while (it.is_valid()) {
    hydra::BufferResource* buffer = buffers.get(it);
    renderer->destroy_buffer(buffer);

    buffers.iterator_advance(it);
  }

  it = samplers.iterator_begin();

  while (it.is_valid()) {
    hydra::SamplerResource* sampler = samplers.get(it);
    renderer->destroy_sampler(sampler);

    samplers.iterator_advance(it);
  }

  it = materials.iterator_begin();

  while (it.is_valid()) {
    hydra::Material* material = materials.get(it);
    renderer->destroy_material(material);

    materials.iterator_advance(it);
  }

  it = techniques.iterator_begin();

  while (it.is_valid()) {
    hydra::GpuTechnique* technique = techniques.get(it);
    renderer->destroy_technique(technique);

    techniques.iterator_advance(it);
  }

  textures.shutdown();
  buffers.shutdown();
  samplers.shutdown();
  materials.shutdown();
  techniques.shutdown();
}

// GpuTechnique ///////////////////////////////////////////////////////////
u32 GpuTechnique::get_pass_index(cstring name) {
  const u64 name_hash = hash_calculate(name);
  return name_hash_to_index.get(name_hash);
}

// GpuTechniquePass ///////////////////////////////////////////////////////
u32 GpuTechniquePass::get_binding_index(cstring name) {
  const u64 name_hash = hash_calculate(name);
  return name_hash_to_descriptor_index.get(name_hash);
}

GpuTechniqueDescriptorCreation& GpuTechniqueDescriptorCreation::reset() {
  descriptor_set_creation.reset();
  pass = nullptr;
  descriptor_set_index = 0;

  return *this;
}

GpuTechniqueDescriptorCreation& GpuTechniqueDescriptorCreation::set_pass(GpuTechniquePass* pass_) {
  pass = pass_;

  return *this;
}

GpuTechniqueDescriptorCreation& GpuTechniqueDescriptorCreation::set_index(u32 index) {
  descriptor_set_index = index;

  return *this;
}

GpuTechniqueDescriptorCreation& GpuTechniqueDescriptorCreation::buffer(BufferHandle buffer,
                                                                       cstring name) {
  const u16 binding_index = pass->get_binding_index(name);
  descriptor_set_creation.buffer(buffer, binding_index);
  return *this;
}

GpuTechniqueDescriptorCreation& GpuTechniqueDescriptorCreation::texture(TextureHandle texture,
                                                                        cstring name) {
  const u16 binding_index = pass->get_binding_index(name);
  descriptor_set_creation.texture(texture, binding_index);

  return *this;
}

DescriptorSetCreation& GpuTechniqueDescriptorCreation::finalize() {
  return descriptor_set_creation;
}

}  // namespace hydra
