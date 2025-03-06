#include "nats/core.h"
#include "nats/operators.h"
#include "nats/stream.h"

#include <boost/asio.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <expected>
struct ExpectedMessageMatcher : Catch::Matchers::MatcherGenericBase {
    ExpectedMessageMatcher(const nats::Message& msg) : expected { msg }
    {}

    bool match(std::expected<nats::OkMessage, nats::Error> const& other) const {
        try {
            return other.has_value() && std::get<nats::Message>(other.value()) == expected;
        } catch (const std::bad_variant_access& ex) {
            return false;
        }
    }

    std::string describe() const override {
        return "Equals: " + to_string(expected);
    }

private:
    const nats::Message expected;
};

auto HasExpectedMessage(const nats::Message& expected) -> ExpectedMessageMatcher {
    return ExpectedMessageMatcher{expected};
}

struct ExpectedNeedMoreDataMatcher : Catch::Matchers::MatcherGenericBase {
    ExpectedNeedMoreDataMatcher(const nats::MessageNeedsMoreData& expected) : expected { expected }
    {}

    bool match(std::expected<nats::OkMessage, nats::Error> const& other) const {
        try {
            return other.has_value() && std::get<nats::MessageNeedsMoreData>(other.value()) == expected;
        } catch (const std::bad_variant_access& ex) {
            return false;
        }
    }

    std::string describe() const override {
        return "Equals: " + to_string(expected);
    }

private:
    const nats::MessageNeedsMoreData expected;
};

auto HasExpectedNeedMoreData(const nats::MessageNeedsMoreData& expected) -> ExpectedNeedMoreDataMatcher {
    return ExpectedNeedMoreDataMatcher{expected};
}
///
struct ExpectedErrorMatcher : Catch::Matchers::MatcherGenericBase {
    ExpectedErrorMatcher(const nats::Error& expected) : expected { expected }
    {}

    template <typename T>
    bool match(std::expected<T, nats::Error> const& other) const {
        return !other.has_value() && other.error() == expected;
    }

    std::string describe() const override {
        return "Equals: " + to_string(expected);
    }

private:
    const nats::Error expected;
};

auto HasExpectedError(const nats::Error& expected) -> ExpectedErrorMatcher {
    return ExpectedErrorMatcher{expected};
}

struct ExpectedInfoMatcher : Catch::Matchers::MatcherGenericBase {
    ExpectedInfoMatcher(const nats::Info& info) : expected { info }
    {}

    bool match(std::expected<nats::Info, nats::Error> const& other) const {
        return other.has_value() && other.value() == expected;
    }

    std::string describe() const override {
        return "Equals: " + to_string(expected);
    }

private:
    const nats::Info expected;
};

auto HasExpectedInfo(const nats::Info& expected) -> ExpectedInfoMatcher {
    return ExpectedInfoMatcher{expected};
}

TEST_CASE( "Complete Text Message", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 3\r\nhi!\r\n";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedMessage(nats::Message{"test.subject", "10", std::nullopt, 3, "hi!"}));
    REQUIRE(buf.size() == 0);
}

TEST_CASE( "Header Continuation", "[message]") {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 ";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
 
}

TEST_CASE( "Payload Continuation", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 3\r\nh";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedNeedMoreData(nats::MessageNeedsMoreData{4, nats::Message{"test.subject", "10", std::nullopt, 3, ""}}));
}

