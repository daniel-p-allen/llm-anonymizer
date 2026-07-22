#include "anonymizer.h"
#include "splashkit.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <regex>
#include "json.hpp"

// Note: I had trouble with the regex library accepting the string type, so I used the std::string type instead

// Set up variables
std::vector<anonymized_pattern> patterns;
const std::string filename = "swapped.json";

// Initialise patterns for anonymization.
//
// Order matters: email addresses are matched first so the digit patterns below
// cannot chew through the numeric parts of an address.
//
// The digit patterns deliberately require either a leading '+', real separators,
// or a long unbroken run. An earlier version made separators optional and
// allowed groups as short as one digit, which meant a bare year such as "2021"
// satisfied it (20|2|1) and was replaced, mangling ordinary prose.
void initialize_patterns() {
    // Email addresses.
    add_pattern("email addresses", "[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,24}");

    // International numbers: an explicit '+' then at least eight digits,
    // optionally broken by spaces, dots or dashes.   e.g. +61 3 9000 1234
    add_pattern("international ", "\\+\\d[\\d .-]{6,}\\d");

    // Separated groups: the separators are required, so plain years and short
    // quantities cannot match.   e.g. 1300 555 010, 555-123-4567, 0412 345 678
    add_pattern("phone numbers ", "\\b\\d{2,4}[ .-]\\d{3,4}[ .-]\\d{3,4}\\b");

    // Long unbroken runs — order and account references. Seven digits is longer
    // than any year, so dates survive.   e.g. 5551234567
    add_pattern("long numbers  ", "\\b\\d{7,}\\b");
}

// Anonymizes the string and maps the original data to the anonymized data.
//
// Each match is replaced individually, and a value that appears more than once
// always receives the same token — including across fields and across emails,
// because the mappings accumulate in `patterns` for the lifetime of the run.
// That consistency is what makes the reverse leg reliable.
std::string anonymize_text(const std::string inputText, placeholder_data& placeholders) {
    std::string working = inputText;

    // Apply each pattern in turn to the result of the previous one.
    for (anonymized_pattern& pattern : patterns) {
        std::regex pattern_regex(pattern.pattern);
        std::string rebuilt;
        std::ptrdiff_t copied_to = 0;

        auto match_iterator = std::sregex_iterator(working.begin(), working.end(), pattern_regex);
        auto match_end = std::sregex_iterator();

        // Walk the matches, copying the untouched text between them and
        // substituting a token for each match.
        for (; match_iterator != match_end; ++match_iterator) {
            std::smatch match = *match_iterator;

            // Text since the previous match passes through unchanged.
            rebuilt.append(working, copied_to, match.position(0) - copied_to);

            const std::string original = match.str();

            // Reuse the token if we have seen this exact value before.
            auto existing = pattern.mapping.find(original);
            if (existing == pattern.mapping.end()) {
                const std::string anonymized = generate_random_replacement(10);
                pattern.mapping[original] = anonymized;
                rebuilt += anonymized;
            } else {
                rebuilt += existing->second;
            }

            copied_to = match.position(0) + match.length(0);
        }

        // Whatever follows the final match.
        rebuilt.append(working, copied_to, std::string::npos);
        working = rebuilt;
    }

    // Rebuild the mapping set from every pattern, then persist it. Doing this
    // from scratch each time keeps swapped.json in step with the accumulated
    // state rather than appending duplicates.
    placeholders.mappings.clear();
    for (const anonymized_pattern& pattern : patterns) {
        if (!pattern.mapping.empty()) {
            placeholders.mappings.push_back(pattern.mapping);
        }
    }
    saveMappingsToJson(placeholders);

    return working;
}

// Generates a random string of the specified length
string generate_random_replacement(int length) {
    string randomReplacement;

    const string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    // Generate a random string of the specified length
    for (int i = 0; i < length; ++i) {
        randomReplacement += characters[rand() % characters.length()];
    }

    return randomReplacement;
}

// Adds a pattern to the patterns vector
void add_pattern(const string& label, const string& pattern) {
    anonymized_pattern newPattern;
    newPattern.label = label;
    newPattern.pattern = pattern;
    patterns.push_back(newPattern);
}

// Clears everything replaced so far, so a second run starts clean.
void reset_patterns() {
    for (anonymized_pattern& pattern : patterns) {
        pattern.mapping.clear();
    }
}

// Prints how many distinct values each pattern replaced.
void print_findings_summary() {
    int total = 0;
    for (const anonymized_pattern& pattern : patterns) {
        total += static_cast<int>(pattern.mapping.size());
    }

    std::cout << "\nFound and replaced " << total << " value(s):" << std::endl;
    for (const anonymized_pattern& pattern : patterns) {
        std::cout << "  " << pattern.label << "  "
                  << pattern.mapping.size() << std::endl;
    }
}

// Saves the mappings to the JSON file
void saveMappingsToJson(const placeholder_data& placeholders) {
    nlohmann::json j;

    j["swapped"] = placeholders.mappings;

    // Open the file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    } else {

        // If the file cannot be opened, print an error message
        std::cerr << "Failed to open file: " << filename << std::endl;
    }

}
