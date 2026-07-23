// Tests for the forward leg: pattern matching, token generation, and the
// consistency guarantee that the reverse leg depends on.
//
// Several of these pin defects that were found and fixed in this file. Where that
// is the case the test says so, because the comment in anonymizer.cpp explaining
// what went wrong is the reason the test exists.

#include "doctest.h"
#include "helpers.h"

#include "anonymizer.h"

#include <set>
#include <string>

namespace {

// Look up the token a value was replaced with, across every pattern's mapping.
// Returns an empty string if the value was never replaced.
std::string token_for(const placeholder_data& placeholders, const std::string& original) {
    for (const std::map<std::string, std::string>& mapping : placeholders.mappings) {
        std::map<std::string, std::string>::const_iterator found = mapping.find(original);
        if (found != mapping.end()) {
            return found->second;
        }
    }
    return "";
}

// Tokens are the shape revert.cpp looks for: exactly ten alphanumeric characters.
bool looks_like_token(const std::string& value) {
    if (value.size() != 10) return false;
    for (size_t i = 0; i < value.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(value[i]))) return false;
    }
    return true;
}

// Every test starts from a clean pattern list inside its own directory, because
// anonymize_text writes swapped.json to the working directory on every call.
struct Fixture {
    TempDir dir;
    placeholder_data placeholders;

    Fixture() { initialize_patterns(); }
};

} // namespace

TEST_CASE("email addresses are replaced") {
    Fixture f;

    const std::string result = anonymize_text("Contact alice@example.com today", f.placeholders);

    CHECK(result.find("alice@example.com") == std::string::npos);
    CHECK(looks_like_token(token_for(f.placeholders, "alice@example.com")));
}

TEST_CASE("international numbers are replaced") {
    Fixture f;

    const std::string result = anonymize_text("Ring +61 3 9000 1234 now", f.placeholders);

    CHECK(result.find("+61 3 9000 1234") == std::string::npos);
    CHECK(looks_like_token(token_for(f.placeholders, "+61 3 9000 1234")));
}

TEST_CASE("separated digit groups are replaced") {
    Fixture f;

    SUBCASE("spaces") {
        anonymize_text("call 1300 555 010", f.placeholders);
        CHECK(looks_like_token(token_for(f.placeholders, "1300 555 010")));
    }

    SUBCASE("dashes") {
        anonymize_text("call 555-123-4567", f.placeholders);
        CHECK(looks_like_token(token_for(f.placeholders, "555-123-4567")));
    }

    SUBCASE("dots") {
        anonymize_text("call 0412.345.678", f.placeholders);
        CHECK(looks_like_token(token_for(f.placeholders, "0412.345.678")));
    }
}

TEST_CASE("long unbroken digit runs are replaced") {
    Fixture f;

    anonymize_text("order 5551234567 shipped", f.placeholders);

    CHECK(looks_like_token(token_for(f.placeholders, "5551234567")));
}

// Regression. An earlier version made the separators optional and allowed groups
// as short as one digit, so a bare year satisfied the phone pattern as 20|2|1 and
// was replaced — mangling ordinary prose.
TEST_CASE("a bare year is left alone") {
    Fixture f;

    const std::string prose = "In 2021 we shipped 42 units.";
    const std::string result = anonymize_text(prose, f.placeholders);

    CHECK(result == prose);
    CHECK(f.placeholders.mappings.empty());
}

TEST_CASE("short quantities and unseparated runs are left alone") {
    Fixture f;

    const std::string prose = "Order 12 boxes, 999 bags and 4000 labels.";
    const std::string result = anonymize_text(prose, f.placeholders);

    CHECK(result == prose);
}

// Regression. The email pattern is registered first so the digit patterns cannot
// chew through the numeric part of an address. Registered the other way round,
// "1234567" inside this address would be replaced on its own and the address
// would come out as a mixture of token and real text.
TEST_CASE("an address containing a long digit run is replaced whole") {
    Fixture f;

    const std::string result = anonymize_text("mail a1234567@example.com", f.placeholders);

    CHECK(result.find("1234567") == std::string::npos);
    CHECK(looks_like_token(token_for(f.placeholders, "a1234567@example.com")));
}

