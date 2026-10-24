#pragma once

#include <cstdint>
#include <string>

namespace oc::rhi {

struct DeviceDesc {
    void* hwnd = nullptr;
    uint32_t width = 1280;
    uint32_t height = 720;
    bool force_warp = false;   // software rasterizer (CI machines / no GPU)
    bool enable_debug = false; // D3D11 debug layer, if installed
};

// M0 renderer backend: device + flip-model swapchain + clear. Grows into the
// full RHI (buffers, pipelines, textures) in M1.
class DeviceD3D11 {
public:
    struct Impl;
    DeviceD3D11() = default;
    ~DeviceD3D11();
    DeviceD3D11(const DeviceD3D11&) = delete;
    DeviceD3D11& operator=(const DeviceD3D11&) = delete;

    bool init(const DeviceDesc& desc);
    void shutdown();

    // Recreates backbuffer views for a resized window. Safe to call with the
    // same size; ignores zero dimensions (minimized window).
    void resize(uint32_t width, uint32_t height);

    void begin_frame(float r, float g, float b, float a);
    void end_frame(bool vsync);

    const std::string& adapter_name() const { return m_adapter_name; }
    bool is_warp() const { return m_warp; }

private:
    bool create_swapchain(uint32_t width, uint32_t height);
    bool create_backbuffer_view();
    void release_backbuffer_views();

    Impl* m_impl = nullptr;
    void* m_hwnd = nullptr;
    std::string m_adapter_name;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_warp = false;
};

} // namespace oc::rhi
