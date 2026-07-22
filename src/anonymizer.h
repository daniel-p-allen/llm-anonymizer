#ifndef ANONYMIZER_H
#define ANONYMIZER_H

// anonymizer — the forward leg of the round trip.
//
// Detects sensitive values in text, replaces each with a random 10-character
// token, and records the original -> token mapping so the substitution can be
// undone later by revert.cpp.
//
// The mapping is written to swapped.json and never leaves the machine.

#include "splashkit.h"
#include <string>
#include <vector>
#include <map>
using namespace std;

// Struct to represent placeholder data
struct placeholder_data {
    vector<std::string> anon_content;
    vector<map<std::string, std::string>> mappings; // Store original -> anonymized mappings
};

// Struct to represent a pattern
struct anonymized_pattern {
    string label;                // Human-readable name, used in the findings summary
    string pattern;
    map<string, string> mapping; // Store original -> anonymized mapping
};

// Initialize the patterns
void initialize_patterns();

// Anonymizes the string and maps the original data to the anonymized data
std::string anonymize_text(const std::string inputText, placeholder_data& placeholders);

// Generates a random replacement string
string generate_random_replacement(int length);

// Adds a pattern to the patterns vector
void add_pattern(const string& label, const string& pattern);

// Saves the mappings to a JSON file
void saveMappingsToJson(const placeholder_data& placeholders);

// Prints a per-pattern count of everything replaced so far
void print_findings_summary();

// Clears all accumulated mappings (used between runs)
void reset_patterns();

#endif // ANONYMIZER_H
