#ifndef NATS_OPERATORS_H
#define NATS_OPERATORS_H

namespace nats {
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
}      // namespace nats
#endif // NATS_OPERATORS_H