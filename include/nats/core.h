#ifndef NATS_CORE_H
#define NATS_CORE_H

#include "types.h"
#include <streambuf>
#include <string>

namespace nats {

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
    MessageResult handleMsg(std::streambuf& buf);
    std::expected<Message, Error> handleMsgCompletion(std::streambuf& buf, MessageNeedsMoreData&& nmd);

    /// @brief Process info from the NATS server
    InfoResult handleInfo(std::streambuf& is);
private:
    Message completeMsg(std::streambuf& is, Message&& msg);
};

} // namespace nats

#endif // NATS_CORE_H
