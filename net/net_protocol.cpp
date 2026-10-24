#include "net/net_protocol.hpp"

#include <cstring>

namespace oc::net {
namespace {

// Explicit little-endian helpers so the format survives big-endian hosts.
void put_u16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void put_u32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

void put_u64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

uint16_t get_u16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }

uint32_t get_u32(const uint8_t* p) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= uint32_t(p[i]) << (8 * i);
    return v;
}

uint64_t get_u64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= uint64_t(p[i]) << (8 * i);
    return v;
}

constexpr size_t kHeaderSize = 3; // u8 type + u16 length
constexpr size_t kMaxPayloadSize = 64 * 1024;

} // namespace

const char* to_string(MsgType type) {
    switch (type) {
        case MsgType::Hello: return "Hello";
        case MsgType::Welcome: return "Welcome";
        case MsgType::Heartbeat: return "Heartbeat";
        case MsgType::HeartbeatAck: return "HeartbeatAck";
    }
    return "Unknown";
}

std::vector<uint8_t> encode_message(MsgType type, const uint8_t* payload, size_t payload_size) {
    std::vector<uint8_t> frame;
    frame.reserve(kHeaderSize + payload_size);
    frame.push_back(static_cast<uint8_t>(type));
    put_u16(frame, static_cast<uint16_t>(payload_size));
    if (payload != nullptr && payload_size > 0) {
        frame.insert(frame.end(), payload, payload + payload_size);
    }
    return frame;
}

bool decode_message(const uint8_t* data, size_t size, Message& out) {
    if (data == nullptr || size < kHeaderSize) return false;
    const uint16_t payload_size = get_u16(data + 1);
    if (size != kHeaderSize + payload_size) return false;
    if (payload_size > kMaxPayloadSize) return false;

    out.type = static_cast<MsgType>(data[0]);
    out.payload.assign(data + kHeaderSize, data + kHeaderSize + payload_size);
    return true;
}

std::vector<uint8_t> make_heartbeat(uint64_t client_ns) {
    std::vector<uint8_t> payload;
    put_u64(payload, client_ns);
    return encode_message(MsgType::Heartbeat, payload.data(), payload.size());
}

bool parse_heartbeat(const Message& msg, uint64_t& client_ns) {
    if (msg.type != MsgType::Heartbeat || msg.payload.size() != 8) return false;
    client_ns = get_u64(msg.payload.data());
    return true;
}

std::vector<uint8_t> make_heartbeat_ack(uint64_t client_ns, uint64_t server_ns) {
    std::vector<uint8_t> payload;
    put_u64(payload, client_ns);
    put_u64(payload, server_ns);
    return encode_message(MsgType::HeartbeatAck, payload.data(), payload.size());
}

bool parse_heartbeat_ack(const Message& msg, uint64_t& client_ns, uint64_t& server_ns) {
    if (msg.type != MsgType::HeartbeatAck || msg.payload.size() != 16) return false;
    client_ns = get_u64(msg.payload.data());
    server_ns = get_u64(msg.payload.data() + 8);
    return true;
}

std::vector<uint8_t> make_welcome(uint32_t protocol_version) {
    std::vector<uint8_t> payload;
    put_u32(payload, protocol_version);
    return encode_message(MsgType::Welcome, payload.data(), payload.size());
}

bool parse_welcome(const Message& msg, uint32_t& protocol_version) {
    if (msg.type != MsgType::Welcome || msg.payload.size() != 4) return false;
    protocol_version = get_u32(msg.payload.data());
    return true;
}

} // namespace oc::net