TEST_CASE( "Malformed Bytes", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 text\r\nhi!\r\n";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}

TEST_CASE( "Missing Bytes", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10\r\nhi!\r\n";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}

TEST_CASE( "Missing CR", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 3\nhi!\r\n";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}

TEST_CASE( "Missing LF", "[message]" ) {
    nats::Core core;
    
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "MSG test.subject 10 3\rhi!\r\n";

    const auto result = core.handleMsg(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}

TEST_CASE( "Info", "[info]" ) {
    nats::Core core;

    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "INFO " <<
    R"(
     {
      "server_id":"NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
      "server_name":"us-south-nats-demo",
      "version":"2.10.26",
      "proto":1,
      "git_commit":"14fa949",
      "go":"go1.23.6",
      "host":"0.0.0.0",
      "port":4222,
      "headers":true,
      "tls_available":true,
      "max_payload":1048576,
      "jetstream":true,
      "client_id":70833,
      "client_ip":"2600:4040:5dde:ca00:2438:6bab:ece4:1b16",
      "nonce":"3UwncZtJXThbQKo",
      "xkey":"XAXEF6WRIX65AMNLYAQS34AEAXMDGRRNHR6YHCSAR5PA5GFO7UKHRBFY"
     }
    )" << "\r\n";

    const auto result = core.handleInfo(buf);
    REQUIRE_THAT(result, HasExpectedInfo(nats::Info{
        .server_id="NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
        .server_name="us-south-nats-demo",
        .version="2.10.26",
        .proto=1,
        .git_commit="14fa949",
        .go="go1.23.6",
        .host="0.0.0.0",
        .port=4222,
        .headers=true,
        .tls_available=true,
        .max_payload=1048576,
        .jetstream=true,
        .client_id=70833,
        .client_ip="2600:4040:5dde:ca00:2438:6bab:ece4:1b16",
        .nonce="3UwncZtJXThbQKo"
                }));
}

TEST_CASE( "Arrays", "[info]" ) {
    nats::Core core;

    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "INFO " <<
    R"({
      "server_id":"NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
      "server_name":"us-south-nats-demo",
      "version":"2.10.26",
      "proto":1,
      "git_commit":"14fa949",
      "go":"go1.23.6",
      "host":"0.0.0.0",
      "port":4222,
      "headers":true,
      "tls_available":true,
      "max_payload":1048576,
      "jetstream":true,
      "client_id":70833,
      "client_ip":"2600:4040:5dde:ca00:2438:6bab:ece4:1b16",
      "nonce":"3UwncZtJXThbQKo",
      "xkey":"XAXEF6WRIX65AMNLYAQS34AEAXMDGRRNHR6YHCSAR5PA5GFO7UKHRBFY",
      "connect_urls":["1.2.3.4","1.2.3.5"]})"
      << "\r\n";

    const auto result = core.handleInfo(buf);
    REQUIRE_THAT(result, HasExpectedInfo(nats::Info{
      .server_id="NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
      .server_name="us-south-nats-demo",
      .version="2.10.26",
      .proto=1,
      .git_commit="14fa949",
      .go="go1.23.6",
      .host="0.0.0.0",
      .port=4222,
      .headers=true,
      .tls_available=true,
      .connect_urls=std::optional<std::vector<std::string>>{{"1.2.3.4", "1.2.3.5"}},
      .max_payload=1048576,
      .jetstream=true,
      .client_id=70833,
      .client_ip="2600:4040:5dde:ca00:2438:6bab:ece4:1b16",
      .nonce="3UwncZtJXThbQKo"
            }));
}

TEST_CASE( "Unexpected Connect URLs type", "[info]" ) {
    nats::Core core;

    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "INFO " <<
    R"(
     {
      "server_id":"NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
      "server_name":"us-south-nats-demo",
      "version":"2.10.26",
      "go":"go1.23.6",
      "host":"0.0.0.0",
      "port":4222,
      "headers":true,
      "max_payload":1048576,
      "proto":1,
      "connect_urls":["1.2.3.4",true,10]
     }
    )" << "\r\n";

    const auto result = core.handleInfo(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}

TEST_CASE( "Bad JSON", "[info]" ) {
    nats::Core core;

    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << "INFO " <<
    R"(
     {
      "server_id":"NAGGEW65XCMGPSNNQQF6FGKDGQCCXZFMHDI264FXBLT5MURRVDAAKII3",
      "server_name":"us-south-nats-demo",
      "version":"2.10.26",
      "proto":1,
      "git_commit":"14fa949",
      "go":"go1.23.6",
      "host":"0.0.0.0",
      "port":4222,
      "headers":true,
      "tls_available":true,
      "max_payload":1048576,
      "jetstream":true,
      "client_id":70833,
      "client_ip":"2600:4040:5dde:ca00:2438:6bab:ece4:1b16",
      "nonce":"3UwncZtJXThbQKo",
      "xkey":"XAXEF6WRIX65AMNLYAQS34AEAXMDGRRNHR6YHCSAR5PA5GFO7UKHRBFY",
      "connect_urls:["1.2.3.4","1.2.3.5"]
     }
    )" << "\r\n";

    const auto result = core.handleInfo(buf);
    REQUIRE_THAT(result, HasExpectedError(nats::Error{}));
}
