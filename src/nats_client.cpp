#include "nats_client.h"
#include "simdjson.h"
#include <vector>

NATSClient::NATSClient(net::io_context& io_context, const std::string& host, const std::string& port)
    : io_context_(io_context), resolver_(io_context), socket_(io_context), host_(host), port_(port) {}

void NATSClient::start() {
    auto endpoints = resolver_.resolve(host_, port_);
    net::async_connect(socket_, endpoints,
        [this](const boost::system::error_code& ec, tcp::endpoint) {
            onConnect(ec);
        });
}

void NATSClient::shutdown() {
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        log_(LogLevel::ERROR, "Error shutting down socket: " + ec.message());
    }
}

void NATSClient::send(const std::string& message) {
    net::async_write(socket_, net::buffer(message),
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            onWrite(ec, bytes_transferred);
        });
}

void NATSClient::close() {
    boost::system::error_code ec;
    socket_.close(ec);
    if (ec) {
        log_(LogLevel::ERROR, "Error closing socket: " + ec.message());
    }
}

void NATSClient::onConnect(const boost::system::error_code& ec) {
    if (!ec) {
        doRead();
    } else {
        log_(LogLevel::ERROR, "Error connecting to NATS server: " + ec.message());
    }
}

void NATSClient::onWrite(const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
    if (ec) {
        log_(LogLevel::ERROR, "Error sending message to NATS server: " + ec.message());
    }
}

void NATSClient::doRead() {
    net::async_read_until(socket_, response_, "\r\n",
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        });
}

void NATSClient::onRead(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::istream response_stream(&response_);
        std::string response_line;
        /// @todo std::getline is likely too naive to handle binary messages
        if (std::getline(response_stream, response_line)) {
            evaluateLine(response_line);
        } else {
            log_(LogLevel::ERROR, "could not read response line");
            close();
        }
    } else if (ec == net::error::eof) {
        log_(LogLevel::INFO, "Connection closed by server.");
        close();
    } else {
        log_(LogLevel::ERROR, "Error reading from NATS server: " + ec.message());
        close();
    }
}

void NATSClient::evaluateLine(const std::string& line) {
    std::istringstream is(line);
    std::string cmd;
    if (is >> cmd) {
        if (cmd != "-ERR") {
            std::function<void()> nextOp = [this] { doRead(); };
            if (cmd == "+OK") {
            } else if (cmd == "PING") {
                pong();
            } else if (cmd == "MSG") {
                nextOp = handleMsg(is);
            } else if (cmd == "INFO") {
                auto info = handleInfo(is);
                info.verbose = true;
                connect(info);
            } else {
                log_(LogLevel::INFO, "Received->" + line);
            }
            nextOp();
        } else {
            std::string error;
            if (!std::getline(is, error)) {
                error = "(no error message)";
            }
            log_(LogLevel::ERROR, "server error: " + error);
            close();
        }
    } else {
        log_(LogLevel::INFO, "stream error reading command");
    }
}
void NATSClient::connect(const NATSInfo& info) {
    log_(LogLevel::INFO, "connected to server name " + info.server_name);
    const auto connect_msg = "CONNECT {\"verbose\":true,\"pedantic\":false,\"tls_required\":false,\"name\":\"nats-client\",\"lang\":\"cpp\",\"version\":\"0.1.0\"}\r\n";
    send(connect_msg);
}

void NATSClient::ping() {
    const auto ping_msg = "PING\r\n";
    send(ping_msg);
}

void NATSClient::pong() {
    const auto pong_msg = "PONG\r\n";
    send(pong_msg);
}

void NATSClient::pub(const Message& msg) {
    auto pub_msg = "PUB " + msg.subject;
    if (msg.replyTo.has_value()) {
        pub_msg += " " + *msg.replyTo;
    }
    pub_msg += " " + std::to_string(msg.payload.size()) + "\r\n" + msg.payload + "\r\n";
    send(pub_msg);
}

void NATSClient::hpub(const std::string& subject) {
    const auto hpub_msg = "HPUB " + subject + " 5\r\nhello\r\n";
    send(hpub_msg);
}

void NATSClient::sub(const Subscription& subscription, const MessageHandler& handler) {
    handlers_.insert({subscription.sid, handler});

    auto sub_msg = "SUB " + subscription.subject;
    if (subscription.queueGroup.has_value()) {
        sub_msg += " " + subscription.queueGroup.value();
    }
    sub_msg += " " + subscription.sid + "\r\n";
    send(sub_msg);
}

