#ifndef NATS_CORE_H
#define NATS_CORE_H

#include <cstddef>
#include <expected>
#include <optional>
#include <ostream>
#include <streambuf>
#include <string>
#include <variant>

namespace nats {

struct Message {
    std::string subject;
    std::string sid;
    std::optional<std::string> replyTo;
    std::size_t bytes = 0;
    std::string payload;
};

inline bool operator!=(const Message& lhs, const Message& rhs) {
    return lhs.bytes != rhs.bytes ||
        lhs.subject != rhs.subject ||
        lhs.sid != rhs.sid ||
        lhs.payload != rhs.payload ||
        lhs.replyTo != rhs.replyTo;
}

inline bool operator==(const Message& lhs, const Message& rhs) {
    return !(lhs != rhs);
}

/// @brief  more data is needed to finish parsing
///
/// 'bytes' is present when the exact number of additional bytes is known.
/// otherwise, it generally means that the first \r\n has not been encountered.
struct MessageNeedsMoreData {
    std::optional<std::size_t> bytes;
    Message partial;
};

inline bool operator!=(const MessageNeedsMoreData& lhs, const MessageNeedsMoreData& rhs) {
    return lhs.bytes != rhs.bytes ||
        lhs.partial != rhs.partial;
}

inline bool operator==(const MessageNeedsMoreData& lhs, const MessageNeedsMoreData& rhs) {
    return !(lhs != rhs);
}

typedef std::variant<Message, MessageNeedsMoreData> OkMessage;

struct Error {
    std::string what;
};

inline bool operator!=(const Error& lhs, const Error& rhs) {
    return false;
}

inline bool operator==(const Error& lhs, const Error& rhs) {
    return !(lhs != rhs);
}

typedef std::expected<OkMessage, Error> MessageResult;

class Core {
public:
    Core() = default;
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    virtual ~Core() = default;

    /// @brief  Procees message from the NATS server
    ///
    /// This function can read a partial message and signal to the caller that
    /// more bytes are required. The caller should inspect the second element of
    /// the tuple to determine if the message is complete. A value of 0 indicates
    /// that the message is complete.
    ///
    /// The entire message header is expected to be in the buffer, else an error is
    /// returned.
    ///
    /// @param is This buffer contains the bytes received from the server.
    /// @return A tuple containing the message and the number of bytes required to complete the message.
    MessageResult handleMsg(std::streambuf& is);
    //std::expected<Message, std::size_t> completeMsg(std::streambuf& is, const Message& msg);

private:
    Message completeMsg(std::streambuf& is, Message&& msg);
};

} // namespace nats

#endif // NATS_CORE_H
