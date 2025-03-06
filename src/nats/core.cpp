#include "nats/core.h"

#include <cassert>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

#include "simdjson.h"

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

    if (tokens.size() < 4 || tokens[0] != "MSG") {
        return std::unexpected{Error{"bad syntax"}};
    }

    Message msg { .subject=tokens[1], .sid=tokens[2]}; 
    std::string bytes_as_str = "";
    if (tokens.size() == 4) {
        bytes_as_str = tokens[3];
    } else if (tokens.size() == 5) {
        msg.reply_to = tokens[3];
        bytes_as_str = tokens[4];
    } else {
        return std::unexpected(nats::Error{"too many tokens"});
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
    assert(buf.in_avail() >= (in.bytes + 2));

    auto msg = in;
    std::istream is(&buf);
    msg.payload.resize(msg.bytes);
    is.read(msg.payload.data(), msg.bytes);

    // consume the trailing CRLF (2 bytes)
    buf.sbumpc();
    buf.sbumpc();

    return msg;
}

nats::InfoResult nats::Core::handleInfo(std::streambuf& buf) {
    std::istream is(&buf);
    std::string cmd_name;
    is >> cmd_name;
    if (!is || cmd_name != "INFO") {
        return std::unexpected(Error("bad syntax"));
    }

    std::string info_json;
    char ch;
    while (is.get(ch)) {
        info_json += ch;
        if (info_json.size() >= 2 && info_json.substr(info_json.size() - 2) == "\r\n") {
            break;
        }
    }

    if (info_json.size() < 2 || info_json.substr(info_json.size() - 2) != "\r\n") {
        return std::unexpected(Error{"bad syntax"});
    }

    // Remove the trailing \r\n
    info_json.erase(info_json.size() - 2);

    std::string last_key;
    simdjson::dom::parser parser;
    try {
        simdjson::dom::element doc = parser.parse(simdjson::pad(info_json));
        Info info;

        for (auto [key, value] : doc.get_object()) {
            last_key = key;
            if (key == "server_id") {
                info.server_id = std::string_view(value);
            } else if (key == "server_name") {
                info.server_name = std::string_view(value);
            } else if (key == "version") {
                info.version = std::string_view(value);
            } else if (key == "go") {
                info.go = std::string_view(value);
            } else if (key == "host") {
                info.host = std::string_view(value);
            } else if (key == "port") {
                info.port = int64_t(value);
            } else if (key == "headers") {
                info.headers = bool(value);
            } else if (key == "max_payload") {
                info.max_payload = int64_t(value);
            } else if (key == "proto") {
                info.proto = int64_t(value);
            } else if (key == "client_id") {
                info.client_id = uint64_t(value);
            } else if (key == "auth_required") {
                info.auth_required = bool(value);
            } else if (key == "tls_required") {
                info.tls_required = bool(value);
            } else if (key == "tls_verified") {
                info.tls_verified = bool(value);
            } else if (key == "tls_available") {
                info.tls_available = bool(value);
            } else if (key == "connect_urls") {
                std::vector<std::string> urls;
                for (auto url : value.get_array()) {
                    urls.emplace_back(std::string_view(url));
                }
                info.connect_urls = urls;
            } else if (key == "ws_connect_urls") {
                std::vector<std::string> urls;
                for (auto url : value.get_array()) {
                    urls.emplace_back(std::string_view(url));
                }
                info.ws_connect_urls = urls;
            } else if (key == "ldm") {
                info.ldm = bool(value);
            } else if (key == "git_commit") {
                info.git_commit = std::string_view(value);
            } else if (key == "jetstream") {
                info.jetstream = bool(value);
            } else if (key == "ip") {
                info.ip = std::string_view(value);
            } else if (key == "client_ip") {
                info.client_ip = std::string_view(value);
            } else if (key == "nonce") {
                info.nonce = std::string_view(value);
            } else if (key == "cluster") {
                info.cluster = std::string_view(value);
            } else if (key == "domain") {
                info.domain = std::string_view(value);
            }
        }

        return info;
    } catch (simdjson::simdjson_error& error) {
        const std::string detail = "JSON error: " + std::string(error.what()) + " near " + last_key + " in " + info_json;
        return std::unexpected(Error{detail});
    }
}
