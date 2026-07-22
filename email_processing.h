#ifndef EMAIL_PROCESSING
#define EMAIL_PROCESSING
#include "splashkit.h"
#include <string>

// Struct to represent email content
struct email_content {
    std::string from;
    std::string content;
    std::string subject;
    std::string to;
};

// Displays the email content
void display_anonymized_text(const std::string& text);

// Function to read email content from data.json
vector<email_content> read_data_from_json(const std::string &filename);

// Save email content to a JSON file
void save_data_to_json(const std::string &filename, const std::vector<email_content> &emailData);

#endif // EMAIL_PROCESSING
