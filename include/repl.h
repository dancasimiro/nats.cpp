#ifndef REPL_H
#define REPL_H

#include <string>
#include <ostream>
#include <boost/asio.hpp>
#include "logging.h"
#include "nats_client.h"

namespace net = boost::asio;

class REPL {
public:
    REPL(net::io_context& io_context, NATSClient& nats_client);
    ~REPL();
    REPL(const REPL&) = delete;
    REPL& operator=(const REPL&) = delete;
    void start();

    
private:
    void doRead();
    void onRead(const boost::system::error_code& ec, std::size_t length);
    void quit();
    bool evaluate(std::istream& is);


    void print(LogLevel level, const std::string& msg) const;
    void preparePrompt() const;

    boost::asio::io_context& io_context_;
    NATSClient& nats_client_;
    boost::asio::posix::stream_descriptor input_;
    boost::asio::streambuf buffer_;
};

#endif // REPL_H