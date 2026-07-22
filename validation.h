#ifndef VALIDATION_H
#define VALIDATION_H
#include <iostream>
#include <vector>
#include <string>
#include "email_processing.h"
#include "anonymizer.h"
using namespace std;

// This procedure will process & validate the email data.
// source_file is the JSON to read — data.json for a real fetch, or
// sample_data.json in demo mode.
// Returns false if the source file could not be read.
bool validate_email_data(placeholder_data& placeholders, const std::string& source_file);

#endif // VALIDATION_H
