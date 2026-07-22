// revert — the reverse leg of the round trip.
//
// Reloads the original -> token mapping written by anonymizer.cpp, then walks
// the LLM's reply and puts the real values back wherever a token appears.
//
// Everything that is not a token is copied through byte for byte. An earlier
// version split each line on commas and stripped non-alphanumeric characters
// from every word, which destroyed punctuation and fragmented any message body
// that contained a comma. Because tokens are a fixed, recognisable shape, we can
// simply find them in place and leave the rest of the text alone.

#include "revert.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <regex>

using json2 = nlohmann::json;

// The filename of the JSON file containing the mappings
const std::string filename = "swapped.json";

// Function to load mappings from JSON
void loadMappingsFromJson(placeholder_data& placeholders) {

    // Open the JSON file
    std::ifstream file(filename);

    // If the file is open, read the JSON data
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        file.close();

        // If the JSON data contains a "swapped" key, read the mappings
        if (j.find("swapped") != j.end()) {
            auto swapped = j["swapped"];

            // Iterate through each mapping and add it to the placeholder data.
            // Note the inversion: on disk it is stored original -> token, but
            // for reversal we need to look up token -> original.
            for (const auto& mapping : swapped) {
                std::map<std::string, std::string> reversedMapping;
                for (auto it = mapping.begin(); it != mapping.end(); ++it) {
                    reversedMapping[it.value()] = it.key();
                }
                placeholders.mappings.push_back(reversedMapping);
            }
        }
    } else {

        // If the file is not open, print an error message
        std::cerr << "Failed to open file: " << filename << std::endl;
        std::cerr << "Run the anonymize step first — it writes the mapping." << std::endl;
    }
}

// Process the LLM's reply using the mapping and write the restored version out.
void processCSVWithMapping(const std::string& inputFilename, const std::string& outputFilename, const std::vector<std::map<std::string, std::string>>& mappings) {

    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open input file: " << inputFilename << std::endl;
        return;
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputFilename << std::endl;
        return;
    }

    // Tokens are exactly ten alphanumeric characters, standing alone.
    const std::regex token_pattern("\\b[A-Za-z0-9]{10}\\b");

    std::string line;
    int restored = 0;
    int unrecognised = 0;

    while (std::getline(inputFile, line)) {
        std::string rebuilt;
        std::ptrdiff_t copied_to = 0;

        auto match_iterator = std::sregex_iterator(line.begin(), line.end(), token_pattern);
        auto match_end = std::sregex_iterator();

        for (; match_iterator != match_end; ++match_iterator) {
            std::smatch match = *match_iterator;

            // Text since the previous candidate passes through untouched.
            rebuilt.append(line, copied_to, match.position(0) - copied_to);

            const std::string candidate = match.str();

            // Search every pattern's mapping for this token.
            const std::string* original = nullptr;
            for (const auto& mapping : mappings) {
                auto found = mapping.find(candidate);
                if (found != mapping.end()) {
                    original = &found->second;
                    break;
                }
            }

            if (original != nullptr) {
                rebuilt += *original;
                ++restored;
            } else {
                // A ten-character word that is not one of ours — ordinary text
                // such as "everything" or "background". Leave it alone.
                rebuilt += candidate;
                ++unrecognised;
            }

            copied_to = match.position(0) + match.length(0);
        }

        // Whatever follows the final candidate on this line.
        rebuilt.append(line, copied_to, std::string::npos);
        outputFile << rebuilt << std::endl;
    }

    inputFile.close();
    outputFile.close();

    std::cout << "Restored " << restored << " value(s) into " << outputFilename << "." << std::endl;
    if (unrecognised > 0) {
        std::cout << "(" << unrecognised << " ten-character word(s) were not tokens and were left as-is.)" << std::endl;
    }
}
