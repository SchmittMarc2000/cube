#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace oc::net {

// One half of an in-process network link. send() delivers to the *peer's*
// inbox, mimicking a socket pair (no loopback-to-self). Thread-safe.
class LoopbackEndpoint {
public:
    using Handler = std::function<void(const uint8_t* data, size_t size)>;

    void send(const uint8_t* data, size_t size);
    void send(const std::vector<uint8_t>& frame) { send(frame.data(), frame.size()); }

    // Drains the inbox, invoking the handler once per delivered frame.
    void poll(const Handler& handler);

    void connect(LoopbackEndpoint* peer) { m_peer = peer; }
    bool has_pending() const;

private:
    LoopbackEndpoint* m_peer = nullptr;
    mutable std::mutex m_mutex;
    std::deque<std::vector<uint8_t>> m_inbox;
};

// A connected client <-> server pair that never leaves the process.
// Replaced by a real UDP transport in M1; the endpoint interface stays.
class Loopback {
public:
    Loopback();

    LoopbackEndpoint& client() { return *m_client; }
    LoopbackEndpoint& server() { return *m_server; }

private:
    std::unique_ptr<LoopbackEndpoint> m_client;
    std::unique_ptr<LoopbackEndpoint> m_server;
};

} // namespace oc::net
