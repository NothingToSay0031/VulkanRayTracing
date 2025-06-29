#pragma once

#include "graphics/gpu_device.hpp"
#include "graphics/gpu_resources.hpp"

#include "foundation/resource_manager.hpp"

namespace hydra {

struct Renderer;

//
//
struct BufferResource : public hydra::Resource {
  BufferHandle handle;
  u32 pool_index;
  BufferDescription desc;

  static constexpr cstring k_type = "hydra_buffer_type";
  static u64 k_type_hash;

};  // struct Buffer

//
//
struct TextureResource : public hydra::Resource {
  TextureHandle handle;
  u32 pool_index;
  TextureDescription desc;

  static constexpr cstring k_type = "hydra_texture_type";
  static u64 k_type_hash;

};  // struct Texture

//
//
struct SamplerResource : public hydra::Resource {
  SamplerHandle handle;
  u32 pool_index;
  SamplerDescription desc;

  static constexpr cstring k_type = "hydra_sampler_type";
  static u64 k_type_hash;

};  // struct Sampler

// Material/Shaders ///////////////////////////////////////////////////////

//
//
struct GpuTechniqueCreation {
  PipelineCreation creations[16];
  u32 num_creations = 0;

  cstring name = nullptr;

  GpuTechniqueCreation& reset();
  GpuTechniqueCreation& add_pipeline(const PipelineCreation& pipeline);
  GpuTechniqueCreation& set_name(cstring name);

};  // struct GpuTechniqueCreation

//
//
struct GpuTechniquePass {
  PipelineHandle pipeline;

  FlatHashMap<u64, u16> name_hash_to_descriptor_index;

  u32 get_binding_index(cstring name);

};  // struct GpuTechniquePass

struct GpuTechniqueDescriptorCreation {
  DescriptorSetCreation descriptor_set_creation;
  GpuTechniquePass* pass = nullptr;
  u32 descriptor_set_index = 0;

  GpuTechniqueDescriptorCreation& reset();
  GpuTechniqueDescriptorCreation& set_pass(GpuTechniquePass* pass);
  GpuTechniqueDescriptorCreation& set_index(u32 index);
  GpuTechniqueDescriptorCreation& buffer(BufferHandle buffer, cstring name);
  GpuTechniqueDescriptorCreation& texture(TextureHandle texture, cstring name);

  DescriptorSetCreation& finalize();
};

//
//
struct GpuTechnique : public hydra::Resource {
  Array<GpuTechniquePass> passes;
  FlatHashMap<u64, u16> name_hash_to_index;

  u32 pool_index;

  u32 get_pass_index(cstring name);

  static constexpr cstring k_type = "hydra_gpu_technique_type";
  static u64 k_type_hash;

};  // struct GpuTechnique

//
//
struct MaterialCreation {
  MaterialCreation& reset();
  MaterialCreation& set_technique(GpuTechnique* technique);
  MaterialCreation& set_name(cstring name);
  MaterialCreation& set_render_index(u32 render_index);

  GpuTechnique* technique = nullptr;
  cstring name = nullptr;
  u32 render_index = u32_max;

};  // struct MaterialCreation

//
//
struct Material : public hydra::Resource {
  GpuTechnique* technique;

  u32 render_index;

  u32 pool_index;

  static constexpr cstring k_type = "hydra_material_type";
  static u64 k_type_hash;

};  // struct Material

// ResourceCache //////////////////////////////////////////////////////////

//
//
struct ResourceCache {
  void init(Allocator* allocator);
  void shutdown(Renderer* renderer);

  FlatHashMap<u64, TextureResource*> textures;
  FlatHashMap<u64, BufferResource*> buffers;
  FlatHashMap<u64, SamplerResource*> samplers;
  FlatHashMap<u64, Material*> materials;
  FlatHashMap<u64, GpuTechnique*> techniques;

  char binary_data_folder[512];

};  // struct ResourceCache

// Renderer ///////////////////////////////////////////////////////////////
//
//
struct RendererResourcePoolCreation {
  u16 textures = 256;
  u16 buffers = 256;
  u16 samplers = 64;
  u16 materials = 256;
  u16 techniques = 128;

};  // struct RendererResourcePoolCreation
//
//
struct RendererCreation {
  hydra::GpuDevice* gpu;
  Allocator* allocator;

  RendererResourcePoolCreation resource_pool_creation;

};  // struct RendererCreation

//
// Main class responsible for handling all high level resources
//
struct Renderer : public Service {
  HYDRA_DECLARE_SERVICE(Renderer);

  void init(const RendererCreation& creation);
  void shutdown();

  void set_loaders(hydra::ResourceManager* manager);

  void imgui_draw();

  void set_presentation_mode(PresentMode::Enum value);
  void resize_swapchain(u32 width, u32 height);

  f32 aspect_ratio() const;

  // Creation/destruction
  BufferResource* create_buffer(const BufferCreation& creation);
  BufferResource* create_buffer(VkBufferUsageFlags type, ResourceUsageType::Enum usage, u32 size,
                                void* data, cstring name);

  TextureResource* create_texture(const TextureCreation& creation);

  SamplerResource* create_sampler(const SamplerCreation& creation);

  GpuTechnique* create_technique(const GpuTechniqueCreation& creation);

  Material* create_material(const MaterialCreation& creation);
  Material* create_material(GpuTechnique* technique, cstring name);

  // Draw
  PipelineHandle get_pipeline(Material* material, u32 pass_index);
  DescriptorSetHandle create_descriptor_set(CommandBuffer* gpu_commands, Material* material,
                                            DescriptorSetCreation& ds_creation);

  void destroy_buffer(BufferResource* buffer);
  void destroy_texture(TextureResource* texture);
  void destroy_sampler(SamplerResource* sampler);
  void destroy_material(Material* material);
  void destroy_technique(GpuTechnique* technique);

  // Update resources
  void* map_buffer(BufferResource* buffer, u32 offset = 0, u32 size = 0);
  void unmap_buffer(BufferResource* buffer);

  CommandBuffer* get_command_buffer(u32 thread_index, u32 current_frame_index, bool begin) {
    return gpu->get_command_buffer(thread_index, current_frame_index, begin);
  }
  void queue_command_buffer(hydra::CommandBuffer* commands) {
    gpu->queue_command_buffer(commands);
  }

  // Multithread friendly update to textures
  void add_texture_to_update(hydra::TextureHandle texture);
  void add_texture_update_commands(u32 thread_id);

  ResourcePoolTyped<TextureResource> textures;
  ResourcePoolTyped<BufferResource> buffers;
  ResourcePoolTyped<SamplerResource> samplers;
  ResourcePoolTyped<Material> materials;
  ResourcePoolTyped<GpuTechnique> techniques;

  ResourceCache resource_cache;

  TextureHandle textures_to_update[128];
  u32 num_textures_to_update = 0;

  hydra::GpuDevice* gpu;
  Allocator* resident_allocator;
  StackAllocator temporary_allocator;

  Array<VmaBudget> gpu_heap_budgets;

  u16 width;
  u16 height;

  static constexpr cstring k_name = "hydra_rendering_service";

};  // struct Renderer

}  // namespace hydra
