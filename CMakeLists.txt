cmake_minimum_required(VERSION 3.10.0)

project(VulkanRayTracing VERSION 0.1.0)

find_package(Vulkan REQUIRED)

if (UNIX)
find_package(SDL2 REQUIRED)
endif()

add_library(HydraFoundation STATIC
    source/hydra/foundation/array.hpp
    source/hydra/foundation/assert.cpp
    source/hydra/foundation/assert.hpp
    source/hydra/foundation/bit.cpp
    source/hydra/foundation/bit.hpp
    source/hydra/foundation/blob_serialization.cpp
    source/hydra/foundation/blob_serialization.hpp
    source/hydra/foundation/blob.hpp
    source/hydra/foundation/camera.cpp
    source/hydra/foundation/camera.hpp
    source/hydra/foundation/color.cpp
    source/hydra/foundation/color.hpp
    source/hydra/foundation/data_structures.cpp
    source/hydra/foundation/data_structures.hpp
    source/hydra/foundation/file.cpp
    source/hydra/foundation/file.hpp
    source/hydra/foundation/gltf.cpp
    source/hydra/foundation/gltf.hpp
    source/hydra/foundation/hash_map.hpp
    source/hydra/foundation/log.cpp
    source/hydra/foundation/log.hpp
    source/hydra/foundation/memory_utils.hpp
    source/hydra/foundation/memory.cpp
    source/hydra/foundation/memory.hpp
    source/hydra/foundation/numerics.cpp
    source/hydra/foundation/numerics.hpp
    source/hydra/foundation/platform.hpp
    source/hydra/foundation/process.cpp
    source/hydra/foundation/process.hpp
    source/hydra/foundation/relative_data_structures.hpp
    source/hydra/foundation/resource_manager.cpp
    source/hydra/foundation/resource_manager.hpp
    source/hydra/foundation/serialization.cpp
    source/hydra/foundation/serialization.hpp
    source/hydra/foundation/service_manager.cpp
    source/hydra/foundation/service_manager.hpp
    source/hydra/foundation/service.cpp
    source/hydra/foundation/service.hpp
    source/hydra/foundation/string.cpp
    source/hydra/foundation/string.hpp
    source/hydra/foundation/time.cpp
    source/hydra/foundation/time.hpp
)

set_property(TARGET HydraFoundation PROPERTY CXX_STANDARD 17)

target_compile_definitions(HydraFoundation PRIVATE
    _CRT_SECURE_NO_WARNINGS

    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(HydraFoundation PRIVATE
    source
    source/hydra
)

add_library(HydraApp STATIC
    source/hydra/application/application.cpp
    source/hydra/application/application.hpp
    source/hydra/application/game_camera.cpp
    source/hydra/application/game_camera.hpp
    source/hydra/application/game_application.cpp
    source/hydra/application/game_application.hpp
    source/hydra/application/input.cpp
    source/hydra/application/input.hpp
    source/hydra/application/keys.cpp
    source/hydra/application/keys.hpp
    source/hydra/application/window.cpp
    source/hydra/application/window.hpp
)

set_property(TARGET HydraApp PROPERTY CXX_STANDARD 17)

target_compile_definitions(HydraApp PRIVATE
    _CRT_SECURE_NO_WARNINGS

    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

target_include_directories(HydraApp PRIVATE
    source
    source/hydra
    )

if (WIN32)
    target_include_directories(HydraApp PRIVATE
        binaries/SDL2-2.0.18/include)
else()
    target_include_directories(HydraApp PRIVATE
        ${SDL2_INCLUDE_DIRS})
endif()

add_library(HydraExternal STATIC
    source/external/tlsf.c

    source/external/json.hpp

    source/external/imgui/imconfig.h
    source/external/imgui/imgui_draw.cpp
    source/external/imgui/imgui_impl_sdl.cpp
    source/external/imgui/imgui_impl_sdl.h
    source/external/imgui/imgui_internal.h
    source/external/imgui/imgui_memory_editor.h
    source/external/imgui/imgui_tables.cpp
    source/external/imgui/imgui_widgets.cpp
    source/external/imgui/imgui.cpp
    source/external/imgui/imgui.h
    source/external/imgui/imstb_rectpack.h
    source/external/imgui/imstb_textedit.h
    source/external/imgui/imstb_truetype.h
    source/external/imgui/stb_image.h
    source/external/imgui/TextEditor.cpp
    source/external/imgui/TextEditor.h

    source/external/tracy/tracy/Tracy.hpp
    source/external/tracy/tracy/TracyVulkan.hpp
    source/external/tracy/TracyClient.cpp

    source/external/enkiTS/LockLessMultiReadPipe.h
    source/external/enkiTS/TaskScheduler.cpp
    source/external/enkiTS/TaskScheduler.h

    source/external/meshoptimizer/allocator.cpp
    source/external/meshoptimizer/clusterizer.cpp
    source/external/meshoptimizer/indexcodec.cpp
    source/external/meshoptimizer/indexgenerator.cpp
    source/external/meshoptimizer/meshoptimizer.h
    source/external/meshoptimizer/overdrawanalyzer.cpp
    source/external/meshoptimizer/overdrawoptimizer.cpp
    source/external/meshoptimizer/simplifier.cpp
    source/external/meshoptimizer/spatialorder.cpp
    source/external/meshoptimizer/stripifier.cpp
    source/external/meshoptimizer/vcacheanalyzer.cpp
    source/external/meshoptimizer/vcacheoptimizer.cpp
    source/external/meshoptimizer/vertexcodec.cpp
    source/external/meshoptimizer/vertexfilter.cpp
    source/external/meshoptimizer/vfetchanalyzer.cpp
    source/external/meshoptimizer/vfetchoptimizer.cpp
)

if (WIN32)
    target_sources(HydraExternal PRIVATE
        source/external/StackWalker.cpp)
endif()

target_compile_definitions(HydraExternal PRIVATE
    TRACY_ENABLE
    TRACY_ON_DEMAND
    TRACY_NO_SYSTEM_TRACING
)

if (WIN32)
    target_include_directories(HydraExternal PRIVATE
        binaries/SDL2-2.0.18/include
        source/hydra
    )
else()
    target_link_libraries(HydraExternal PRIVATE
        dl
    )
    target_include_directories(HydraExternal PRIVATE
        ${SDL2_INCLUDE_DIRS})
endif()

set_property(TARGET HydraExternal PROPERTY CXX_STANDARD 17)

add_subdirectory(source)
