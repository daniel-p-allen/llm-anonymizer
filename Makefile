# anonymizer v1 — build
#
# SplashKit is not a conventional library: the SplashKit Manager (skm) wraps the
# compiler and injects the correct include and link paths. So we invoke the
# compiler *through* skm rather than setting the flags ourselves.
#
# Requires: skm on PATH — https://splashkit.io/installation/

#   src/     program source
#   vendor/  third-party headers, vendored so there is nothing to install
#   assets/  fonts and other runtime resources
#   scripts/ repository tooling

BIN  := anonymizer
SRCS := $(wildcard src/*.cpp)
INCS := -Isrc -Ivendor
STD  := c++14

.PHONY: all clean check demo deps

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
check:
	@./scripts/check-pii.sh

clean:
	rm -rf $(BIN) *.o *.dSYM anonymized_data.json swapped.json output_data.csv
