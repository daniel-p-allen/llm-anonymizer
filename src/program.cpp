
#include "email_logic.h"
#include "gmail_connector.h"
#include "validation.h"
#include "revert.h"
#include "gui.h"
#include "anonymizer.h"
#include "email_processing.h"
#include <vector>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include "splashkit.h"

// Data Anonymizer V 1.0
//
// Lets you use an LLM on real email without handing over the contents.
//
//   1. Read emails into JSON (from Gmail, or the bundled sample file)
//   2. Replace every email address and phone number with a random token,
//      recording the original -> token mapping in swapped.json
//   3. Show the scrubbed version in a window so you can check it by eye
//   4. You paste that into ChatGPT and save its reply as finished_data.csv
//   5. Reload the mapping and put the real values back, giving output_data.csv
//
// The mapping never leaves this machine, so the model only ever sees tokens
// while the final result still contains the real details.

const std::string SAMPLE_FILE = "sample_data.json";
const std::string LIVE_FILE   = "data.json";

// Shown after anonymizing, because the next step happens outside this program
// and is easy to guess wrong.
void print_next_steps() {
    write_line("");
    write_line("Next:");
    write_line("  1. Open anonymized_data.json and paste it into ChatGPT.");
    write_line("  2. Ask for whatever you need, and save the reply as finished_data.csv.");
    write_line("  3. Run this program again and choose 3 to put your real details back.");
}

// Anonymize a JSON file, then have the result reviewed before it can be shared.
void anonymize_and_review(placeholder_data& placeholders, const std::string& source, bool show_gui) {
    if (!validate_email_data(placeholders, source)) {
        return;
    }

    if (!show_gui) {
        // --demo skips the review window so it can run in a plain terminal.
        write_line("");
        write_line("(Demo mode: the review window was skipped. Run ./anonymizer to use it.)");
        print_next_steps();
        return;
    }

    GUI myGui;
    init_gui(myGui, "gui_config.json");

    std::vector<review_result> decisions;
    run_gui(myGui, decisions);

    // Rewrite the file so it holds only what was actually approved. Anything
    // rejected or left undecided is withheld — the file on disk should never
    // contain something the reviewer did not agree to share.
    std::vector<email_content> reviewed = read_data_from_json("anonymized_data.json");
    std::vector<email_content> approved;

    for (size_t i = 0; i < reviewed.size() && i < decisions.size(); ++i) {
        if (decisions[i] == REVIEW_APPROVED) {
            approved.push_back(reviewed[i]);
        }
    }

    const int withheld = static_cast<int>(reviewed.size()) - static_cast<int>(approved.size());

    write_line("");
    if (approved.empty()) {
        remove("anonymized_data.json");
        write_line("Nothing was approved, so anonymized_data.json has been deleted.");
        write_line("The mapping in swapped.json was kept, so an earlier reply can still be restored.");
        return;
    }

    save_data_to_json("anonymized_data.json", approved);

    write_line("Approved " + std::to_string(approved.size()) + " of " +
               std::to_string(reviewed.size()) + " email(s).");
    if (withheld > 0) {
        write_line(std::to_string(withheld) + " withheld — anonymized_data.json contains only the approved ones.");
    }

    print_next_steps();
}

// This is the menu that will be displayed to the user
void menu(placeholder_data& placeholders) {

    int choice;

    // Ordered to match the workflow: try it, run it for real, then restore.
    write_line("Menu:");
    write_line("");
    write_line("  1. Anonymize the sample emails  (demo - no setup needed)");
    write_line("  2. Anonymize my Gmail           (needs secret.json)");
    write_line("  3. Restore an LLM reply         (needs finished_data.csv)");
    write_line("  4. Exit");
    write_line("");

    choice = convert_to_integer(read_line());

    // Demo run against the bundled sample data
    if (choice == 1) {
        anonymize_and_review(placeholders, SAMPLE_FILE, true);
    }

    // Live run against the user's mailbox
    else if (choice == 2) {
        write_line("How many emails do you want to process?");
        int num_emails = convert_to_integer(read_line());

        // Stop here if retrieval failed, rather than silently anonymizing
        // whatever data.json was left behind by a previous run.
        if (!EmailLogic::processEmails(num_emails)) {
            return;
        }

        anonymize_and_review(placeholders, LIVE_FILE, true);
    }

    // Reverse leg: put the real values back into the LLM's reply
    else if (choice == 3) {

        // Load the mappings from the JSON file
        loadMappingsFromJson(placeholders);

        // Process the CSV data and write to a new CSV file
        processCSVWithMapping("finished_data.csv", "output_data.csv", placeholders.mappings);
    }

    else {
        write_line("Exiting program.");
    }
}

// Main function starts
int main(int argc, char* argv[]) {

    // Tokens should differ between runs. Without seeding, rand() produces the
    // same sequence every time the program starts.
    srand(static_cast<unsigned>(time(nullptr)));

    // Create a placeholder_data object to store the mappings
    placeholder_data placeholders;

    // Initialize the patterns used for anonymization
    initialize_patterns();

    // --demo runs the sample round trip without opening a window, so it works
    // over a terminal session and from `make demo`.
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        anonymize_and_review(placeholders, SAMPLE_FILE, false);
        return 0;
    }

    // Load the resources (fonts for the review window)
    load_resources();

    // Menu for user to choose what to do
    menu(placeholders);

    // program finished message
    cout << "Program finished." << endl;
    return 0;
}
