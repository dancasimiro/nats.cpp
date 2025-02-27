#include "nats_client.h"
#include "simdjson.h"

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
            if (cmd == "+OK") {
            } else if (cmd == "PING") {
                pong();
            } else if (cmd == "MSG") {
                handleMsg(is);
            } else if (cmd == "INFO") {
                auto info = handleInfo(is);
                info.verbose = true;
                connect(info);
            } else {
                log_(LogLevel::INFO, "Received->" + line);
            }
            doRead(); // Continue reading for more responses
        } else {
            std::string error;
            if (!std::getline(is, error)) {
                error = "(no error message)";
            }
            log_(LogLevel::ERROR, "server error: " + error);
            close();
        }
    } else {
        log_(LogLevel::ERROR, "stream error reading command");
        close();
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

void NATSClient::pub(const std::string& subject) {
    const auto pub_msg = "PUB " + subject + " 5\r\nhello\r\n";
    send(pub_msg);
}

void NATSClient::hpub(const std::string& subject) {
    const auto hpub_msg = "HPUB " + subject + " 5\r\nhello\r\n";
    send(hpub_msg);
}

std::string NATSClient::sub(const std::string& subject) {
    const auto sub_msg = "SUB " + subject + " 1\r\n";
    send(sub_msg);
    return "1";
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

void NATSClient::handleMsg(std::istream& is) {
    std::string msg;
    std::getline(is, msg);
    log_(LogLevel::INFO, msg);
}
