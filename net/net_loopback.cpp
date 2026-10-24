#include "net/net_loopback.hpp"

namespace oc::net {

void LoopbackEndpoint::send(const uint8_t* data, size_t size) {
    if (m_peer == nullptr || data == nullptr || size == 0) return;
    std::lock_guard<std::mutex> lock(m_peer->m_mutex);
    m_peer->m_inbox.emplace_back(data, data + size);
}

void LoopbackEndpoint::poll(const Handler& handler) {
    std::deque<std::vector<uint8_t>> drained;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        drained.swap(m_inbox);
    }
    for (const auto& frame : drained) {
        handler(frame.data(), frame.size());
    }
}

bool LoopbackEndpoint::has_pending() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_inbox.empty();
}

Loopback::Loopback() {
    m_client = std::make_unique<LoopbackEndpoint>();
    m_server = std::make_unique<LoopbackEndpoint>();
    m_client->connect(m_server.get());
    m_server->connect(m_client.get());
}

} // namespace oc::net
