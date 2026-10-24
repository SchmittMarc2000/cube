// Minimal test harness. Replaced by a real framework (Catch2/doctest) when the
// suite outgrows this - the point of M0 is that tests exist and run in CI.

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>

#include "net/embedded_server.hpp"
#include "net/net_loopback.hpp"
#include "net/net_protocol.hpp"

namespace {
struct TestEntry {
    const char* name;
    void (*fn)();
};
std::vector<TestEntry>& tests() {
    static std::vector<TestEntry> registry;
    return registry;
}
} // namespace

#define OC_TEST(name)                                                                     \
    static void name();                                                                   \
    [[maybe_unused]] static const bool name##_registered = [] {                           \
        tests().push_back({#name, &name});                                                \
        return true;                                                                      \
    }();                                                                                  \
    static void name()

#define OC_EXPECT(cond)                                                                   \
    do {                                                                                  \
        if (!(cond)) throw std::runtime_error("expect failed: " #cond);                   \
    } while (false)

OC_TEST(encode_decode_roundtrip) {
    const uint8_t payload[] = {1, 2, 3, 4, 5};
    auto frame = oc::net::encode_message(oc::net::MsgType::Hello, payload, sizeof(payload));
    OC_EXPECT(frame.size() == 3 + sizeof(payload));

    oc::net::Message msg;
    OC_EXPECT(oc::net::decode_message(frame.data(), frame.size(), msg));
    OC_EXPECT(msg.type == oc::net::MsgType::Hello);
    OC_EXPECT(msg.payload.size() == sizeof(payload));
    OC_EXPECT(msg.payload[4] == 5);
}

OC_TEST(decode_rejects_truncated) {
    const uint8_t payload[] = {9, 8, 7};
    auto frame = oc::net::encode_message(oc::net::MsgType::Heartbeat, payload, sizeof(payload));
    oc::net::Message msg;
    for (size_t cut = 0; cut < frame.size(); ++cut) {
        OC_EXPECT(!oc::net::decode_message(frame.data(), cut, msg));
    }
    OC_EXPECT(!oc::net::decode_message(nullptr, 100, msg));
}

OC_TEST(decode_rejects_length_mismatch) {
    const uint8_t garbage[] = {0x03, 0xFF, 0xFF}; // claims 65535-byte payload
    oc::net::Message msg;
    OC_EXPECT(!oc::net::decode_message(garbage, sizeof(garbage), msg));
}

OC_TEST(heartbeat_roundtrip) {
    const uint64_t stamp = 0x1122334455667788ull;
    auto frame = oc::net::make_heartbeat(stamp);

    oc::net::Message msg;
    OC_EXPECT(oc::net::decode_message(frame.data(), frame.size(), msg));
    OC_EXPECT(msg.type == oc::net::MsgType::Heartbeat);

    uint64_t echoed = 0;
    OC_EXPECT(oc::net::parse_heartbeat(msg, echoed));
    OC_EXPECT(echoed == stamp);
}

OC_TEST(heartbeat_ack_roundtrip) {
    auto frame = oc::net::make_heartbeat_ack(111, 222);
    oc::net::Message msg;
    OC_EXPECT(oc::net::decode_message(frame.data(), frame.size(), msg));

    uint64_t client_ns = 0;
    uint64_t server_ns = 0;
    OC_EXPECT(oc::net::parse_heartbeat_ack(msg, client_ns, server_ns));
    OC_EXPECT(client_ns == 111);
    OC_EXPECT(server_ns == 222);
}

OC_TEST(welcome_roundtrip) {
    auto frame = oc::net::make_welcome(oc::net::kProtocolVersion);
    oc::net::Message msg;
    OC_EXPECT(oc::net::decode_message(frame.data(), frame.size(), msg));

    uint32_t version = 0;
    OC_EXPECT(oc::net::parse_welcome(msg, version));
    OC_EXPECT(version == oc::net::kProtocolVersion);
}

OC_TEST(loopback_delivers_to_peer_only) {
    oc::net::Loopback link;
    link.client().send(oc::net::make_heartbeat(42));

    OC_EXPECT(!link.client().has_pending()); // sender never sees its own frame
    OC_EXPECT(link.server().has_pending());

    int seen = 0;
    link.server().poll([&](const uint8_t*, size_t) { ++seen; });
    OC_EXPECT(seen == 1);
    OC_EXPECT(!link.server().has_pending()); // drained
}

OC_TEST(embedded_server_answers_heartbeat) {
    oc::net::Loopback link;
    oc::net::EmbeddedServer server(link.server());

    const uint64_t stamp = 777;
    link.client().send(oc::net::make_heartbeat(stamp));
    server.update(12345);

    int acks = 0;
    link.client().poll([&](const uint8_t* data, size_t size) {
        oc::net::Message msg;
        if (!oc::net::decode_message(data, size, msg)) return;
        if (msg.type != oc::net::MsgType::HeartbeatAck) return;
        uint64_t echoed = 0;
        uint64_t server_ns = 0;
        if (!oc::net::parse_heartbeat_ack(msg, echoed, server_ns)) return;
        OC_EXPECT(echoed == stamp);
        OC_EXPECT(server_ns == 12345);
        ++acks;
    });

    OC_EXPECT(acks == 1);
    OC_EXPECT(server.heartbeats_received() == 1);
}

OC_TEST(embedded_server_welcomes_hello) {
    oc::net::Loopback link;
    oc::net::EmbeddedServer server(link.server());

    link.client().send(oc::net::encode_message(oc::net::MsgType::Hello, nullptr, 0));
    server.update(0);

    bool welcomed = false;
    link.client().poll([&](const uint8_t* data, size_t size) {
        oc::net::Message msg;
        if (!oc::net::decode_message(data, size, msg)) return;
        uint32_t version = 0;
        if (oc::net::parse_welcome(msg, version)) {
            OC_EXPECT(version == oc::net::kProtocolVersion);
            welcomed = true;
        }
    });

    OC_EXPECT(welcomed);
}

int main() {
    size_t failed = 0;
    for (const auto& test : tests()) {
        try {
            test.fn();
            std::printf("[ PASS ] %s\n", test.name);
        } catch (const std::exception& e) {
            ++failed;
            std::printf("[ FAIL ] %s: %s\n", test.name, e.what());
        } catch (...) {
            ++failed;
            std::printf("[ FAIL ] %s: unknown error\n", test.name);
        }
    }
    std::printf("%zu/%zu passed\n", tests().size() - failed, tests().size());
    return failed == 0 ? 0 : 1;
}
