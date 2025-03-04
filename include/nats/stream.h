#ifndef NATS_STREAM_H
#define NATS_STREAM_H

#include "core.h"
#include <ostream>
#include <string>

namespace nats {

inline std::string to_string(const Message& msg)
{
    return "Message{" + msg.subject
        + "," + msg.sid
        + ",[" + msg.replyTo.value_or("{none}")
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

} // namespace nats
#endif // NATS_STREAM_H