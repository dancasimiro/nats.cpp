#ifndef NATS_CLIENT_H
#define NATS_CLIENT_H

#include "logging.h"

#include <boost/asio.hpp>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace net = boost::asio;
using tcp = net::ip::tcp;

struct NATSError {
    std::string message;
};

struct NATSInfo {
    std::string server_name;
    std::string server_id;
    std::optional<std::string> nonce;
    std::vector<std::string> connect_urls;
    bool verbose = false;
};

class NATSClient {
public:
    NATSClient(net::io_context& io_context, const std::string& host, const std::string& port);
    NATSClient(const NATSClient&) = delete;
    NATSClient& operator=(const NATSClient&) = delete;
    void start();
    void shutdown();
    void setLogging(const Logger& l) { log_ = l; }

    ///
    /// \begingroup NATS core public client API
    struct Message {
        std::string subject;
        std::string sid;
        std::optional<std::string> replyTo;
        std::size_t bytes = 0;
        std::string payload;
    };

    void pub( const Message& msg);
    void hpub(const std::string& subject);

    struct Subscription {
        std::string subject;
        std::string sid;
        std::optional<std::string> queueGroup;
    };
    typedef std::function<Message(const Message&)> MessageHandler;
    void sub(const Subscription& subscription, const MessageHandler& handler);
    void unsub(const std::string& sid);
    /// \endgroup
    
private:
    void send(const std::string& message);
    void close();

    ///
    /// \begingroup NATS private client API
    void connect(const NATSInfo& info);
    void ping();
    void pong();
    /// \endgroup

    ///
    /// \begingroup handlers for NATS server APIs
    NATSInfo handleInfo(std::istream& is);
    /// @brief
    /// @param is 
    /// @return next operation to perform
    std::function<void()> handleMsg(std::istream& is);
    void handleMsgPayload(const Message& in, std::istream& is);
    /// \endgroup

    // async handlers
    void onConnect(const boost::system::error_code& ec);
    void onWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void doRead();
    void onRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void evaluateLine(const std::string& line);
    std::expected<NATSInfo, NATSError> parseInfo(std::istream& is);

    net::io_context& io_context_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string host_;
    std::string port_;
    boost::asio::streambuf response_;
    Logger log_;

    /// the subscription key is a tuple of the subject and the sid.
    /// maps subscribed sid tuples to message handlers.
    std::unordered_map<std::string, MessageHandler> handlers_;
};

void request(NATSClient& nats_client, const NATSClient::Message& msg, const NATSClient::MessageHandler& handler);
void reply(NATSClient& nats_client, const std::string& subject, const NATSClient::MessageHandler& handler);

#endif // NATS_CLIENT_H
