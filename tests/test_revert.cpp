// Tests for the reverse leg, and for the round trip as a whole.
//
// The round trip is the product's actual promise: what comes back after
// restoring must be exactly what went in, or the tool has quietly corrupted
// someone's mail. That is the test worth having above all the others here.

#include "doctest.h"
#include "helpers.h"

#include "anonymizer.h"
#include "revert.h"

#include <string>

namespace {

// Run the full circuit: anonymize, then reload the mapping from disk and restore.
// Deliberately goes through swapped.json rather than reusing the in-memory
// mappings, because that file is the real handover between the two legs.
//
// The console output is captured to keep the test run readable. The capture is
// released before the caller asserts, so a doctest failure message is never
// swallowed by it.
std::string round_trip(const std::string& original, placeholder_data& forward) {
    CaptureCout quiet;

    const std::string anonymized = anonymize_text(original, forward);

    write_file("finished_data.csv", anonymized + "\n");

    placeholder_data loaded;
    loadMappingsFromJson(loaded);
    processCSVWithMapping("finished_data.csv", "output_data.csv", loaded.mappings);

    return read_file("output_data.csv");
}

// Restore an already-written reply, reporting nothing to the console.
void restore_quietly(const std::vector<std::map<std::string, std::string> >& mappings) {
    CaptureCout quiet;
    processCSVWithMapping("finished_data.csv", "output_data.csv", mappings);
}

struct Fixture {
    TempDir dir;
    placeholder_data placeholders;

    Fixture() { initialize_patterns(); }
};

} // namespace

TEST_CASE("a value survives the round trip unchanged") {
    Fixture f;

    const std::string original = "Email alice@example.com or ring +61 3 9000 1234 today";

    CHECK(round_trip(original, f.placeholders) == original + "\n");
}

// Regression. An earlier version split each line on commas and stripped
// non-alphanumeric characters from every word, which destroyed punctuation and
// fragmented any message body containing a comma.
TEST_CASE("punctuation survives the round trip") {
    Fixture f;

    const std::string original =
        "Hi, Bob - mail alice@example.com, then call 1300 555 010. Thanks!";

    CHECK(round_trip(original, f.placeholders) == original + "\n");
}

TEST_CASE("multiple lines keep their structure") {
    Fixture f;

    const std::string original =
        "From: alice@example.com\nTo: bob@example.com\nCall 1300 555 010.";

    CHECK(round_trip(original, f.placeholders) == original + "\n");
}

// Regression. Tokens are recognised by shape — ten alphanumeric characters — so
// ordinary English words of that length are candidates too. They must be passed
// through rather than substituted or mangled.
TEST_CASE("ordinary ten-character words are left alone") {
    Fixture f;

    const std::string original =
        "everything in the background stayed put, mail alice@example.com";

    const std::string restored = round_trip(original, f.placeholders);

    CHECK(restored == original + "\n");
    CHECK(restored.find("everything") != std::string::npos);
    CHECK(restored.find("background") != std::string::npos);
}

TEST_CASE("text with nothing to restore is copied through") {
    Fixture f;

    // A mapping exists, but this reply happens to contain none of its tokens.
    anonymize_text("alice@example.com", f.placeholders);

    const std::string reply = "No contact details were mentioned at all.";
    write_file("finished_data.csv", reply + "\n");

    placeholder_data loaded;
    loadMappingsFromJson(loaded);
    restore_quietly(loaded.mappings);

    CHECK(read_file("output_data.csv") == reply + "\n");
}

TEST_CASE("loadMappingsFromJson inverts the mapping for lookup by token") {
    Fixture f;

    anonymize_text("alice@example.com", f.placeholders);

    // On disk it is stored original -> token. Restoring needs the opposite.
    std::string token;
    for (size_t i = 0; i < f.placeholders.mappings.size(); ++i) {
        std::map<std::string, std::string>::const_iterator found =
            f.placeholders.mappings[i].find("alice@example.com");
        if (found != f.placeholders.mappings[i].end()) token = found->second;
    }
    REQUIRE_FALSE(token.empty());

    placeholder_data loaded;
    loadMappingsFromJson(loaded);

    REQUIRE_FALSE(loaded.mappings.empty());
    CHECK(loaded.mappings[0].count(token) == 1);
    CHECK(loaded.mappings[0].at(token) == "alice@example.com");
}

TEST_CASE("a missing mapping file is reported rather than crashing") {
    TempDir dir; // no swapped.json here

    placeholder_data loaded;
    loadMappingsFromJson(loaded);

    CHECK(loaded.mappings.empty());
}

TEST_CASE("a missing input file leaves no output behind") {
    TempDir dir;

    std::vector<std::map<std::string, std::string> > mappings;
    processCSVWithMapping("finished_data.csv", "output_data.csv", mappings);

    CHECK_FALSE(file_exists("output_data.csv"));
}

TEST_CASE("an empty reply produces an empty result") {
    Fixture f;

    anonymize_text("alice@example.com", f.placeholders);
    write_file("finished_data.csv", "");

    placeholder_data loaded;
    loadMappingsFromJson(loaded);
    restore_quietly(loaded.mappings);

    CHECK(read_file("output_data.csv") == "");
}
