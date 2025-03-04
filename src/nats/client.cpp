#include "nats/client.h"
#include "nats/stream.h"
#include "simdjson.h"
#include <cassert>
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
        if (evalResponse()) {
            log_(LogLevel::ERROR, "could not read response");
            close();
        } else {
            doRead();
        }
    } else if (ec == net::error::eof) {
        log_(LogLevel::INFO, "Connection closed by server.");
        close();
    } else {
        log_(LogLevel::ERROR, "Error reading from NATS server: " + ec.message());
        close();
    }
}

bool NATSClient::evalResponse() {
    auto error = false;
    const auto next = response_.sgetc();
    switch (next) {
    case '+': // +OK
        handleOk();
        break;
    case 'P': // PING
        handlePing();
        break;
    case 'M': // MSG
        handleMsg();
        break;
    case 'I': // INFO
        handleInfo();
        break;
    case '-': // -ERR
        handleErr();
        break;
    default:
        log_(LogLevel::ERROR, "unexpected character: " + std::to_string(next));
        error = true;
        break;
    };
    return error;
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

void NATSClient::handleErr() {
    std::istream is(&response_);
    std::string cmd;
    std::getline(is, cmd);
    log_(LogLevel::INFO, cmd);
}

void NATSClient::handleOk() {
    std::istream is(&response_);
    std::string cmd;
    std::getline(is, cmd);
    log_(LogLevel::INFO, cmd);
}

void NATSClient::handleInfo() {
    std::istream is(&response_);
    std::string cmd;
    is >> cmd;
    log_(LogLevel::INFO, cmd);
    if (cmd == "INFO") {
        if (const auto result = parseInfo(is); result.has_value()) {
            auto info = result.value();
            info.verbose = true;
            connect(info);
        } else {
            log_(LogLevel::ERROR, "error parsing info: " + result.error().message);
        }
    }
}

void NATSClient::handlePing() {
    std::istream is(&response_);
    std::string cmd;
    std::getline(is, cmd);
    log_(LogLevel::INFO, cmd);
    if (cmd == "PING") {
        pong();
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

void NATSClient::handleMsg() {
    log_(LogLevel::INFO, "MSG");
    if (const auto result = core_.handleMsg(response_); result.has_value()) {
        const auto& ok = result.value();
        if (std::holds_alternative<Message>(ok)) {
            handleMsgPayload(std::get<Message>(ok));
        } else if (std::holds_alternative<nats::MessageNeedsMoreData>(ok)) {
            const auto& nmd = std::get<nats::MessageNeedsMoreData>(ok);
            if (nmd.bytes.has_value()) {
                log_(LogLevel::INFO, "need " + std::to_string(nmd.bytes.value()) + " more bytes.");
            }
            log_(LogLevel::ERROR, "need to implement support for partial reads. " + to_string(nmd));
            close();
        } else {
            log_(LogLevel::ERROR, "unhandled type");
            close();
        }
    } else {
        log_(LogLevel::ERROR, "stream error reading message: " + result.error().what);
        close();
    }
}
// nats::MessageResult NATSClient::handleMsg() {
//     // expected syntax:
//     // MSG <subject> <sid> [reply-to] <#bytes>␍␊
//     if (const auto result = core_.handleMsg(response_); result.has_value()) {
//         return [this, result] {
//             const auto& ok = result.value();
//             if (std::holds_alternative<Message>(ok)) {
//                 handleMsgPayload(std::get<Message>(ok));
//                 doRead();
//             } else if (std::holds_alternative<nats::MessageNeedsMoreData>(ok)) {
//                 const auto& nmd = std::get<nats::MessageNeedsMoreData>(ok);
//                 if (nmd.bytes.has_value()) {
//                     net::async_read(socket_, response_, net::transfer_exactly(nmd.bytes.value()),
//                         [this, msg=nmd.partial](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
//                             std::cout << "received msg of " << bytes_transferred << " bytes" << std::endl;
//                             if (!ec) {
//                                 handleMsgPayload(msg);
//                                 doRead();
//                             } else {
//                                 onRead(ec, bytes_transferred);
//                             }
//                         });
//                 } else {
//                     // unhandled!
//                 }
//             } else {
//                 // unhandled!
//             }
//         };
//     } else {
//         // unhandled!
//     }
//     return []{};
// }

Message NATSClient::handleMsgPayload(const Message& msg) {
    log_(LogLevel::INFO, to_string(msg));
    if (const auto it = handlers_.find(msg.sid); it != handlers_.end()) {
        it->second(msg);
        // handler should stay in the hash table until unsubscribed.
    } else {
        log_(LogLevel::INFO, "No handler for message with sid " + msg.sid);
    }
    return msg;
}


void request(NATSClient& nats_client, const nats::Message& tmplt, const NATSClient::MessageHandler& handler) {
    const auto replyInbox = "inbox";
    const NATSClient::Subscription sub = {.subject=replyInbox, .sid = "inbox.1"};
    nats_client.sub(sub, [&nats_client, sub, handler](const nats::Message& msg) {
        nats_client.unsub(sub.sid);
        return handler(msg);
    });
    auto msg = tmplt;
    msg.replyTo = replyInbox;
    nats_client.pub(msg);
}

void reply(NATSClient& nats_client, const std::string& subject, const NATSClient::MessageHandler& handler) {
    nats_client.sub({.subject=subject, .sid = "1"}, [handler, &nats_client](const nats::Message& msg) {
        auto response = handler(msg);
        if (msg.replyTo.has_value()) {
            response.subject = msg.replyTo.value();
            nats_client.pub(response);
        }
        return response;
    });
}
