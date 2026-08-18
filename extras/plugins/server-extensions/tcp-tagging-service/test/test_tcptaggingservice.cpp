// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: Inria

#include <TcpTaggingService.h>
#include <TcpTaggingProtocol.h>

#include <CNetwork.h>
#include <CServer.h>
#include <NAL.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using OpenViBE::Services::TcpTaggingService;
using OpenViBE::Services::TcpTagging::Tag;
using namespace OpenViBE::Services::TcpTagging; // FLAG_* constants

namespace {

constexpr uint16_t TEST_PORT = 15380;
constexpr uint16_t TEST_PORT_2 = 15381;

SOCKET connectClient(uint16_t port)
{
    SOCKADDR_IN addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Server-side bind/listen happens synchronously inside CServer::run(), so a
    // freshly-(re)started server should already be listening; the retry loop is
    // only a safety margin against scheduling jitter.
    for (int attempt = 0; attempt < 40; ++attempt)
    {
        SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) {
            return INVALID_SOCKET;
        }
        if (connect(s, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == 0) {
            return s;
        }

        closesocket(s);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return INVALID_SOCKET;
}

bool sendBytes(SOCKET s, const uint8_t* data, size_t len)
{
    bool bRes = true;
    size_t sent = 0;
    while (sent < len && bRes)
    {
        const int n = send(s, reinterpret_cast<const char*>(data + sent), static_cast<int>(len - sent), 0);
        if (n <= 0) {
            bRes = false;
        }
        sent += static_cast<size_t>(n);
    }
    return bRes;
}

bool sendTag(SOCKET s, const Tag& tag)
{
    return sendBytes(s, reinterpret_cast<const uint8_t*>(&tag), sizeof(tag));
}

bool waitForTag(TcpTaggingService& service, Tag& out, int timeoutMs = 2000)
{
    constexpr int stepMs = 20;
    for (int waited = 0; waited < timeoutMs; waited += stepMs)
    {
        if (service.popTag(out)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
    }
    return false;
}

} // namespace

// Single end-to-end scenario, deliberately not split into independent TEST_F
// cases: every block below shares one live TcpTaggingService singleton and one
// real bound socket, and later blocks depend on state left by earlier ones
// (e.g. the live port change near the end). Splitting this into separate cases
// would break under gtest_discover_tests, which invokes each case as its own
// process via --gtest_filter, so every case would otherwise start fresh against
// the real default port instead of the shared test port set up here.
TEST(TcpTaggingServiceTest, EndToEndScenario)
{
    TcpTaggingService& service = TcpTaggingService::instance();

    // Move off the real default port (15361, auto-started by the constructor)
    // onto a private test port before running any other test.
    service.getPortParameter()->setValue(static_cast<int>(TEST_PORT));

    {
        Tag tag;
        EXPECT_FALSE(service.popTag(tag)) << "popTag() on empty queue returns false";
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        EXPECT_NE(s, INVALID_SOCKET) << "connect to service port";
        Tag sent; sent.flags = FLAG_FPTIME; sent.identifier = 42; sent.timestamp = 123456789ULL;
        EXPECT_TRUE(sendTag(s, sent)) << "send single tag in one write";
        Tag received;
        const bool got = waitForTag(service, received);
        EXPECT_TRUE(got) << "popTag() receives the sent tag";
        if (got)
        {
            EXPECT_TRUE(received.flags == sent.flags && received.identifier == sent.identifier && received.timestamp == sent.timestamp)
                << "received tag matches sent tag byte-for-byte";
        }
        closesocket(s);
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        EXPECT_NE(s, INVALID_SOCKET) << "connect for split-send test";
        Tag sent; sent.flags = FLAG_FPTIME; sent.identifier = 7; sent.timestamp = 999999ULL;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&sent);
        bool ok = sendBytes(s, bytes, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        ok = ok && sendBytes(s, bytes + 1, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        ok = ok && sendBytes(s, bytes + 6, sizeof(sent) - 6);
        EXPECT_TRUE(ok) << "split send across 3 chunks (1/5/18 bytes)";
        Tag received;
        const bool got = waitForTag(service, received);
        EXPECT_TRUE(got) << "popTag() receives split-sent tag";
        if (got)
        {
            EXPECT_TRUE(received.identifier == sent.identifier && received.timestamp == sent.timestamp)
                << "split-sent tag reassembled correctly";
        }
        closesocket(s);
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        EXPECT_NE(s, INVALID_SOCKET) << "connect for multi-tag test";
        Tag a; a.flags = FLAG_FPTIME; a.identifier = 1; a.timestamp = 100;
        Tag b; b.flags = FLAG_FPTIME; b.identifier = 2; b.timestamp = 200;
        Tag c; c.flags = FLAG_FPTIME; c.identifier = 3; c.timestamp = 300;
        std::vector<uint8_t> buf(sizeof(Tag) * 3);
        std::memcpy(buf.data(), &a, sizeof(Tag));
        std::memcpy(buf.data() + sizeof(Tag), &b, sizeof(Tag));
        std::memcpy(buf.data() + 2 * sizeof(Tag), &c, sizeof(Tag));
        EXPECT_TRUE(sendBytes(s, buf.data(), buf.size())) << "send 3 concatenated tags in one buffer";

        Tag r1, r2, r3;
        const bool got1 = waitForTag(service, r1);
        const bool got2 = got1 && waitForTag(service, r2);
        const bool got3 = got2 && waitForTag(service, r3);
        EXPECT_TRUE(got1 && got2 && got3) << "all 3 tags popped";
        if (got1 && got2 && got3) { EXPECT_TRUE(r1.identifier == 1 && r2.identifier == 2 && r3.identifier == 3) << "tags popped in order"; }
        closesocket(s);
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        Tag sent; sent.flags = FLAG_AUTOSTAMP_SERVERSIDE; sent.identifier = 55; sent.timestamp = 4242ULL;
        sendTag(s, sent);
        Tag received;
        const bool got = waitForTag(service, received);
        EXPECT_TRUE(got) << "popTag() receives FLAG_AUTOSTAMP_SERVERSIDE tag";
        if (got) { EXPECT_NE(received.timestamp, 4242ULL) << "server overwrote client timestamp under FLAG_AUTOSTAMP_SERVERSIDE"; }
        closesocket(s);
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        Tag sent; sent.flags = FLAG_FPTIME; sent.identifier = 66; sent.timestamp = 0;
        sendTag(s, sent);
        Tag received;
        const bool got = waitForTag(service, received);
        EXPECT_TRUE(got) << "popTag() receives zero-timestamp tag";
        if (got) { EXPECT_NE(received.timestamp, 0ULL) << "server stamped a zero timestamp"; }
        closesocket(s);
    }

    {
        SOCKET s = connectClient(TEST_PORT);
        Tag sent; sent.flags = 0; sent.identifier = 77; sent.timestamp = 987654321ULL;
        sendTag(s, sent);
        Tag received;
        const bool got = waitForTag(service, received);
        EXPECT_TRUE(got) << "popTag() receives non-FPTIME tag";
        if (got)
        {
            EXPECT_NE(received.timestamp, 987654321ULL)
                << "non-FPTIME tag without autostamp flags gets server-stamped (legacy fallback behavior)";
        }
        closesocket(s);
    }

    {
        service.getPortParameter()->setValue(static_cast<int>(TEST_PORT_2));
        SOCKET s = connectClient(TEST_PORT_2);
        EXPECT_NE(s, INVALID_SOCKET) << "connect after live port change";
        Tag sent; sent.flags = FLAG_FPTIME; sent.identifier = 88; sent.timestamp = 111;
        sendTag(s, sent);
        Tag received;
        EXPECT_TRUE(waitForTag(service, received)) << "tag received on the new port after live port change";
        closesocket(s);
    }

    service.getRunParameter()->setValue(false);
}
