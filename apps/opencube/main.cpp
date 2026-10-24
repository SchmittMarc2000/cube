// OpenCube client - M0 skeleton.
// Window + D3D11 clear + input + heartbeat round-trip to the embedded server.
// Every piece here is the seed of a real subsystem; see PLAN.md milestones.

#include <cmath>
#include <cstdlib>
#include <string>

#include <SDL3/SDL.h>

#include "engine/base/oc_clock.hpp"
#include "engine/base/oc_log.hpp"
#include "engine/platform/window.hpp"
#include "engine/rhi/d3d11_device.hpp"
#include "net/embedded_server.hpp"
#include "net/net_loopback.hpp"
#include "net/net_protocol.hpp"

namespace {

struct Options {
    int width = 1280;
    int height = 720;
    int frames = 0; // 0 = run until window closed
    bool warp = false;
    bool debug_layer = false;
};

Options parse_args(int argc, char** argv) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? std::string(argv[++i]) : std::string(); };
        if (arg == "--frames") {
            opts.frames = std::atoi(next().c_str());
        } else if (arg == "--width") {
            opts.width = std::atoi(next().c_str());
        } else if (arg == "--height") {
            opts.height = std::atoi(next().c_str());
        } else if (arg == "--warp") {
            opts.warp = true;
        } else if (arg == "--debug-layer") {
            opts.debug_layer = true;
        }
    }
    return opts;
}

// h in [0,1), s/v in [0,1]. No external dependency for one color.
void hsv_to_rgb(double h, double s, double v, double& r, double& g, double& b) {
    const int sector = static_cast<int>(h * 6.0);
    const double frac = h * 6.0 - sector;
    const double p = v * (1.0 - s);
    const double q = v * (1.0 - frac * s);
    const double t = v * (1.0 - (1.0 - frac) * s);
    switch (((sector % 6) + 6) % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

} // namespace

int main(int argc, char** argv) {
    using namespace oc;
    const Options opts = parse_args(argc, argv);

    log::info("OpenCube M0 client starting");

    if (!platform::init_video()) return 1;

    platform::Window window;
    platform::WindowDesc window_desc;
    window_desc.title = "OpenCube - M0 skeleton";
    window_desc.width = opts.width;
    window_desc.height = opts.height;
    if (!window.create(window_desc)) {
        platform::shutdown_video();
        return 1;
    }

    rhi::DeviceDesc device_desc;
    int w = 0;
    int h = 0;
    window.size(w, h);
    device_desc.hwnd = window.native_hwnd();
    device_desc.width = static_cast<uint32_t>(w > 0 ? w : opts.width);
    device_desc.height = static_cast<uint32_t>(h > 0 ? h : opts.height);
    device_desc.force_warp = opts.warp;
    device_desc.enable_debug = opts.debug_layer;

    rhi::DeviceD3D11 device;
    if (!device.init(device_desc)) {
        window.destroy();
        platform::shutdown_video();
        return 1;
    }

    // Client <-> embedded server link. In-process for M0; M1 swaps the
    // loopback pair for a real transport without touching this loop's shape.
    net::Loopback link;
    net::EmbeddedServer server(link.server());

    uint64_t frames = 0;
    uint64_t beats_sent = 0;
    uint64_t acks_received = 0;
    uint64_t last_rtt_ns = 0;
    const uint64_t started_ns = now_ns();
    uint64_t last_beat_ns = started_ns;

    while (window.pump_events()) {
        for (const SDL_Event& e : window.events()) {
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                window.request_close();
            }
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                device.resize(static_cast<uint32_t>(e.window.data1), static_cast<uint32_t>(e.window.data2));
            }
        }

        server.update(now_ns());

        const uint64_t now = now_ns();
        if (now - last_beat_ns >= 1'000'000'000ull) {
            link.client().send(net::make_heartbeat(now));
            ++beats_sent;
            last_beat_ns = now;
        }

        link.client().poll([&](const uint8_t* data, size_t size) {
            net::Message msg;
            if (!net::decode_message(data, size, msg)) {
                log::warn("client: dropped malformed frame");
                return;
            }
            if (msg.type == net::MsgType::HeartbeatAck) {
                uint64_t sent_ns = 0;
                uint64_t server_ns = 0;
                if (net::parse_heartbeat_ack(msg, sent_ns, server_ns)) {
                    last_rtt_ns = now_ns() - sent_ns;
                    ++acks_received;
                    if (acks_received == 1 || acks_received % 10 == 0) {
                        log::infof("heartbeat ack #%llu rtt=%.3f ms",
                            static_cast<unsigned long long>(acks_received), ns_to_ms(last_rtt_ns));
                    }
                }
            }
        });

        // Slow hue cycle so M0 isn't a black rectangle; validates present path.
        const double t = ns_to_s(now - started_ns);
        double r = 0;
        double g = 0;
        double b = 0;
        hsv_to_rgb(std::fmod(t * 0.02, 1.0), 0.45, 0.85, r, g, b);
        device.begin_frame(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 1.0f);
        device.end_frame(/*vsync=*/true);

        ++frames;
        if (opts.frames > 0 && static_cast<int>(frames) >= opts.frames) break;
    }

    const double secs = ns_to_s(now_ns() - started_ns);
    log::infof("shutdown: %llu frames in %.2f s (%.1f fps avg), heartbeats sent=%llu acked=%llu last_rtt=%.3f ms",
        static_cast<unsigned long long>(frames), secs, secs > 0.0 ? static_cast<double>(frames) / secs : 0.0,
        static_cast<unsigned long long>(beats_sent), static_cast<unsigned long long>(acks_received),
        ns_to_ms(last_rtt_ns));

    device.shutdown();
    window.destroy();
    platform::shutdown_video();
    return 0;
}
