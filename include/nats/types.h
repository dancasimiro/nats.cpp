#ifndef NATS_TYPES_H
#define NATS_TYPES_H

#include <cstddef>
#include <expected>
#include <optional>
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

/// @brief  more data is needed to finish parsing
///
/// 'bytes' is present when the exact number of additional bytes is known.
/// otherwise, it generally means that the first \r\n has not been encountered.
struct MessageNeedsMoreData {
    std::optional<std::size_t> bytes;
    Message partial;
};

typedef std::variant<Message, MessageNeedsMoreData> OkMessage;

struct Error {
    std::string what;
};

typedef std::expected<OkMessage, Error> MessageResult;

}      // namespace nats
#endif // NATS_TYPES_H