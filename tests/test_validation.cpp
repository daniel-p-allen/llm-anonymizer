// Tests for validate_email_data — the step that ties reading, anonymizing and
// writing together, and the one the menu actually calls.

#include "doctest.h"
#include "helpers.h"

#include "anonymizer.h"
#include "email_processing.h"
#include "validation.h"

#include <string>
#include <vector>

namespace {

struct Fixture {
    TempDir dir;
    placeholder_data placeholders;

    Fixture() { initialize_patterns(); }
};

// validate_email_data reports what it found on the console, which is right for a
// user and noise for a test run. The capture is released before the caller
// asserts on the result, so a doctest failure message is never swallowed.
bool validate_quietly(placeholder_data& placeholders, const std::string& source) {
    CaptureCout quiet;
    return validate_email_data(placeholders, source);
}

// Write a small source file and hand back its name.
std::string given_source_file(const std::string& name) {
    std::vector<TestEmail> input;

    TestEmail first;
    first.from = "alice@example.com";
    first.to = "bob@example.com";
    first.subject = "Invoice";
    first.content = "Ring 1300 555 010 to confirm.";
    input.push_back(first);

    TestEmail second;
    second.from = "carol@example.com";
    second.to = "bob@example.com";
    second.subject = "Re: Invoice";
    second.content = "Nothing sensitive here.";
    input.push_back(second);

    write_file(name, emails_as_json(input));
    return name;
}

} // namespace

// The documented contract: false means the caller should stop rather than carry
// on with whatever data.json happened to be left over from a previous run.
TEST_CASE("a missing source file is refused") {
    Fixture f;

    CHECK_FALSE(validate_quietly(f.placeholders, "not_here.json"));
}

TEST_CASE("a source file with no emails is refused") {
    Fixture f;
    write_file("empty.json", "[]");

    CHECK_FALSE(validate_quietly(f.placeholders, "empty.json"));
}

TEST_CASE("a readable source file is accepted") {
    Fixture f;

    CHECK(validate_quietly(f.placeholders, given_source_file("data.json")));
}

// Regression. placeholders was previously taken by value, so every mapping built
// inside this function was discarded when it returned — leaving the caller with
// nothing to restore from.
TEST_CASE("the mappings it builds are visible to the caller") {
    Fixture f;

    REQUIRE(validate_quietly(f.placeholders, given_source_file("data.json")));

    CHECK_FALSE(f.placeholders.mappings.empty());
}

TEST_CASE("it writes the anonymized file") {
    Fixture f;

    REQUIRE(validate_quietly(f.placeholders, given_source_file("data.json")));

    CHECK(file_exists("anonymized_data.json"));
}

TEST_CASE("the anonymized file holds no original addresses") {
    Fixture f;

    REQUIRE(validate_quietly(f.placeholders, given_source_file("data.json")));

    const std::string written = read_file("anonymized_data.json");

    CHECK(written.find("alice@example.com") == std::string::npos);
    CHECK(written.find("carol@example.com") == std::string::npos);
    CHECK(written.find("1300 555 010") == std::string::npos);
}

TEST_CASE("every email is kept, including ones with nothing to replace") {
    Fixture f;

    REQUIRE(validate_quietly(f.placeholders, given_source_file("data.json")));

    const std::vector<email_content> written = read_data_from_json("anonymized_data.json");

    REQUIRE(written.size() == 2);
    CHECK(written[1].content == "Nothing sensitive here.");
}

// A second run must not inherit the first's mappings, which is why
// validate_email_data resets before it starts.
TEST_CASE("a second run starts from a clean slate") {
    Fixture f;
    given_source_file("data.json");

    REQUIRE(validate_quietly(f.placeholders, "data.json"));
    const size_t first_run = f.placeholders.mappings.size();

    placeholder_data second_placeholders;
    REQUIRE(validate_quietly(second_placeholders, "data.json"));

    CHECK(second_placeholders.mappings.size() == first_run);
}
