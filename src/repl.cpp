#include "repl.h"
#include <iostream>
#include <string>

REPL::REPL(net::io_context& io_context, NATSClient& nats_client)
    : io_context_(io_context)
    , nats_client_(nats_client)
    , input_(io_context, ::dup(STDIN_FILENO))
{
    nats_client_.setLogging([this](LogLevel level, const std::string& msg) {
        print(level, msg);
    });
}

REPL::~REPL() {
    nats_client_.setLogging(nullptr);
}

void REPL::start() {
    std::cout << "Welcome to the REPL! Type 'exit' to quit." << std::endl;
    doRead();
}

void REPL::doRead() {
    preparePrompt();
    boost::asio::async_read_until(input_, buffer_, '\n',
        [this](const boost::system::error_code& ec, std::size_t length) {
            onRead(ec, length);
        });
}

void REPL::quit()
{
    nats_client_.shutdown();
    input_.close();
}

void REPL::onRead(const boost::system::error_code& ec, std::size_t length) {
    if (!ec) {
        std::istream is(&buffer_);
        if (evaluate(is)) {
            doRead();
        } else {
            quit();
        }
    } else {
        if (ec != boost::asio::error::eof) {
            std::cerr << "Error reading input: " << ec.message() << std::endl;
        }
        quit();
    }
}

bool REPL::evaluate(std::istream &is) {
    auto stop_reading = false;
    std::string input;
    if (std::getline(is, input)) {
        if (input == "exit" || input == "quit") {
            stop_reading = true;
        } else if (input == "sub") {
            nats_client_.sub("foo");
        } else if (input == "unsub") {
            nats_client_.unsub("1");
        } else if (input == "pub") {
            nats_client_.pub("foo");
        } else if (input == "hpub") {
            nats_client_.hpub("foo");
        } else {
            std::cerr << "Unknown command: " << input << std::endl;
        }
    } else {
        std::cerr << "Error reading input" << std::endl;
        stop_reading = true;
    }
    return !stop_reading;
}

void REPL::print(LogLevel level, const std::string& msg) const {
    std::cout << std::endl;
    switch (level) {
        case LogLevel::INFO:
            std::cout << "\033[1;34mINFO: \033[0m" << msg << std::endl;
            break;
        case LogLevel::WARN:
            std::cout << "\033[1;33mWARN: \033[0m" << msg << std::endl;
            break;
        case LogLevel::ERROR:
            std::cerr << "\033[1;31mERROR: \033[0m" << msg << std::endl;
            break;
    }
    preparePrompt();
}

void REPL::preparePrompt() const {
    // print prompt in bold green
    std::cout << "\033[1;32m> \033[0m" << std::flush;
}
