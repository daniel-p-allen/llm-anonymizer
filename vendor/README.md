# vendor

Third-party code, committed so the project has nothing to install beyond
SplashKit itself.

| File | Source | License |
|---|---|---|
| `json.hpp` | [nlohmann/json](https://github.com/nlohmann/json) | MIT |
| `doctest.h` | [doctest](https://github.com/doctest/doctest) v2.4.11 | MIT |

`doctest.h` is the test framework, used by `make test`. It is a single header for
the same reason everything else here is committed: GoogleTest and Catch2 both need
building first, which would have meant something else to install.

Nothing here is my own work, and nothing here should be edited — replace the file
wholesale to upgrade.
