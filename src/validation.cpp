
#include "validation.h"
// #include "splashkit.h"
#include "email_processing.h"
#include "anonymizer.h"
#include <vector>
#include <iostream>

// This outputs the data to the console and processes the data.
// placeholders is taken by reference so the mappings built here are visible to
// the caller — previously it was taken by value and every mapping was discarded
// when the function returned.
bool validate_email_data(placeholder_data& placeholders, const std::string& source_file) {

    // Read the data from the JSON file and store it in a vector
    vector<email_content> emailData = read_data_from_json(source_file);

    // Nothing to do if the file was missing or empty
    if (emailData.empty()) {
        cerr << "No emails found in " << source_file << "." << endl;
        return false;
    }

    // Start from a clean slate so a second run does not inherit the first
    reset_patterns();

    // Continue with anonymization and other operations
    for (email_content &email : emailData) {

        // Anonymize all fields using the initialized patterns
        email.from = anonymize_text(email.from, placeholders);
        email.to = anonymize_text(email.to, placeholders);
        email.subject = anonymize_text(email.subject, placeholders);
        email.content = anonymize_text(email.content, placeholders);
    }

    // Save the data to a JSON file
    save_data_to_json("anonymized_data.json", emailData);

    // Report what was found rather than dumping every field to the console
    cout << "\nScanned " << emailData.size() << " email(s) from " << source_file << "." << endl;
    print_findings_summary();
    cout << "\nWrote anonymized_data.json  (safe to share)" << endl;
    cout << "Wrote swapped.json          (the mapping — keep this private)" << endl;

    return true;
}
