#include "nats_client.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE( "Link NATS Client", "[link]" ) {
    net::io_context io_context;
    NATSClient nats_client(io_context, "demo.nats.io", "4222");
}
