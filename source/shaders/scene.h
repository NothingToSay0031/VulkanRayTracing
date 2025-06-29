
#ifndef HYDRA_GLSL_SCENE_H
#define HYDRA_GLSL_SCENE_H

// Scene common code /////////////////////////////////////////////////////
layout(std140, set = MATERIAL_SET, binding = 0) uniform SceneConstants {
  mat4 view_projection;
  mat4 view_projection_debug;
  mat4 inverse_view_projection;
  mat4 world_to_camera;
  mat4 world_to_camera_debug;
  mat4 previous_view_projection;
  mat4 inverse_projection;
  mat4 inverse_view;

  vec4 camera_position;
  vec4 camera_position_debug;
  vec3 camera_direction;
  int current_frame;

  uint active_lights;
  uint use_tetrahedron_shadows;
  uint dither_texture_index;
  float z_near;

  float z_far;
  float projection_00;
  float projection_11;
  uint culling_options;

  vec2 resolution;
  float aspect_ratio;
  uint num_mesh_instances;

  vec2 halton_xy;
  uint depth_texture_index;
  uint blue_noise_128_rg_texture_index;

  vec2 jitter_xy;
  vec2 previous_jitter_xy;

  float forced_metalness;
  float forced_roughness;
  float volumetric_fog_application_dithering_scale;
  uint volumetric_fog_application_options;

  vec4 frustum_planes[6];
};

bool enable_volumetric_fog_opacity_anti_aliasing() {
  return (volumetric_fog_application_options & 1) == 1;
}

bool enable_volumetric_fog_opacity_tricubic_filtering() {
  return (volumetric_fog_application_options & 2) == 2;
}

// Options ///////////////////////////////////////////////////////////////
bool disable_frustum_cull_meshes() { return (culling_options & 1) != 1; }

bool disable_frustum_cull_meshlets() { return (culling_options & 2) != 2; }

bool disable_occlusion_cull_meshes() { return (culling_options & 4) != 4; }

bool disable_occlusion_cull_meshlets() { return (culling_options & 8) != 8; }

bool freeze_occlusion_camera() { return (culling_options & 16) == 16; }

bool disable_shadow_meshlets_cone_cull() { return (culling_options & 32) != 32; }

bool disable_shadow_meshlets_sphere_cull() { return (culling_options & 64) != 64; }

bool disable_shadow_meshlets_cubemap_face_cull() { return (culling_options & 128) != 128; }

bool disable_shadow_meshes_sphere_cull() { return (culling_options & 256) != 256; }

// Utility methods ///////////////////////////////////////////////////////
float dither(vec2 screen_pixel_position, float value) {
  float dither_value =
      texelFetch(global_textures[nonuniformEXT(dither_texture_index)],
                 ivec2(int(screen_pixel_position.x) % 4, int(screen_pixel_position.y) % 4), 0)
          .r;
  return value - dither_value;
}

// Convert raw_depth (0..1) to linear depth (near...far)
float linearize_raw_depth(float raw_depth) {
  // NOTE(marco): Vulkan depth is [0, 1]
  return z_near * z_far / (z_far + raw_depth * (z_near - z_far));
}

#endif  // HYDRA_GLSL_SCENE_H
