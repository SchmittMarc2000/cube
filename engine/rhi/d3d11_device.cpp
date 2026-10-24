#include "engine/rhi/d3d11_device.hpp"

#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include "engine/base/oc_log.hpp"

using Microsoft::WRL::ComPtr;

namespace oc::rhi {

struct DeviceD3D11::Impl {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
};

namespace {

bool try_create_device(DeviceD3D11::Impl& impl, D3D_DRIVER_TYPE driver, UINT extra_flags, bool& used_debug_layer) {
    static const D3D_FEATURE_LEVEL kFeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
    };

    const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | extra_flags;
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_10_1;
    HRESULT hr = D3D11CreateDevice(nullptr, driver, nullptr, flags, kFeatureLevels,
        ARRAYSIZE(kFeatureLevels), D3D11_SDK_VERSION, impl.device.ReleaseAndGetAddressOf(), &obtained,
        impl.context.ReleaseAndGetAddressOf());

    if (FAILED(hr)) return false;
    used_debug_layer = (extra_flags & D3D11_CREATE_DEVICE_DEBUG) != 0;

    // Climb to DXGI so we can create a swapchain and inspect the adapter.
    ComPtr<IDXGIDevice> dxgi_device;
    if (FAILED(impl.device->QueryInterface(IID_PPV_ARGS(&dxgi_device)))) return false;
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgi_device->GetAdapter(&adapter))) return false;
    if (FAILED(adapter->GetParent(IID_PPV_ARGS(&impl.factory)))) return false;
    return true;
}

std::string narrow(const wchar_t* wide) {
    if (wide == nullptr) return {};
    char buffer[256] = {};
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer, sizeof(buffer), nullptr, nullptr);
    return std::string(buffer);
}

} // namespace

DeviceD3D11::~DeviceD3D11() { shutdown(); }

bool DeviceD3D11::init(const DeviceDesc& desc) {
    if (m_impl != nullptr) {
        oc::log::error("rhi: device already initialized");
        return false;
    }
    if (desc.hwnd == nullptr) {
        oc::log::error("rhi: DeviceDesc.hwnd is null");
        return false;
    }
    m_hwnd = desc.hwnd;
    m_width = desc.width;
    m_height = desc.height;

    m_impl = new Impl();

    const UINT debug_flag = desc.enable_debug ? D3D11_CREATE_DEVICE_DEBUG : 0u;
    bool debug_layer_used = false;
    bool ok = false;

    if (desc.force_warp) {
        ok = try_create_device(*m_impl, D3D_DRIVER_TYPE_WARP, debug_flag, debug_layer_used) ||
             try_create_device(*m_impl, D3D_DRIVER_TYPE_WARP, 0, debug_layer_used);
        m_warp = ok;
    } else {
        ok = try_create_device(*m_impl, D3D_DRIVER_TYPE_HARDWARE, debug_flag, debug_layer_used);
        if (!ok && debug_flag != 0) {
            // The debug layer is an optional OS feature; don't die without it.
            oc::log::warn("rhi: hardware device with debug layer failed, retrying without");
            ok = try_create_device(*m_impl, D3D_DRIVER_TYPE_HARDWARE, 0, debug_layer_used);
        }
        if (!ok) {
            oc::log::warn("rhi: hardware device creation failed, falling back to WARP");
            ok = try_create_device(*m_impl, D3D_DRIVER_TYPE_WARP, 0, debug_layer_used);
            m_warp = ok;
        }
    }

    if (!ok) {
        oc::log::error("rhi: all device creation attempts failed");
        return false;
    }
    if (debug_layer_used) oc::log::info("rhi: D3D11 debug layer active");

    // Adapter name for diagnostics.
    {
        ComPtr<IDXGIDevice> dxgi_device;
        ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(m_impl->device->QueryInterface(IID_PPV_ARGS(&dxgi_device))) &&
            SUCCEEDED(dxgi_device->GetAdapter(&adapter))) {
            DXGI_ADAPTER_DESC adapter_desc = {};
            if (SUCCEEDED(adapter->GetDesc(&adapter_desc))) {
                m_adapter_name = narrow(adapter_desc.Description);
            }
        }
        if (m_adapter_name.empty()) m_adapter_name = m_warp ? "WARP (software)" : "unknown";
        if (m_adapter_name.find("Basic Render") != std::string::npos) m_warp = true;
    }

    if (!create_swapchain(desc.width, desc.height)) return false;

    oc::log::infof("rhi: D3D11 device on %s%s", m_adapter_name.c_str(), m_warp ? " (WARP)" : "");
    return true;
}

