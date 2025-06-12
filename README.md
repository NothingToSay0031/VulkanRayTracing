# Vulkan Ray Tracing

This repository demonstrates a range of modern real-time ray tracing techniques implemented in Vulkan, including dynamic global illumination, ray-traced shadows, reflections, and advanced filtering methods.

## Dynamic Diffuse Global Illumination (DDGI)

**Implements real-time dynamic diffuse GI using probe-based irradiance fields.**
Based on [Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields](https://www.jcgt.org/published/0008/02/01/paper-lowres.pdf).

### Pipeline Overview

1. **Probe Ray Tracing:** For each probe, perform ray tracing to calculate radiance and hit distance.
2. **Irradiance Update:** Update probe irradiance using the traced radiance values with temporal hysteresis for stability.
3. **Visibility Update:** Update probe visibility data using traced distances, also applying hysteresis.
4. **(Optional) Probe Offset Update:** Adjust per-probe offset positions based on ray tracing distances.
5. **Indirect Lighting Calculation:** Compute indirect lighting for the scene by sampling the updated irradiance, visibility, and probe offsets.

## Ray-Traced Shadows

**Efficient real-time ray-traced shadow technique with spatiotemporal filtering.**
Based on [Ray Traced Shadows: Maintaining Real-Time Frame Rates](https://www.cg.tuwien.ac.at/research/publications/2019/BOKSANSKY-2019-RTS/).

### Pipeline Overview

1. **Visibility Variance History:** Compute and store visibility variance across the past four frames.
2. **Sample Count Estimation:** Estimate per-fragment and per-light sample counts using a max filter followed by a tent filter.
3. **Ray Tracing:** Trace shadow rays into the scene using the sample counts to obtain raw visibility values.
4. **Denoising:** Apply temporal and spatial filtering to the raw visibility to reduce noise.
5. **Lighting Integration:** Use the filtered shadow visibility in the final lighting calculation.

## Ray-Traced Reflections & SVGF

**Real-time ray-traced reflections with spatiotemporal variance-guided denoising.**
Based on [Spatiotemporal Variance-Guided Filtering: Real-Time Reconstruction for Path-Traced Global Illumination](https://research.nvidia.com/publication/2017-07_spatiotemporal-variance-guided-filtering-real-time-reconstruction-path-traced).

### Algorithm Steps

1. **G-buffer Check:** For each fragment, check if the surface roughness is below a specified threshold to determine if reflection should be computed.
2. **Ray Casting:** Cast one reflection ray per fragment. Two methods are implemented:

   * Mirror reflection direction
   * GGX distribution sampling
3. **Secondary Lighting:** If the reflection ray hits geometry, shoot a secondary ray toward an importance-sampled light source. If the light is visible, compute the color using the lighting model.
4. **Denoising:** As only one sample per fragment is used (resulting in noisy output), process the raw reflections using **spatiotemporal variance-guided filtering (SVGF)** to significantly reduce noise by leveraging spatial and temporal information.
5. **Lighting Integration:** Integrate the denoised reflection data into the scene's specular lighting calculation.


## Temporal Anti-Aliasing (TAA)

**Robust temporal anti-aliasing for stable, artifact-free images.**

### Algorithm Steps

1. **Velocity Coordinate Calculation:** Compute pixel motion vectors (velocity) by searching a 3x3 neighborhood using the current frame's depth texture, improving edge quality and reducing ghosting.
2. **Velocity Fetch:** Fetch velocity from the velocity texture using linear sampling for sub-pixel accuracy.
3. **History Color Fetch:** Sample color information from the previous frame's TAA result (history texture), with optional filtering for improved quality.
4. **Neighborhood Cache:** Sample the current frame’s color in a neighborhood to help constrain the fetched history color during resolve.
5. **History Constraint:** Limit the previous frame’s color within a confidence range around the current color to reject occluded or disoccluded samples, reducing ghosting.
6. **Final Resolve:** Combine the current color and constrained history color, applying additional filtering to generate the final anti-aliased pixel output.

# Usage

1. Install CMake
2. `python3 .\bootstrap.py`
3. Install Vulkan SDK
4. `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