void NATSClient::unsub(const std::string& sid) {
    const auto unsub_msg = "UNSUB " + sid + "\r\n";
    send(unsub_msg);
}

NATSInfo NATSClient::handleInfo(std::istream& is) {
    if (const auto result = parseInfo(is); result.has_value()) {
        return result.value();
    } else {
        log_(LogLevel::ERROR, "error parsing info: " + result.error().message);
        return NATSInfo{};
    }
}

std::expected<NATSInfo, NATSError> NATSClient::parseInfo(std::istream& is) {
    std::string info_json;
    if (!std::getline(is, info_json)) {
        return std::unexpected(NATSError{"info payload stream error"});
    }

    simdjson::ondemand::document doc;
    simdjson::ondemand::parser parser;
    try {
        simdjson::padded_string payload(info_json);
        doc = parser.iterate(payload);
        return NATSInfo{.server_name{std::string_view(doc["server_name"])}};
    } catch (simdjson::simdjson_error& error) {
        const char* current_location = doc.current_location();
        log_(LogLevel::ERROR, "JSON error: " + std::string(error.what()) + " near " + current_location + " in " + info_json);
        return std::unexpected(NATSError{error.what()});
    }
}

std::function<void()> NATSClient::handleMsg(std::istream& is) {
    // expected syntax:
    // MSG <subject> <sid> [reply-to] <#bytes>␍␊
    std::vector<std::string> tokens;
    {
        std::string token;
        while (is >> token) {
            tokens.push_back(token);
        }
    }

    Message msg;
    if (tokens.size() > 0) {
        msg.subject = tokens[0];
    }
    if (tokens.size() > 1) {
        msg.sid = tokens[1];
    }

    if (tokens.size() > 3) {
        msg.replyTo = tokens[2];
        msg.bytes = std::stoi(tokens[3]);
    } else if (tokens.size() > 2) {
        msg.bytes = std::stoi(tokens[2]);
    }

    return [this, msg] {
        // add two bytes for the CRLF
        size_t bytes_to_read = msg.bytes + 2;
        if (bytes_to_read > response_.size()) {
            bytes_to_read -= response_.size();
            std::cout << "schedule read of " << bytes_to_read << " bytes" << std::endl;
            net::async_read(socket_, response_, net::transfer_exactly(bytes_to_read),
            [this, msg](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
                std::cout << "received msg of " << bytes_transferred << " bytes" << std::endl;
                if (!ec) {
                    std::istream response_stream(&response_);
                    handleMsgPayload(msg, response_stream);
                    doRead();
                } else {
                    onRead(ec, bytes_transferred);
                }
            });
        } else {
            std::istream response_stream(&response_);
            handleMsgPayload(msg, response_stream);
            doRead();
        }
    };
}

void NATSClient::handleMsgPayload(const Message& in, std::istream& is) {
    ///@todo handle binary data
    Message msg = in;
    std::getline(is, msg.payload);
    if (const auto it = handlers_.find(msg.sid); it != handlers_.end()) {
        it->second(msg);
        // handler should stay in the hash table until unsubscribed.
    } else {
        log_(LogLevel::INFO, "No handler for message with sid " + msg.sid);
    }
}


void request(NATSClient& nats_client, const NATSClient::Message& tmplt, const NATSClient::MessageHandler& handler) {
    const auto replyInbox = "inbox";
    const NATSClient::Subscription sub = {.subject=replyInbox, .sid = "inbox.1"};
    nats_client.sub(sub, [&nats_client, sub, handler](const NATSClient::Message& msg) {
        nats_client.unsub(sub.sid);
        return handler(msg);
    });
    NATSClient::Message msg = tmplt;
    msg.replyTo = replyInbox;
    nats_client.pub(msg);
}

void reply(NATSClient& nats_client, const std::string& subject, const NATSClient::MessageHandler& handler) {
    nats_client.sub({.subject=subject, .sid = "1"}, [handler, &nats_client](const NATSClient::Message& msg) {
        NATSClient::Message response = handler(msg);
        if (msg.replyTo.has_value()) {
            response.subject = msg.replyTo.value();
            nats_client.pub(response);
        }
        return response;
    });
}
