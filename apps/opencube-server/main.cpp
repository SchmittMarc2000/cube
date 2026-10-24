// OpenCube dedicated server - M0 skeleton.
// Runs the server protocol logic headless against a built-in probe client to
// prove the authoritative side works without a window. M1 replaces the probe
// with a real UDP transport and a fixed tick loop.

#include "engine/base/oc_clock.hpp"
#include "engine/base/oc_log.hpp"
#include "net/embedded_server.hpp"
#include "net/net_loopback.hpp"
#include "net/net_protocol.hpp"

int main() {
    using namespace oc;
    log::info("OpenCube dedicated server - M0 skeleton (no sockets yet; M1 adds ENet)");

    net::Loopback link;
    net::EmbeddedServer server(link.server());

    const uint64_t started_ns = now_ns();
    const uint64_t probe_duration_ns = 300'000'000ull; // 300 ms
    const uint64_t hard_timeout_ns = 5'000'000'000ull; // never hang
    const uint64_t probe_interval_ns = 100'000'000ull; // 100 ms

    uint64_t last_send_ns = 0;
    int acks = 0;
    const int expected_acks = 3;

    while (true) {
        const uint64_t now = now_ns();
        const uint64_t elapsed = now - started_ns;

        if (elapsed < probe_duration_ns && now - last_send_ns >= probe_interval_ns) {
            link.client().send(net::make_heartbeat(now));
            last_send_ns = now;
        }

        server.update(now);

        link.client().poll([&](const uint8_t* data, size_t size) {
            net::Message msg;
            if (net::decode_message(data, size, msg) && msg.type == net::MsgType::HeartbeatAck) {
                ++acks;
            }
        });

        if (acks >= expected_acks) break;
        if (elapsed >= hard_timeout_ns) {
            log::errorf("probe timed out: acks=%d of %d", acks, expected_acks);
            return 1;
        }
    }

    log::infof("probe complete: heartbeats=%llu acks=%d - server logic OK",
        static_cast<unsigned long long>(server.heartbeats_received()), acks);
    return 0;
}
