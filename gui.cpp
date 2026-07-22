#include "gui.h"
#include "anonymizer.h"
// #include "splashkit.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Load resources
void load_resources() {

    load_font("arial", "DMSans_36pt-Bold.ttf");
}

// Display text on the GUI
void display_text(const std::string &text, int x, int y) {
    color text_color = random_color();
    draw_text(text, text_color, "arial", 16, x, y);
}

// Render a long string, splitting it into segments if necessary.
// Returns the y position just below the text it drew, so the caller can stack
// fields without them overlapping — previously each field was drawn at a fixed
// y and anything that wrapped ran straight over the field below it.
int render_long_string(const std::string &text, int x, int y, int max_width, int font_size) {
    const std::string font_name = "arial";

    // Split the input string into segments of at most 111 characters each
    std::vector<std::string> segments;
    for (size_t i = 0; i < text.length(); i += 111) {
        segments.push_back(text.substr(i, 111));
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

// Function to run the GUI
void run_gui(GUI &gui) {

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

    // Main loop for the GUI
    while (true) {
        process_events();

        if (window_close_requested(gui.title)) {
            std::cout << "Window close requested." << std::endl;
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
                                   "  -  check nothing sensitive remains", 20, y, 1000, 16);
            y += 10;
            y = render_long_string("From: " + from, 20, y, 1000, 16);
            y = render_long_string("To: " + to, 20, y, 1000, 16);
            y = render_long_string("Subject: " + subject, 20, y, 1000, 16);
            y = render_long_string("Content: " + content, 20, y, 1000, 16);

            render_long_string("[ press any key for the next email ]", 20, y + 20, 1000, 16);

            refresh_screen(60);

            // Advance on a keypress rather than a fixed delay, so there is time
            // to actually read the entry being reviewed.
            if (any_key_pressed()) {
                ++current_entry;
            }
        } else {
            // Reviewed everything — hold the last frame until the window closes
            clear_screen(COLOR_WHITE);
            render_long_string("Reviewed " + std::to_string(total_entries) +
                               " email(s). Close this window to continue.", 20, 20, 1000, 16);
            refresh_screen(60);
        }

    }
    std::cout << "Exiting the main loop." << std::endl;
    // Close the window
    close_window(gui.title);
    std::cout << "After close window." << std::endl;
}

