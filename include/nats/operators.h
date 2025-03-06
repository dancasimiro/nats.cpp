#ifndef NATS_OPERATORS_H
#define NATS_OPERATORS_H

#include "types.h"
#include <string>
#include <optional>
#include <vector>

namespace nats {

inline bool operator!=(const Message& lhs, const Message& rhs) {
    return lhs.bytes != rhs.bytes ||
        lhs.subject != rhs.subject ||
        lhs.sid != rhs.sid ||
        lhs.payload != rhs.payload ||
        lhs.reply_to != rhs.reply_to;
}

inline bool operator==(const Message& lhs, const Message& rhs) {
    return !(lhs != rhs);
}

inline bool operator!=(const MessageNeedsMoreData& lhs, const MessageNeedsMoreData& rhs) {
    return lhs.bytes != rhs.bytes ||
        lhs.partial != rhs.partial;
}

inline bool operator==(const MessageNeedsMoreData& lhs, const MessageNeedsMoreData& rhs) {
    return !(lhs != rhs);
}

inline bool operator!=(const Error& lhs, const Error& rhs) {
    return false;
}

inline bool operator==(const Error& lhs, const Error& rhs) {
    return !(lhs != rhs);
}

inline bool operator!=(const Info& lhs, const Info& rhs) {
    return lhs.server_id != rhs.server_id ||
        lhs.server_name != rhs.server_name ||
        lhs.version != rhs.version ||
        lhs.go != rhs.go ||
        lhs.host != rhs.host ||
        lhs.port != rhs.port ||
        lhs.headers != rhs.headers ||
        lhs.max_payload != rhs.max_payload ||
        lhs.proto != rhs.proto ||
        lhs.client_id != rhs.client_id ||
        lhs.auth_required != rhs.auth_required ||
        lhs.tls_required != rhs.tls_required ||
        lhs.tls_verified != rhs.tls_verified ||
        lhs.tls_available != rhs.tls_available ||
        lhs.connect_urls != rhs.connect_urls ||
        lhs.ws_connect_urls != rhs.ws_connect_urls ||
        lhs.ldm != rhs.ldm ||
        lhs.git_commit != rhs.git_commit ||
        lhs.jetstream != rhs.jetstream ||
        lhs.ip != rhs.ip ||
        lhs.client_ip != rhs.client_ip ||
        lhs.nonce != rhs.nonce ||
        lhs.cluster != rhs.cluster ||
        lhs.domain != rhs.domain;
}

inline bool operator==(const Info& lhs, const Info& rhs) {
    return !(lhs != rhs);
}

}      // namespace nats
#endif // NATS_OPERATORS_H