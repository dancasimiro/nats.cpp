#ifndef NATS_CLIENT_H
#define NATS_CLIENT_H

#include "logging.h"
#include "core.h"

#include <boost/asio.hpp>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace net = boost::asio;
using tcp = net::ip::tcp;
using nats::Message;
using nats::MessageResult;
using nats::Core;
typedef nats::Info NATSInfo;

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

    /// returns false on success
    typedef std::variant<nats::Ok, nats::MessageNeedsMoreData> ResponseSuccessType;
    typedef std::expected<ResponseSuccessType, nats::Error> RespResult;
    RespResult evalResponse();

    ///
    /// \begingroup handlers for NATS server APIs
    nats::Error handleErr();
    nats::Ok handleOk();
    void handleInfo();
    RespResult handleMsg();
    void handlePing();
    /// @brief
    /// @param is 
    /// @return next operation to perform
    Message handleMsgPayload(const Message& msg);
    /// \endgroup

    // async handlers
    void onConnect(const boost::system::error_code& ec);
    void onWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void doRead();
    void onRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void onCompleteMsg(const boost::system::error_code& ec, std::size_t bytes_transferred, nats::MessageNeedsMoreData&& nmd);

    net::io_context& io_context_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string host_;
    std::string port_;
    boost::asio::streambuf response_;
    Core core_;
    Logger log_;

    /// the subscription key is a tuple of the subject and the sid.
    /// maps subscribed sid tuples to message handlers.
    std::unordered_map<std::string, MessageHandler> handlers_;
};

void request(NATSClient& nats_client, const Message& msg, const NATSClient::MessageHandler& handler);
void reply(NATSClient& nats_client, const std::string& subject, const NATSClient::MessageHandler& handler);

#endif // NATS_CLIENT_H