TEST_CASE("a repeated value always gets the same token") {
    Fixture f;

    SUBCASE("within one string") {
        const std::string result =
            anonymize_text("from alice@example.com to alice@example.com", f.placeholders);

        const std::string token = token_for(f.placeholders, "alice@example.com");
        REQUIRE(looks_like_token(token));

        // Both occurrences became the same token, so it appears twice.
        const size_t first = result.find(token);
        REQUIRE(first != std::string::npos);
        CHECK(result.find(token, first + 1) != std::string::npos);
    }

    // This is what makes the reverse leg reliable: mappings accumulate for the
    // lifetime of the run, so a value seen in one field is restored in every other.
    SUBCASE("across separate calls, as different fields and emails are processed") {
        anonymize_text("alice@example.com", f.placeholders);
        const std::string first_token = token_for(f.placeholders, "alice@example.com");

        const std::string second = anonymize_text("reply to alice@example.com", f.placeholders);

        CHECK(token_for(f.placeholders, "alice@example.com") == first_token);
        CHECK(second.find(first_token) != std::string::npos);
    }
}

TEST_CASE("distinct values get distinct tokens") {
    Fixture f;

    anonymize_text("alice@example.com bob@example.com carol@example.com", f.placeholders);

    std::set<std::string> tokens;
    tokens.insert(token_for(f.placeholders, "alice@example.com"));
    tokens.insert(token_for(f.placeholders, "bob@example.com"));
    tokens.insert(token_for(f.placeholders, "carol@example.com"));

    CHECK(tokens.size() == 3);
    CHECK(tokens.count("") == 0);
}

TEST_CASE("text around a match passes through byte for byte") {
    Fixture f;

    const std::string result = anonymize_text("Call +61 3 9000 1234, please.", f.placeholders);
    const std::string token = token_for(f.placeholders, "+61 3 9000 1234");
    REQUIRE(looks_like_token(token));

    CHECK(result == "Call " + token + ", please.");
}

TEST_CASE("text with nothing sensitive is returned unchanged") {
    Fixture f;

    const std::string prose = "The meeting is on Tuesday in the usual room.";

    CHECK(anonymize_text(prose, f.placeholders) == prose);
    CHECK(f.placeholders.mappings.empty());
}

TEST_CASE("an empty string is handled") {
    Fixture f;

    CHECK(anonymize_text("", f.placeholders) == "");
}

TEST_CASE("the mapping is written to swapped.json") {
    Fixture f;

    anonymize_text("alice@example.com", f.placeholders);

    const std::string written = read_file("swapped.json");
    const std::string token = token_for(f.placeholders, "alice@example.com");

    REQUIRE_FALSE(written.empty());
    CHECK(written.find("alice@example.com") != std::string::npos);
    CHECK(written.find(token) != std::string::npos);
}

TEST_CASE("reset_patterns clears what has been replaced so far") {
    Fixture f;

    anonymize_text("alice@example.com", f.placeholders);
    REQUIRE_FALSE(token_for(f.placeholders, "alice@example.com").empty());

    reset_patterns();

    // The accumulated mappings are gone, so the same value is treated as new.
    placeholder_data fresh;
    anonymize_text("alice@example.com", fresh);
    CHECK(fresh.mappings.size() == 1);
}

// Regression. add_pattern appends to a list that lives for the life of the
// process, and reset_patterns clears the mappings but not the list itself.
// Without a clear at the top of initialize_patterns, calling it twice left two
// copies of all four patterns behind and the findings summary reported each of
// them twice.
TEST_CASE("initialize_patterns can be called more than once") {
    Fixture f; // has already called it once

    initialize_patterns();

    anonymize_text("alice@example.com", f.placeholders);

    CaptureCout captured;
    print_findings_summary();

    // Four patterns are registered, so the summary lists four and no more.
    CHECK(count_lines_containing(captured.text(), "email addresses") == 1);
    CHECK(count_lines_containing(captured.text(), "phone numbers") == 1);
}

TEST_CASE("the findings summary counts what was replaced") {
    Fixture f;

    anonymize_text("alice@example.com and bob@example.com", f.placeholders);

    CaptureCout captured;
    print_findings_summary();

    CHECK(captured.text().find("Found and replaced 2 value(s)") != std::string::npos);
}
