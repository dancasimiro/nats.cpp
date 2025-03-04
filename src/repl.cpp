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
    std::string line;
    if (!std::getline(is, line)) {
        std::cerr << "Error reading input line" << std::endl;
        return false;
    }
    
    std::vector<std::string> tokens;
    {
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
    }

    auto stop_reading = false;
    if (!tokens.empty()) {
        const auto input = tokens.front();
        if (input == "exit" || input == "quit") {
            stop_reading = true;
        } else if (input == "sub") {
            const auto subject = tokens.size() > 1 ? tokens[1] : "foo";
            nats_client_.sub({.subject=subject, .sid="1"}, [](const nats::Message& msg) {
                std::cout << "Received message: " << msg.payload << std::endl;
                return nats::Message{};
            });
        } else if (input == "unsub") {
            nats_client_.unsub("1");
        } else if (input == "pub") {
            nats_client_.pub({
                .subject=tokens.size() > 1 ? tokens[1] : "foo", 
                .payload=tokens.size() > 2 ? tokens[2] : "hello"
            });
        } else if (input == "hpub") {
            const auto subject = tokens.size() > 1 ? tokens[1] : "foo";
            nats_client_.hpub(subject);
        } else if (input == "request") {
            const auto logger = [this](LogLevel level, const std::string& msg) {
                print(level, msg);
            };
            request(nats_client_, {
                .subject=tokens.size() > 1 ? tokens[1] : "foo",
                .payload=tokens.size() > 2 ? tokens[2] : "hello"},
                [logger](const nats::Message& msg) {
                    logger(LogLevel::INFO, "Received reply: " + msg.payload);
                    return nats::Message{};
            });
        } else if (input == "reply") {
            const auto subject = tokens.size() > 1 ? tokens[1] : "foo";
            const auto payload = tokens.size() > 2 ? tokens[2] : "hello";
            print(LogLevel::INFO, "Listening on [" + subject + "] for requests");
            const auto logger = [this](LogLevel level, const std::string& msg) {
                print(level, msg);
            };
            reply(nats_client_, subject, [payload, logger](const nats::Message& msg) {
                logger(LogLevel::INFO, "Received request: " + msg.payload);
                return nats::Message{.payload = payload};
            });
        } else {
            std::cerr << "Unknown command: " << input << std::endl;
        }
    } else {
        std::cerr << "empty input" << std::endl;
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
