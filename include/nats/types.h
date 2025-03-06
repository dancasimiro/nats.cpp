#ifndef NATS_TYPES_H
#define NATS_TYPES_H

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace nats {

struct Error {
    std::string what;
};

struct Info {
    std::string server_id;
    std::string server_name;
    std::string version;
    std::string go;
    std::string host;
    int64_t port;
    bool headers;
    int64_t max_payload;
    int64_t proto;
    std::optional<uint64_t> client_id;
    std::optional<bool> auth_required;
    std::optional<bool> tls_required;
    std::optional<bool> tls_verified;
    std::optional<bool> tls_available;
    std::optional<std::vector<std::string>> connect_urls;
    std::optional<std::vector<std::string>> ws_connect_urls;
    std::optional<bool> ldm;
    std::optional<std::string> git_commit;
    std::optional<bool> jetstream;
    std::optional<std::string> ip;
    std::optional<std::string> client_ip;
    std::optional<std::string> nonce;
    std::optional<std::string> cluster;
    std::optional<std::string> domain;
};

struct Message {
    std::string subject;
    std::string sid;
    std::optional<std::string> reply_to;
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

typedef std::expected<Info, Error> InfoResult;
typedef std::expected<OkMessage, Error> MessageResult;

}      // namespace nats
#endif // NATS_TYPES_H