bool DeviceD3D11::create_swapchain(uint32_t width, uint32_t height) {
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    HRESULT hr = m_impl->factory->CreateSwapChainForHwnd(
        m_impl->device.Get(), static_cast<HWND>(m_hwnd), &desc, nullptr, nullptr,
        m_impl->swapchain.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        oc::log::errorf("rhi: CreateSwapChainForHwnd failed (hr=0x%08lX)", static_cast<unsigned long>(hr));
        return false;
    }

    // We handle fullscreen/alt-enter ourselves later; stop DXGI from eating input.
    m_impl->factory->MakeWindowAssociation(
        static_cast<HWND>(m_hwnd), DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

    if (!create_backbuffer_view()) return false;
    return true;
}

bool DeviceD3D11::create_backbuffer_view() {
    ComPtr<ID3D11Texture2D> backbuffer;
    HRESULT hr = m_impl->swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    if (FAILED(hr)) {
        oc::log::errorf("rhi: GetBuffer failed (hr=0x%08lX)", static_cast<unsigned long>(hr));
        return false;
    }
    hr = m_impl->device->CreateRenderTargetView(
        backbuffer.Get(), nullptr, m_impl->rtv.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        oc::log::errorf("rhi: CreateRenderTargetView failed (hr=0x%08lX)", static_cast<unsigned long>(hr));
        return false;
    }
    return true;
}

void DeviceD3D11::release_backbuffer_views() {
    if (m_impl == nullptr) return;
    if (m_impl->context != nullptr) {
        ID3D11RenderTargetView* null_rtv = nullptr;
        m_impl->context->OMSetRenderTargets(0, &null_rtv, nullptr);
    }
    m_impl->rtv.Reset();
}

void DeviceD3D11::resize(uint32_t width, uint32_t height) {
    if (m_impl == nullptr || m_impl->swapchain == nullptr) return;
    if (width == 0 || height == 0) return; // minimized
    if (width == m_width && height == m_height) return;

    release_backbuffer_views();
    HRESULT hr = m_impl->swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        oc::log::errorf("rhi: ResizeBuffers failed (hr=0x%08lX)", static_cast<unsigned long>(hr));
        return;
    }
    m_width = width;
    m_height = height;
    create_backbuffer_view();
}

void DeviceD3D11::begin_frame(float r, float g, float b, float a) {
    if (m_impl == nullptr || m_impl->rtv == nullptr) return;
    ID3D11RenderTargetView* rtv = m_impl->rtv.Get();
    m_impl->context->OMSetRenderTargets(1, &rtv, nullptr);
    const float color[4] = {r, g, b, a};
    m_impl->context->ClearRenderTargetView(rtv, color);
}

void DeviceD3D11::end_frame(bool vsync) {
    if (m_impl == nullptr || m_impl->swapchain == nullptr) return;
    m_impl->swapchain->Present(vsync ? 1 : 0, 0);
}

void DeviceD3D11::shutdown() {
    if (m_impl == nullptr) return;
    release_backbuffer_views();
    m_impl->context.Reset();
    m_impl->swapchain.Reset();
    m_impl->factory.Reset();
    m_impl->device.Reset();
    delete m_impl;
    m_impl = nullptr;
}

} // namespace oc::rhi
