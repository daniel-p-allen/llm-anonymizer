// gmail_connector — bridge from C++ to the Python retrieval script.
//
// The Google client libraries are Python-only, so retrieval is delegated to
// fetch_emails.py and run as a subprocess. Keeping the bridge in one file stops
// the Python dependency spreading through the rest of the program.

#include "gmail_connector.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// Returns the interpreter to use. The project venv is preferred, because that
// is where the Google client libraries are installed; a bare `python3` will
// usually not have them. Note that `python` (no 3) does not exist at all on
// current macOS — invoking it was why this step used to fail silently.
static std::string python_interpreter() {
    std::ifstream venv(".venv/bin/python");
    if (venv.good()) {
        return ".venv/bin/python";
    }
    return "python3";
}

std::string GmailConnector::fetchEmails(int num_emails) {
    const std::string interpreter = python_interpreter();

    // Redirect stderr into the pipe so authentication errors are visible
    // rather than vanishing.
    const std::string command = interpreter + " src/fetch_emails.py " + std::to_string(num_emails) + " 2>&1";

    std::cout << "Fetching " << num_emails << " email(s) via " << interpreter << "..." << std::endl;

    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Could not start the retrieval process." << std::endl;
        return "";
    }
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
        result += buffer;
    }

    // Fail loudly. Previously the exit status was discarded, so a failed fetch
    // left the previous data.json in place and looked like a working run that
    // was stuck on an old number of emails.
    const int status = pclose(pipe);
    if (status != 0) {
        std::cerr << "\nRetrieval failed (exit " << status << ")." << std::endl;
        if (!result.empty()) {
            std::cerr << result << std::endl;
        }
        std::cerr << "Check that:" << std::endl;
        std::cerr << "  * the Python dependencies are installed   (make deps)" << std::endl;
        std::cerr << "  * secret.json is present                  (Google OAuth client)" << std::endl;
        return "";
    }

    return result;
}
