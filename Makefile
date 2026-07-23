# anonymizer v1 — build
#
# SplashKit is not a conventional library: the SplashKit Manager (skm) wraps the
# compiler and injects the correct include and link paths. So we invoke the
# compiler *through* skm rather than setting the flags ourselves.
#
# Requires: skm on PATH — https://splashkit.io/installation/

#   src/     program source
#   tests/   unit tests
#   vendor/  third-party headers, vendored so there is nothing to install
#   assets/  fonts and other runtime resources
#   scripts/ repository tooling

BIN  := anonymizer
SRCS := $(wildcard src/*.cpp)
INCS := -Isrc -Ivendor
STD  := c++14

# The test binary links the logic and nothing else. program.cpp is excluded
# because doctest supplies main(); gui.cpp, gmail_connector.cpp and
# email_logic.cpp are excluded because they need a window, a network and OAuth
# credentials respectively. What is left is self-contained.
TEST_BIN  := run_tests
CORE_SRCS := src/anonymizer.cpp src/email_processing.cpp \
             src/validation.cpp src/revert.cpp
TEST_SRCS := $(wildcard tests/*.cpp)

.PHONY: all clean check demo deps test

all: $(BIN)

$(BIN): $(SRCS)
	skm clang++ -std=$(STD) $(INCS) -g $(SRCS) -o $(BIN)

# Run the round trip against the bundled sample data.
# No Gmail account, no OAuth credentials, no setup.
demo: $(BIN)
	./$(BIN) --demo

# The Python side (Gmail retrieval only) lives in a venv so it never touches
# system python.
deps:
	python3 -m venv .venv
	.venv/bin/pip install -r requirements.txt

# Refuse to ship if anything resembling real personal data is present. This
# tool's whole promise is protecting data, so the repo has to model that.
#
# Kept separate from `test`: this needs nothing installed and runs in a second,
# which is what makes it usable as a habit before every push.
check:
	@./scripts/check-pii.sh

# Unit tests. Each one runs in a temporary directory of its own, because the code
# under test writes swapped.json and anonymized_data.json to the working
# directory — running them here would overwrite real files.
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(CORE_SRCS) $(TEST_SRCS)
	skm clang++ -std=$(STD) $(INCS) -Itests -g $(CORE_SRCS) $(TEST_SRCS) -o $(TEST_BIN)

clean:
	rm -rf $(BIN) $(TEST_BIN) *.o *.dSYM anonymized_data.json swapped.json output_data.csv
