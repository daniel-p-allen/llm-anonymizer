#ifndef GUI_H
#define GUI_H
#include "anonymizer.h"
#include "json.hpp"
#include <string>
#include <vector>

// Struct to store the GUI properties
struct GUI {
    std::string title;
    int width;
    int height;
};

// The outcome of reviewing a single email. Anything left as REVIEW_NONE was
// never judged, and is withheld rather than shared.
enum review_result {
    REVIEW_NONE,
    REVIEW_APPROVED,
    REVIEW_REJECTED
};

// Load resources
void load_resources();

// Initialize the GUI
void init_gui(GUI &gui, const std::string &configFile);

// Run the GUI. Fills `decisions` with one verdict per email, in file order.
void run_gui(GUI &gui, std::vector<review_result> &decisions);

#endif // GUI_H
