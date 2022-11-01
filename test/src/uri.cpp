#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ekisocket/Uri.hpp>

using ekisocket::http::Uri;

TEST_CASE("complete_uri", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host:81/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("non_normalized_uri", "[uri]")
{
    const auto uri = Uri::parse("ScheMe://user:pass@HoSt:81/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_scheme", "[uri]")
{
    const auto uri = Uri::parse("//user:pass@HoSt:81/path?query#fragment");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_path", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host:81");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("uri_without_query", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host:81/path#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_fragment", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host:81/path?query");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "");
}

TEST_CASE("uri_without_userinfo", "[uri]")
{
    const auto uri = Uri::parse("scheme://host:81/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_password", "[uri]")
{
    const auto uri = Uri::parse("scheme://user@host:81/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == 81);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_port", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_with_an_empty_port", "[uri]")
{
    const auto uri = Uri::parse("scheme://user:pass@host:/path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "pass");
    REQUIRE(uri.host == "host");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_with_host_ipv4", "[uri]")
{
    const auto uri = Uri::parse("scheme://192.168.0.1/p?q#f");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "192.168.0.1");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/p");
    REQUIRE(uri.query.at("q") == "");
    REQUIRE(uri.fragment == "f");
}

TEST_CASE("uri_with_host_ipv6", "[uri]")
{
    const auto uri = Uri::parse("scheme://[2001:db8::1]/p?q#f");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "2001:db8::1");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/p");
    REQUIRE(uri.query.at("q") == "");
    REQUIRE(uri.fragment == "f");
}

TEST_CASE("uri_without_authority", "[uri]")
{
    const auto uri = Uri::parse("scheme:path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_scheme_and_authority", "[uri]")
{
    const auto uri = Uri::parse("/path");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("uri_with_empty_host", "[uri]")
{
    const auto uri = Uri::parse("scheme:///path?query#fragment");

    REQUIRE(uri.scheme == "scheme");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("uri_without_scheme_and_empty_host", "[uri]")
{
    const auto uri = Uri::parse("///path?query#fragment");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query.at("query") == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("complete_uri_without_scheme", "[uri]")
{
    const auto uri = Uri::parse("//user@[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:42?q#f");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "user");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "fedc:ba98:7654:3210:fedc:ba98:7654:3210");
    REQUIRE(uri.port == 42);
    REQUIRE(uri.path == "");
    REQUIRE(uri.query.at("q") == "");
    REQUIRE(uri.fragment == "f");
}

TEST_CASE("single_word_is_path", "[uri]")
{
    const auto uri = Uri::parse("path");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "path");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("single_word_is_path_with_scheme", "[uri]")
{
    const auto uri = Uri::parse("http:::/path");

    REQUIRE(uri.scheme == "http");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "::/path");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("fragment_with_pseudo_segment", "[uri]")
{
    const auto uri = Uri::parse("http://example.com#foo=1/bar=2");

    REQUIRE(uri.scheme == "http");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "example.com");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "foo=1/bar=2");
}

TEST_CASE("empty_string", "[uri]")
{
    const auto uri = Uri::parse("");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("complex_uri", "[uri]")
{
    const auto uri = Uri::parse("htà+d/s:totot");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "htà+d/s:totot");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("scheme_only_uri", "[uri]")
{
    const auto uri = Uri::parse("http:");

    REQUIRE(uri.scheme == "http");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("ldap_example_uri", "[uri]")
{
    const auto uri = Uri::parse("ldap://[2001:db8::7]/c=GB?objectClass?one");

    REQUIRE(uri.scheme == "ldap");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "2001:db8::7");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/c=GB");
    REQUIRE(uri.query.at("objectClass?one") == "");
    REQUIRE(uri.fragment == "");
}

TEST_CASE("rfc_3987_example", "[uri]")
{
    const auto uri = Uri::parse("http://bébé.bé./有词法别名.zh");

    REQUIRE(uri.scheme == "http");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "bébé.bé.");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/有词法别名.zh");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("colon_detection_respect", "[uri]")
{
    const auto uri = Uri::parse("http://example.org/hello:12?foo=bar#test");

    REQUIRE(uri.scheme == "http");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "example.org");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/hello:12");
    REQUIRE(uri.query.at("foo") == "bar");
    REQUIRE(uri.fragment == "test");
}

TEST_CASE("colon_detection_respect_2", "[uri]")
{
    const auto uri = Uri::parse("/path/to/colon:34");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/path/to/colon:34");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("scheme_with_hypher", "[uri]")
{
    const auto uri
        = Uri::parse("android-app://org.wikipedia/http/en.m.wikipedia.org/wiki/The_Hitchhiker%27s_Guide_to_the_Galaxy");

    REQUIRE(uri.scheme == "android-app");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "org.wikipedia");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/http/en.m.wikipedia.org/wiki/The_Hitchhiker%27s_Guide_to_the_Galaxy");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("uri_with_absolute_path", "[uri]")
{
    const auto uri = Uri::parse("/?#");

    REQUIRE(uri.scheme == "");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "");
}

TEST_CASE("uri_with_absolute_authority", "[uri]")
{
    const auto uri = Uri::parse("https://thephpleague.com./p?#f");

    REQUIRE(uri.scheme == "https");
    REQUIRE(uri.username == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "thephpleague.com.");
    REQUIRE(uri.port == std::nullopt);
    REQUIRE(uri.path == "/p");
    REQUIRE(uri.query.empty());
    REQUIRE(uri.fragment == "f");
}

int main(int argc, char* argv[]) { return Catch::Session().run(argc, argv); }
