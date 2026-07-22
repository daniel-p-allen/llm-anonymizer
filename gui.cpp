#include "gui.h"
#include "anonymizer.h"
// #include "splashkit.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

// Load resources
void load_resources() {

    load_font("arial", "DMSans_36pt-Bold.ttf");
}

// Render a long string, splitting it into segments if necessary.
// Returns the y position just below the text it drew, so the caller can stack
// fields without them overlapping — previously each field was drawn at a fixed
// y and anything that wrapped ran straight over the field below it.
int render_long_string(const std::string &text, int x, int y, int max_width, int font_size) {
    const std::string font_name = "arial";

    // Split the input into lines of at most 111 characters, breaking at spaces.
    // Cutting at a fixed offset split words down the middle ("call u / s on"),
    // which looked like a rendering fault rather than a wrap.
    const size_t max_chars = 111;
    std::vector<std::string> segments;
    size_t start = 0;

    while (start < text.length()) {
        size_t take = std::min(max_chars, text.length() - start);

        // If more text follows, back up to the last space so a word is not split.
        if (start + take < text.length()) {
            size_t space = text.rfind(' ', start + take);
            if (space != std::string::npos && space > start) {
                take = space - start;
            }
        }

        segments.push_back(text.substr(start, take));
        start += take;

        // Skip the space we broke on, so lines do not begin with one.
        while (start < text.length() && text[start] == ' ') {
            ++start;
        }
    }

    // Render each segment on a new line
    for (const std::string &segment : segments) {
        draw_text(segment, COLOR_BLACK, font_name, font_size, x, y);
        y += text_height(segment, font_name, font_size) + 2;
    }

    return y;
}

// Initialize the GUI
void init_gui(GUI &gui, const std::string &configFile) {
    // Read the configuration from the JSON file
    try {
        nlohmann::json jsonConfig;
        std::ifstream configFileStream(configFile);
        
        // Check if the file was opened successfully
        if (!configFileStream.is_open()) {
            std::cerr << "Error opening config file: " << configFile << std::endl;
            exit(1);
        }
        
        configFileStream >> jsonConfig;
        configFileStream.close();

        // Set the GUI properties
        gui.title = jsonConfig["title"];
        gui.width = jsonConfig["width"];
        gui.height = jsonConfig["height"];

    // Catch any exceptions thrown by the JSON library
    } catch (const std::exception &e) {
        std::cerr << "Error reading JSON configuration: " << e.what() << std::endl;
        exit(1);
    }
}

// Function to run the GUI.
//
// This is the human-in-the-loop step: the regex matching cannot detect names or
// addresses, so a person confirms the scrubbed text before any of it is shared.
// A review with no verdict would be decoration, so each email is judged on its
// own and the results are collected in `decisions`.
//
// Per-email rather than one verdict for the batch: with thirty emails you may
// well want to keep twenty-nine and drop one, and a single decision would force
// you to discard everything or share something you were unsure about.
//
// Anything left undecided is not approved. Silence is not consent here.
void run_gui(GUI &gui, std::vector<review_result> &decisions) {

    // Open the window
    open_window(gui.title, gui.width, gui.height);

    nlohmann::json jsonData;

    // Read the JSON data from the file
    std::ifstream jsonFileStream("anonymized_data.json");

    // Check if the file was opened successfully
    if (!jsonFileStream.is_open()) {
        std::cerr << "Error opening JSON file: anonymized_data.json" << std::endl;
        exit(1);
    }

    // Parse the JSON data
    jsonFileStream >> jsonData;
    jsonFileStream.close();

    int current_entry = 0;
    int total_entries = jsonData.size();

    // One verdict per email, all starting undecided.
    decisions.assign(total_entries, REVIEW_NONE);

    // Describes a single verdict for display.
    auto label_for = [](review_result r) -> std::string {
        if (r == REVIEW_APPROVED) return "approved";
        if (r == REVIEW_REJECTED) return "rejected";
        return "not yet decided";
    };

    // Main loop for the GUI
    while (true) {
        process_events();

        if (window_close_requested(gui.title)) {
            break;  // Exit the loop if the user closes the window
        }

        // Display the current entry, if there are any left
        if (current_entry < total_entries) {
            clear_screen(COLOR_WHITE);

            // Get the data for the current entry
            const auto &entry = jsonData[current_entry];
            std::string from = entry["from"];
            std::string to = entry["to"];
            std::string subject = entry["subject"];
            std::string content = entry["content"];

            // Render the data on the screen, each field starting below the last
            int y = 20;
            y = render_long_string("Email " + std::to_string(current_entry + 1) +
                                   " of " + std::to_string(total_entries) +
                                   "  -  " + label_for(decisions[current_entry]),
                                   20, y, 1000, 16);
            y += 10;
            y = render_long_string("From: " + from, 20, y, 1000, 16);
            y = render_long_string("To: " + to, 20, y, 1000, 16);
            y = render_long_string("Subject: " + subject, 20, y, 1000, 16);
            y = render_long_string("Content: " + content, 20, y, 1000, 16);

            y += 20;
            y = render_long_string("[A] approve this one     [R] reject this one", 20, y, 1000, 16);
            render_long_string("[SPACE] skip for now     [B] back", 20, y, 1000, 16);

            refresh_screen(60);

            // Recording a verdict moves on to the next email automatically.
            if (key_typed(A_KEY)) {
                decisions[current_entry] = REVIEW_APPROVED;
                ++current_entry;
            } else if (key_typed(R_KEY)) {
                decisions[current_entry] = REVIEW_REJECTED;
                ++current_entry;
            } else if (key_typed(SPACE_KEY) || key_typed(RETURN_KEY)) {
                ++current_entry;
            } else if (key_typed(B_KEY) && current_entry > 0) {
                --current_entry;
            }
        } else {
            // Every email has been seen — show what was decided.
            int approved = 0, rejected = 0, undecided = 0;
            for (review_result r : decisions) {
                if (r == REVIEW_APPROVED) ++approved;
                else if (r == REVIEW_REJECTED) ++rejected;
                else ++undecided;
            }

            clear_screen(COLOR_WHITE);
            int y = 20;
            y = render_long_string("Reviewed " + std::to_string(total_entries) + " email(s):", 20, y, 1000, 16);
            y += 10;
            y = render_long_string("  approved         " + std::to_string(approved), 20, y, 1000, 16);
            y = render_long_string("  rejected         " + std::to_string(rejected), 20, y, 1000, 16);
            y = render_long_string("  not yet decided  " + std::to_string(undecided), 20, y, 1000, 16);
            y += 10;

            if (undecided > 0) {
                y = render_long_string("Undecided emails will be withheld, not shared.", 20, y, 1000, 16);
                y += 10;
            }

            y = render_long_string("[ENTER] finish and keep only the approved emails", 20, y, 1000, 16);
            render_long_string("[B] go back and change a decision", 20, y, 1000, 16);
            refresh_screen(60);

            if (key_typed(RETURN_KEY)) {
                break;
            } else if (key_typed(B_KEY) && total_entries > 0) {
                current_entry = total_entries - 1;
            }
        }

    }

    // Close the window
    close_window(gui.title);
}

