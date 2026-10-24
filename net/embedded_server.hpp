#pragma once

#include <cstdint>

#include "net/net_loopback.hpp"

namespace oc::net {

// Minimal authoritative-server skeleton for M0. The transport is a loopback
// pair; M1 swaps in a real UDP socket transport and a tick loop. The protocol
// handling below is the part that survives unchanged.
class EmbeddedServer {
public:
    explicit EmbeddedServer(LoopbackEndpoint& link);

    // Pump the link and answer protocol messages. Call once per frame/tick.
    void update(uint64_t now_ns);

    uint64_t heartbeats_received() const { return m_heartbeats; }
    uint64_t bytes_received() const { return m_bytes; }

private:
    LoopbackEndpoint* m_link = nullptr;
    uint64_t m_heartbeats = 0;
    uint64_t m_bytes = 0;
};

} // namespace oc::net
