# Engine layer: platform abstraction, rendering, audio, UI (built up over milestones).

add_library(ocbase STATIC
    base/oc_log.cpp
)
target_include_directories(ocbase PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_features(ocbase PUBLIC cxx_std_20)

add_library(ocplatform STATIC
    platform/window_sdl3.cpp
)
target_link_libraries(ocplatform PUBLIC ocbase SDL3::SDL3)

add_library(ocrhi STATIC
    rhi/d3d11_device.cpp
)
target_link_libraries(ocrhi PUBLIC ocbase d3d11 dxgi)
