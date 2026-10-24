#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oc::net {

// Bump when the wire format changes in a way old builds can't parse.
constexpr uint32_t kProtocolVersion = 1;

enum class MsgType : uint8_t {
    Hello = 1,        // client -> server: introduce yourself
    Welcome = 2,      // server -> client: accepted, carries protocol version
    Heartbeat = 3,    // client -> server: liveness probe
    HeartbeatAck = 4, // server -> client: echo + server clock
};

struct Message {
    MsgType type = MsgType::Heartbeat;
    std::vector<uint8_t> payload;
};

const char* to_string(MsgType type);

// Wire format: [u8 type][u16 little-endian payload size][payload bytes]
std::vector<uint8_t> encode_message(MsgType type, const uint8_t* payload, size_t payload_size);
bool decode_message(const uint8_t* data, size_t size, Message& out);

// Heartbeat: u64 client steady-clock at send time.
std::vector<uint8_t> make_heartbeat(uint64_t client_ns);
bool parse_heartbeat(const Message& msg, uint64_t& client_ns);

// HeartbeatAck: u64 echo of the client timestamp, u64 server steady-clock.
std::vector<uint8_t> make_heartbeat_ack(uint64_t client_ns, uint64_t server_ns);
bool parse_heartbeat_ack(const Message& msg, uint64_t& client_ns, uint64_t& server_ns);

// Welcome: u32 protocol version.
std::vector<uint8_t> make_welcome(uint32_t protocol_version);
bool parse_welcome(const Message& msg, uint32_t& protocol_version);

} // namespace oc::net
