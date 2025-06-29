#pragma once

#include "graphics/renderer.hpp"

namespace hydra {

struct FrameGraph;

//
//
struct RenderResourcesLoader {
  void init(hydra::Renderer* renderer, hydra::StackAllocator* temp_allocator,
            hydra::FrameGraph* frame_graph);
  void shutdown();

  GpuTechnique* load_gpu_technique(cstring json_path, bool use_shader_cache,
                                   bool& is_shader_changed);
  TextureResource* load_texture(cstring path, bool generate_mipmaps = true);

  void parse_gpu_technique(GpuTechniqueCreation& technique_creation, cstring json_path,
                           bool use_shader_cache, bool& is_techinque_changed);
  void reload_gpu_technique(cstring json_path, bool use_shader_cache, bool& is_techinque_changed);

  Renderer* renderer;
  FrameGraph* frame_graph;
  StackAllocator* temp_allocator;

};  // struct RenderResourcesLoader

}  // namespace hydra
