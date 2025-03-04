#include "nats/core.h"

#include <cassert>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

nats::MessageResult nats::Core::handleMsg(std::streambuf& buf) {
    // expected syntax:
    // MSG <subject> <sid> [reply-to] <#bytes>␍␊
    std::istream is(&buf);
    std::vector<std::string> tokens;
    {
        std::string line;
        if (!std::getline(is, line) || line.empty() || line.back() != '\r') {
            return std::unexpected(Error{"malformed line"});
        }

        std::string token;
        std::istringstream iss(line);
        while (iss >> token) {
            tokens.push_back(token);
        }
    }

    if (tokens.size() < 3 || tokens[0] != "MSG") {
        return std::unexpected{Error{"bad syntax"}};
    }

    Message msg { .subject=tokens[1], .sid=tokens[2]}; 
    std::string bytes_as_str = "";
    if (tokens.size() > 4) {
        msg.replyTo = tokens[3];
        bytes_as_str = tokens[4];
    } else if (tokens.size() > 3) {
        bytes_as_str = tokens[3];
    } else {
        return std::unexpected(nats::Error{"missing bytes specifier"});
    }
    
    std::optional<std::size_t> bytes;
    try {
        bytes = std::stoi(bytes_as_str);
    } catch (...) {
        return std::unexpected(nats::Error{"malformed bytes: " + bytes_as_str});
    }
        
    msg.bytes = bytes.value();
    size_t bytes_to_read = msg.bytes + 2;
    if (buf.in_avail() < bytes_to_read) {
        bytes_to_read -= buf.in_avail();
        return MessageNeedsMoreData{ .bytes = bytes_to_read, .partial = msg };
    }
    return completeMsg(buf, std::move(msg));
}

nats::Message nats::Core::completeMsg(std::streambuf& buf, Message&& in) {
    // if (buf.in_avail() < (in.bytes + 2)) {
    //     return std::unexpected(in.bytes + 2 - buf.in_avail());
    // }

    assert(buf.in_avail() >= (in.bytes + 2));

    auto msg = in;
    std::istream is(&buf);
    msg.payload.resize(msg.bytes);
    is.read(msg.payload.data(), msg.bytes);

    // consume the trailing CRLF (2 bytes)
    buf.sbumpc();
    buf.sbumpc();

    // if (const auto it = handlers_.find(msg.sid); it != handlers_.end()) {
    //     it->second(msg);
    //     // handler should stay in the hash table until unsubscribed.
    // } else {
    //     log_(LogLevel::INFO, "No handler for message with sid " + msg.sid);
    // }
    return msg;
}