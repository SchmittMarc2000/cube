#include "net/embedded_server.hpp"

#include "engine/base/oc_log.hpp"
#include "net/net_protocol.hpp"

namespace oc::net {

EmbeddedServer::EmbeddedServer(LoopbackEndpoint& link) : m_link(&link) {}

void EmbeddedServer::update(uint64_t now_ns) {
    if (m_link == nullptr) return;

    m_link->poll([&](const uint8_t* data, size_t size) {
        m_bytes += size;

        Message msg;
        if (!decode_message(data, size, msg)) {
            oc::log::warn("server: dropped malformed frame");
            return;
        }

        switch (msg.type) {
            case MsgType::Hello:
                m_link->send(make_welcome(kProtocolVersion));
                break;

            case MsgType::Heartbeat: {
                uint64_t client_ns = 0;
                if (!parse_heartbeat(msg, client_ns)) {
                    oc::log::warn("server: malformed heartbeat payload");
                    break;
                }
                ++m_heartbeats;
                m_link->send(make_heartbeat_ack(client_ns, now_ns));
                break;
            }

            case MsgType::Welcome:
            case MsgType::HeartbeatAck:
                // Server-bound only; ignore if a misbehaving client sends one.
                break;
        }
    });
}

} // namespace oc::net
