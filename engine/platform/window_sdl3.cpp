#include "engine/platform/window.hpp"

#include <SDL3/SDL.h>

#include "engine/base/oc_log.hpp"

namespace oc::platform {

bool init_video() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        oc::log::errorf("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

void shutdown_video() { SDL_Quit(); }

bool Window::create(const WindowDesc& desc) {
    SDL_WindowFlags flags = desc.resizable ? SDL_WINDOW_RESIZABLE : 0;
    m_window = SDL_CreateWindow(desc.title, desc.width, desc.height, flags);
    if (m_window == nullptr) {
        oc::log::errorf("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

Window::~Window() { destroy(); }

void Window::destroy() {
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

bool Window::pump_events() {
    m_events.clear();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            m_should_close = true;
        }
        m_events.push_back(event);
    }
    return !m_should_close;
}

void Window::size(int& w, int& h) const {
    if (m_window == nullptr) {
        w = h = 0;
        return;
    }
    SDL_GetWindowSize(m_window, &w, &h);
}

void* Window::native_hwnd() const {
    if (m_window == nullptr) return nullptr;
    SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
}

} // namespace oc::platform
