#include "nats/client.h"
#include "repl.h"
#include <iostream>

int main() {
    net::io_context io_context;
    NATSClient nats_client(io_context, "demo.nats.io", "4222");
    REPL repl(io_context, nats_client);
    repl.start();
    nats_client.start();

    try {
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
