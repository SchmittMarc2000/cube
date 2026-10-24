#pragma once

#include <vector>

struct SDL_Window;
union SDL_Event;

namespace oc::platform {

struct WindowDesc {
    const char* title = "OpenCube";
    int width = 1280;
    int height = 720;
    bool resizable = true;
};

// Thin SDL3 window wrapper. Pump the event queue once per frame, then inspect
// events() before calling clear_events() (or just pump again next frame).
class Window {
public:
    Window() = default;
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(const WindowDesc& desc);
    void destroy();

    // Pumps the platform event queue. Returns false when the app should exit.
    bool pump_events();

    const std::vector<SDL_Event>& events() const { return m_events; }

    void size(int& w, int& h) const;
    void* native_hwnd() const; // HWND on Windows
    SDL_Window* sdl_window() const { return m_window; }

    bool should_close() const { return m_should_close; }
    void request_close() { m_should_close = true; }

private:
    SDL_Window* m_window = nullptr;
    std::vector<SDL_Event> m_events;
    bool m_should_close = false;
};

bool init_video();
void shutdown_video();

} // namespace oc::platform
