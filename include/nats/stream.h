#ifndef NATS_STREAM_H
#define NATS_STREAM_H

#include "core.h"
#include <ostream>
#include <string>
#include <optional>
#include <vector>

namespace nats {

inline std::string to_string(const Message& msg)
{
    return "Message{" + msg.subject
        + "," + msg.sid
        + ",[" + msg.reply_to.value_or("{none}")
        + "]," + std::to_string(msg.bytes)
        + "," + msg.payload
        + "}";
}

inline std::ostream& operator<<(std::ostream& os, const Message& msg)
{
    if (os) {
        os << to_string(msg);
    }
    return os;
}

inline std::string to_string(const MessageNeedsMoreData& nmd)
{
    std::string str = "MessageNeedsMoreData{";
    str += nmd.bytes.has_value() ? std::to_string(nmd.bytes.value()) : "std::nullopt";
    str += "," + to_string(nmd.partial) + "}";
    return str;
}

inline std::ostream& operator<<(std::ostream& os, const MessageNeedsMoreData& nmd)
{
    if (os) {
        os << to_string(nmd);
    }
    return os;
}

inline std::string to_string(const OkMessage& ok)
{
    if (std::holds_alternative<Message>(ok)) {
        return to_string(std::get<Message>(ok));
    } else if (std::holds_alternative<MessageNeedsMoreData>(ok)) {
        return to_string(std::get<MessageNeedsMoreData>(ok));
    } else {
        return "Unhandled OkMessage type";
    }
}

inline std::ostream& operator<<(std::ostream& os, const OkMessage& ok)
{
    if (os) {
        os << to_string(ok);
    }
    return os;
}

inline std::string to_string(const Error& err)
{
    return "Error{" + err.what + "}";
}

inline std::ostream& operator<<(std::ostream& os, const Error& err)
{
    if (os) {
        os << to_string(err);
    }
    return os;
}

inline std::string to_string(const MessageResult& result)
{
    if (result.has_value()) {
        return to_string(result.value());
    }
    return to_string(result.error());
}

inline std::ostream& operator<<(std::ostream& os, const MessageResult& t)
{
    if (os) {
        os << to_string(t);
    }
    return os;
}

// Implementing to_string for nats::Info
inline std::string to_string(const Info& info)
{
    std::string str = "Info{";
    str += "server_id=" + info.server_id + ",";
    str += "server_name=" + info.server_name + ",";
    str += "version=" + info.version + ",";
    str += "go=" + info.go + ",";
    str += "host=" + info.host + ",";
    str += "port=" + std::to_string(info.port) + ",";
    str += "headers=" + std::to_string(info.headers) + ",";
    str += "max_payload=" + std::to_string(info.max_payload) + ",";
    str += "proto=" + std::to_string(info.proto) + ",";
    str += "client_id=" + (info.client_id.has_value() ? std::to_string(info.client_id.value()) : "std::nullopt") + ",";
    str += "auth_required=" + (info.auth_required.has_value() ? std::to_string(info.auth_required.value()) : "std::nullopt") + ",";
    str += "tls_required=" + (info.tls_required.has_value() ? std::to_string(info.tls_required.value()) : "std::nullopt") + ",";
    str += "tls_verified=" + (info.tls_verified.has_value() ? std::to_string(info.tls_verified.value()) : "std::nullopt") + ",";
    str += "tls_available=" + (info.tls_available.has_value() ? std::to_string(info.tls_available.value()) : "std::nullopt") + ",";
    str += "connect_urls=[";
    if (info.connect_urls.has_value()) {
        for (const auto& url : info.connect_urls.value()) {
            str += url + ",";
        }
        if (!info.connect_urls.value().empty()) {
            str.pop_back(); // Remove the trailing comma
        }
    } else {
        str += "std::nullopt";
    }
    str += "],";
    str += "ws_connect_urls=[";
    if (info.ws_connect_urls.has_value()) {
        for (const auto& url : info.ws_connect_urls.value()) {
            str += url + ",";
        }
        if (!info.ws_connect_urls.value().empty()) {
            str.pop_back(); // Remove the trailing comma
        }
    } else {
        str += "std::nullopt";
    }
    str += "],";
    str += "ldm=" + (info.ldm.has_value() ? std::to_string(info.ldm.value()) : "std::nullopt") + ",";
    str += "git_commit=" + (info.git_commit.has_value() ? info.git_commit.value() : "std::nullopt") + ",";
    str += "jetstream=" + (info.jetstream.has_value() ? std::to_string(info.jetstream.value()) : "std::nullopt") + ",";
    str += "ip=" + (info.ip.has_value() ? info.ip.value() : "std::nullopt") + ",";
    str += "client_ip=" + (info.client_ip.has_value() ? info.client_ip.value() : "std::nullopt") + ",";
    str += "nonce=" + (info.nonce.has_value() ? info.nonce.value() : "std::nullopt") + ",";
    str += "cluster=" + (info.cluster.has_value() ? info.cluster.value() : "std::nullopt") + ",";
    str += "domain=" + (info.domain.has_value() ? info.domain.value() : "std::nullopt") + ",";
    str += "}";
    return str;
}

inline std::ostream& operator<<(std::ostream& os, const Info& info)
{
    if (os) {
        os << to_string(info);
    }
    return os;
}

inline std::string to_string(const InfoResult& result)
{
    if (result.has_value()) {
        return to_string(result.value());
    }
    return to_string(result.error());
}

inline std::ostream& operator<<(std::ostream& os, const InfoResult& t)
{
    if (os) {
        os << to_string(t);
    }
    return os;
}

} // namespace nats
#endif // NATS_STREAM_H