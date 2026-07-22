#include <fstream>
#include <string>
#include <iostream>
#include "json.hpp"
#include "splashkit.h" 
#include "email_processing.h"

using json_custom = nlohmann::json;
using namespace std;

// Function to read email content from data.json
vector<email_content> read_data_from_json(const std::string &filename) {
    vector<email_content> emailData;

    // Open the file
    ifstream file(filename);

    // Display an error message if the file cannot be opened
    if (!file.is_open()) {
        cerr << "Error opening " << filename << endl;
        return emailData;
    }
    
    // Read the file into a json_custom object
    json_custom jsonData; 
    file >> jsonData;
    file.close();

    // Print the number of entries read from the file
    cout << "Read " << jsonData.size() << " entries from " << filename << endl; // Debug statement

    // Iterate through the json_custom object and store the data in a vector of email_content
    for (const auto &entry : jsonData) {
        email_content email;
        email.from = entry["from"].is_null() ? "" : entry["from"].get<std::string>();
        email.to = entry["to"].is_null() ? "" : entry["to"].get<std::string>();
        email.subject = entry["subject"].is_null() ? "" : entry["subject"].get<std::string>();
        email.content = entry["content"].is_null() ? "" : entry["content"].get<std::string>();

        emailData.push_back(email);
    }

    return emailData;
}

// vector<email_content> read_data_from_json(const std::string &filename) {
//     vector<email_content> emailData;

//     // Open the file
//     ifstream file(filename);

//     // Display an error message if the file cannot be opened
//     if (!file.is_open()) {
//         cerr << "Error opening " << filename << endl;
//         return emailData;
//     }
    
//     // Read the file into a json_custom object
//     json_custom jsonData; 
//     file >> jsonData;
//     file.close();

//     // Print the number of entries read from the file
//     cout << "Read " << jsonData.size() << " entries from " << filename << endl; // Debug statement

//     // Iterate through the json_custom object and store the data in a vector of email_content
//     for (const auto &entry : jsonData) {
//         email_content email;
//         email.from = entry["from"];
//         email.to = entry["to"];
//         email.subject = entry["subject"];
//         email.content = entry["content"];

//         emailData.push_back(email);
//     }

//     return emailData;
// }

// Function to save email content to a JSON file
void save_data_to_json(const std::string &filename, const vector<email_content> &emailData) {
    json_custom jsonData; 

    // Iterate through the vector 
    for (const email_content &email : emailData) {
        json_custom entry;
        entry["from"] = email.from;
        entry["to"] = email.to;
        entry["subject"] = email.subject;
        entry["content"] = email.content;

        jsonData.push_back(entry);
    }

    // Open the file, display an error message if the file cannot be opened
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening " << filename << " for writing" << endl;
        return;
    }
    // Write the json_custom object to the file
    file << jsonData.dump(4); 
    file.close();
}
