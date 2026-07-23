// Tests for reading and writing the email JSON files.
//
// This is the layer between disk and everything else, so its failure modes are
// the ones a user meets first: a file that is not there, or a field the mail
// provider left null.

#include "doctest.h"
#include "helpers.h"

#include "email_processing.h"

#include <string>
#include <vector>

TEST_CASE("a missing file yields no emails rather than crashing") {
    TempDir dir;

    const std::vector<email_content> emails = read_data_from_json("not_here.json");

    CHECK(emails.empty());
}

TEST_CASE("an empty array yields no emails") {
    TempDir dir;
    write_file("data.json", "[]");

    CHECK(read_data_from_json("data.json").empty());
}

TEST_CASE("all four fields are read") {
    TempDir dir;

    std::vector<TestEmail> input;
    TestEmail email;
    email.from = "alice@example.com";
    email.to = "bob@example.com";
    email.subject = "Invoice";
    email.content = "Please pay by Friday.";
    input.push_back(email);

    write_file("data.json", emails_as_json(input));

    const std::vector<email_content> emails = read_data_from_json("data.json");

    REQUIRE(emails.size() == 1);
    CHECK(emails[0].from == "alice@example.com");
    CHECK(emails[0].to == "bob@example.com");
    CHECK(emails[0].subject == "Invoice");
    CHECK(emails[0].content == "Please pay by Friday.");
}

// A mailbox will happily hand back a message with no subject. Without the
// is_null guard in read_data_from_json this threw rather than returning "".
TEST_CASE("null fields become empty strings") {
    TempDir dir;

    write_file("data.json",
               "[{\"from\":null,\"to\":null,\"subject\":null,\"content\":\"body only\"}]");

    const std::vector<email_content> emails = read_data_from_json("data.json");

    REQUIRE(emails.size() == 1);
    CHECK(emails[0].from == "");
    CHECK(emails[0].to == "");
    CHECK(emails[0].subject == "");
    CHECK(emails[0].content == "body only");
}

TEST_CASE("saving and reading back preserves every field") {
    TempDir dir;

    std::vector<email_content> written;
    email_content email;
    email.from = "alice@example.com";
    email.to = "bob@example.com";
    email.subject = "Re: lunch";
    email.content = "Line one.\nLine two, with a comma.";
    written.push_back(email);

    save_data_to_json("anonymized_data.json", written);
    const std::vector<email_content> read_back = read_data_from_json("anonymized_data.json");

    REQUIRE(read_back.size() == 1);
    CHECK(read_back[0].from == written[0].from);
    CHECK(read_back[0].to == written[0].to);
    CHECK(read_back[0].subject == written[0].subject);
    CHECK(read_back[0].content == written[0].content);
}

TEST_CASE("non-ASCII content survives being written and read") {
    TempDir dir;

    std::vector<email_content> written;
    email_content email;
    email.from = "renée@example.com";
    email.to = "bob@example.com";
    email.subject = "Café meeting — 15:00";
    email.content = "See you there. £20 covers it.";
    written.push_back(email);

    save_data_to_json("out.json", written);
    const std::vector<email_content> read_back = read_data_from_json("out.json");

    REQUIRE(read_back.size() == 1);
    CHECK(read_back[0].from == written[0].from);
    CHECK(read_back[0].subject == written[0].subject);
    CHECK(read_back[0].content == written[0].content);
}

TEST_CASE("several emails keep their order") {
    TempDir dir;

    std::vector<email_content> written;
    for (int i = 0; i < 3; ++i) {
        email_content email;
        email.from = "sender" + std::to_string(i) + "@example.com";
        email.to = "bob@example.com";
        email.subject = "Message " + std::to_string(i);
        email.content = "Body " + std::to_string(i);
        written.push_back(email);
    }

    save_data_to_json("out.json", written);
    const std::vector<email_content> read_back = read_data_from_json("out.json");

    REQUIRE(read_back.size() == 3);
    for (int i = 0; i < 3; ++i) {
        CHECK(read_back[i].subject == "Message " + std::to_string(i));
    }
}
