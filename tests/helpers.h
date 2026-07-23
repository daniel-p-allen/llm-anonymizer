#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

// Shared fixtures for the test suite.
//
// The functions under test write to fixed filenames in the working directory —
// anonymizer.cpp and revert.cpp both hardcode "swapped.json", and
// validate_email_data writes "anonymized_data.json". Running the tests in the
// repository would therefore overwrite real files and let one test's output leak
// into the next. TempDir moves each test into a directory of its own instead.

#include <ftw.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Callback for nftw below. Deletes whatever it is handed, depth first.
inline int remove_entry(const char* path, const struct stat*, int, struct FTW*) {
    return ::remove(path);
}

// A scratch directory that becomes the working directory for as long as it is in
// scope, and is deleted on the way out. std::filesystem would be tidier but this
// project builds as C++14, so nftw does the recursive delete.
class TempDir {
public:
    TempDir() {
        char previous[4096];
        if (::getcwd(previous, sizeof(previous)) == nullptr) {
            throw std::runtime_error("could not read the current directory");
        }
        previous_ = previous;

        char pattern[] = "/tmp/anonymizer_test_XXXXXX";
        const char* made = ::mkdtemp(pattern);
        if (made == nullptr) {
            throw std::runtime_error("could not create a temporary directory");
        }
        dir_ = made;

        if (::chdir(dir_.c_str()) != 0) {
            throw std::runtime_error("could not enter the temporary directory");
        }
    }

    ~TempDir() {
        // Back out before deleting, or the working directory is left dangling.
        ::chdir(previous_.c_str());
        ::nftw(dir_.c_str(), remove_entry, 16, FTW_DEPTH | FTW_PHYS);
    }

    // Not copyable — two owners would delete the same directory twice.
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::string& path() const { return dir_; }

private:
    std::string dir_;
    std::string previous_;
};

// Write a file into the current (temporary) directory.
inline void write_file(const std::string& name, const std::string& contents) {
    std::ofstream file(name);
    if (!file.is_open()) {
        throw std::runtime_error("could not write " + name);
    }
    file << contents;
}

// Read a whole file back, bytes as they are. Returns an empty string if it is
// not there, so tests can assert on absence without throwing.
inline std::string read_file(const std::string& name) {
    std::ifstream file(name);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline bool file_exists(const std::string& name) {
    std::ifstream file(name);
    return file.is_open();
}

// Captures anything written to std::cout for as long as it is in scope.
// print_findings_summary reports to the console rather than returning a value, so
// reading its output is the only way to assert on what it counted.
class CaptureCout {
public:
    CaptureCout() : original_(std::cout.rdbuf()) {
        std::cout.rdbuf(buffer_.rdbuf());
    }

    ~CaptureCout() { std::cout.rdbuf(original_); }

    CaptureCout(const CaptureCout&) = delete;
    CaptureCout& operator=(const CaptureCout&) = delete;

    std::string text() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    std::streambuf* original_;
};

// How many lines of the captured output contain the given fragment.
inline int count_lines_containing(const std::string& text, const std::string& fragment) {
    std::istringstream stream(text);
    std::string line;
    int found = 0;
    while (std::getline(stream, line)) {
        if (line.find(fragment) != std::string::npos) ++found;
    }
    return found;
}

// Build the JSON shape that read_data_from_json expects, so tests describe their
// input as emails rather than as hand-written JSON.
struct TestEmail {
    std::string from;
    std::string to;
    std::string subject;
    std::string content;
};

inline std::string emails_as_json(const std::vector<TestEmail>& emails) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < emails.size(); ++i) {
        if (i > 0) json << ",";
        json << "{"
             << "\"from\":\"" << emails[i].from << "\","
             << "\"to\":\"" << emails[i].to << "\","
             << "\"subject\":\"" << emails[i].subject << "\","
             << "\"content\":\"" << emails[i].content << "\""
             << "}";
    }
    json << "]";
    return json.str();
}

#endif // TEST_HELPERS_H
