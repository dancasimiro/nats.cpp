# NATS Client Library for C++

This project implements a NATS client library in C++. It allows users to connect to a NATS server, publish and subscribe to messages, and explore the NATS protocol using an included Read-Eval-Print Loop (REPL).

## Features

- Connect to a NATS server.
- Publish messages to subjects.
- Subscribe to subjects and receive messages.
- Handle various NATS server responses.
- Included REPL for exploring the NATS protocol.
- Simple and extensible design.

## Project Structure

```
nats.cpp
├── src
│ ├── main.cpp # Entry point of the application
│ ├── repl.cpp # Implementation of the REPL class
│ └── nats/*   # Implementation of the nats.cpp library
├── include
│ ├── repl.h # Public interface for the REPL class
│ └── nats/* # Public interface for the nats.cpp library
├── tests
│ └── test.cpp # Unit tests for the nats.cpp library
├── CMakeLists.txt # Build configuration file
└── README.md # Project documentation
```

## External Dependencies

The following external dependencies are required but not provided with the project:

- [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html): For asynchronous I/O operations.
- [simdjson](https://github.com/simdjson/simdjson): For fast JSON parsing.
- [Catch2](https://github.com/catchorg/Catch2): For unit testing.

Make sure to install Boost.Asio and Catch2 dependencies before building the project; simdjson is included.

## Building the Project

To build the project, you need to have CMake installed. Follow these steps:

1. Clone the repository or download the project files.
2. Open a terminal and navigate to the project directory.
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Run CMake to configure the project:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   make
   ```

## Running the REPL

After building the project, you can run the REPL by executing the generated binary:

```
./repl
```

## Running Tests

To run the unit tests, you can use the following command in the build directory:

```
make test
```

## Sample Program

Here is a sample program demonstrating how to use `nats::Core` with `NATSClient` and integrate it into a `boost::asio` main loop:

```cpp
#include <boost/asio.hpp>
#include "nats/client.h"
#include "nats/core.h"

int main() {
    boost::asio::io_context io_context;

    // Create a NATS client
    nats::Client client(io_context, "localhost", "4222");

    // Set up logging
    client.setLogging([](const std::string& message) {
        std::cout << message << std::endl;
    });

    // Start the client
    client.start();

    // Publish a message
    nats::Message msg;
    msg.subject = "test.subject";
    msg.payload = "Hello, NATS!";
    client.pub(msg);

    // Subscribe to a subject
    client.sub({"test.subject", "1"}, [](const nats::Message& msg) {
        std::cout << "Received message: " << msg.payload << std::endl;
        return msg;
    });

    // Run the Boost.Asio main loop
    io_context.run();

    return 0;
}
```

## Contributing

Feel free to contribute to this project by submitting issues or pull requests. Your feedback and contributions are welcome!