#ifndef GUI_H
#define GUI_H
#include "anonymizer.h"
#include "json.hpp"
#include <string>

// Struct to store the GUI properties
struct GUI {
    std::string title;
    int width;
    int height;
};

// Load resources
void load_resources();

// Initialize the GUI
void init_gui(GUI &gui, const std::string &configFile);

// Run the GUI
void run_gui(GUI &gui);

#endif // GUI_H
