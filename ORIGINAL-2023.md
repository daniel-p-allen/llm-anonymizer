# anonymizer v1 — design record

What this project was, how far the original 2023 thinking got, and what had to be
repaired in 2026 to make it work as intended. The architecture described here is
the architecture that ships: C++ core, SplashKit review window, Python bridge for
Gmail, regex detection, opaque tokens, manual LLM step.

---

## 1. The problem it set out to solve

In 2023, using ChatGPT on anything real meant pasting real data into someone
else's server. There was no practical way to get the *usefulness* of an LLM on
personal correspondence without surrendering the correspondence itself.

The insight behind this project: you don't need the model to see the sensitive
values. You need it to see the *structure*. So strip the identifying values out,
let the model work on what's left, then put the values back into what it returns.

Stated in the source itself (`program.cpp`):

> This program is designed to anonymize data contained in a JSON file (output
> data received from gmail perhaps) ... The user will then use that anonymous
> JSON data in something like chatGPT ... The program will also allow the user to
> revert the anonymized data back to the original data.

## 2. The core idea — a reversible airlock

Anonymising data is easy. Anonymising it *reversibly*, so the model's output is
still usable, is the hard half — and that reverse leg is what the project
committed to building. Two directions, with `swapped.json` as the pivot. The
mapping never leaves the machine.

## 3. Architecture

Deliberately separated into single-responsibility translation units — a design
choice, not an accident:

| File | Responsibility |
|---|---|
| `program.cpp` | Entry point, menu, orchestration |
| `fetch_emails.py` | Gmail API retrieval (Python, because the Google client libraries are Python) |
| `gmail_connector.cpp` | Bridges C++ to the Python script via `popen` |
| `email_logic.cpp` | Thin coordination layer over the connector |
| `email_processing.cpp` | JSON ↔ `email_content` struct serialisation |
| `anonymizer.cpp` | Pattern matching, token generation, mapping capture (forward leg) |
| `revert.cpp` | Mapping reload and substitution back into LLM output (reverse leg) |
| `validation.cpp` | Drives anonymisation across every field |
| `gui.cpp` | SplashKit window for human review |

**Language split:** C++ for the core, Python only where an external API forced it.
The bridge is a `popen` subprocess call — crude, but it kept the Python
dependency contained to one file rather than letting it spread.

**Data model:** an email is `{from, to, subject, content}` — four strings, each
anonymised independently.

## 4. Design decisions

- **Human-in-the-loop by default.** The GUI is not decoration; it exists so a
  person confirms what's been scrubbed *before* anything is exposed. The trust
  model assumes automated detection is imperfect and a human is the last check.
- **Local-first.** Retrieval, anonymisation, mapping and reversal all happen on
  the machine. Only the scrubbed text is ever intended to leave it.
- **Read-only Gmail scope** — `gmail.readonly`. The tool is structurally
  incapable of modifying the mailbox; a deliberate blast-radius limit.
- **Random opaque tokens.** Each detected value becomes a random 10-character
  string. The reasoning: a token carrying no information can't leak anything by
  its shape.
- **Regex-based detection.** Chosen for having no dependencies and being
  inspectable.
- **JSON for the machine legs, CSV for the human leg** — JSON in and out of the
  tool; CSV as the format you'd paste to and from a chat window.

## 5. What v1 established

The prototype settled the questions that mattered:

- The reversible airlock is a coherent idea and can be built.
- Local-first with human review is the right trust model.
- The two legs should be independently runnable, because the LLM step happens
  outside the program.
- The hard part is not detection — it is faithful reversal.

Everything after this point is an argument about *execution*: better detection,
typed placeholders instead of opaque tokens, and automating the manual step. The
concept did not need revisiting.

---

## 6. The 2026 repair

The 2023 build completed its round trip and was titled *"API Finished Tested
Working"* — but it only worked reliably on small, simple inputs. Six defects were
found and fixed. The architecture was not changed.

| # | Defect | Location | Resolution |
|---|--------|----------|------------|
| 1 | All matches of a pattern collapsed to a single token | `anonymizer.cpp` | Matches replaced individually while walking the string; a repeated value reuses its token |
| 2 | Reversal stripped punctuation from all text | `revert.cpp` | Tokens substituted in place; everything else copied byte for byte |
| 3 | CSV parsing split on every comma | `revert.cpp` | Removed — the reverse leg locates tokens by shape instead of parsing CSV |
| 4 | Digit pattern matched bare years | `anonymizer.cpp` | Separators now required; long references handled by a separate `\d{7,}` rule |
| 5 | Python bridge invoked `python`, absent on current macOS, and failed silently | `gmail_connector.cpp` | Prefers `.venv/bin/python`, checks exit status, reports the cause |
| 6 | `rand()` never seeded, so tokens repeated across runs | `program.cpp` | Seeded from the clock at startup |

**Defect 1** was the serious one. The original called `std::regex_replace` over
the whole string once per match, so the first match's token replaced *every*
match of that pattern, while later iterations recorded tokens that never appeared
in the text. On reversal, every value in a field returned as the first value. It
only manifests with more than one match per field, which is why test data with a
single address per field appeared to work.

**Defect 4** came from optional separators and single-digit groups:
`\b\d{1,4}[-.\s]?\d{1,4}[-.\s]?\d{1,4}\b` accepts `2021` as `20|2|1`. Years and
quantities were replaced, corrupting ordinary prose.

### One behavioural change

The review window originally auto-advanced on a two-second timer and then ended,
with no way to accept or refuse the result — the outcome was identical whether or
not the reviewer spotted a problem. Since `program.cpp` states the window exists
"to allow the user to verify the data", a review with no possible verdict does
not achieve what was intended.

Each email now carries its own decision: `A` approves, `R` rejects, `SPACE`
defers, `B` steps back to revise. On finishing, `anonymized_data.json` is
rewritten to hold only approved entries. Per-email rather than one verdict for
the batch, because a run of thirty may contain a single problem email, and one
decision would force discarding all thirty or sharing the doubtful one.

Anything left undecided is withheld rather than shared, and approving nothing
deletes the file outright. Treating unreviewed data as safe is precisely the
failure this step exists to prevent.

### Verification

From actual runs, not inspection:

- **Detection** — `2021`, `5000`, `4200`, `22 October 2023`, `Sep 21`, `AU$5` and
  `48 hours` survive untouched; only contact details and long references are
  replaced.
- **Token consistency** — 10 mappings from the sample set, no collisions. The
  same address received the same token in every field of every email, so a model
  can still infer that separate mentions refer to one person.
- **Round trip** — a deliberately punctuation-heavy reply restored 8 values with
  parentheses, semicolons, em-dashes, `$`, `#`, and commas inside quoted fields
  all preserved.
- **Review window** — confirmed by use, including revising earlier decisions.
  `any_key_pressed()` never registered input and was replaced with explicit
  `key_typed()` checks.

**Not verified:** the Gmail retrieval path, which needs a `secret.json` OAuth
client. The historical *"5 email limit"* noted in a 2023 commit message therefore
remains a well-supported theory rather than a confirmed diagnosis:
`fetch_emails.py` defaults to 5 when no argument arrives, and defect 5 meant a
failed fetch left the previous `data.json` in place — which presents exactly as a
stuck limit. `maxResults` imposes no such cap.

### Supporting work

A `Makefile` (the build recipe previously existed only inside the editor
configuration), a `make demo` path needing no Gmail account or credentials,
a findings summary in place of per-word debug output, a menu reordered to match
the workflow, and `scripts/check-pii.sh` so the repository checks itself.

## 7. Data handling

The 2023 repository had real inbox contents committed across its history — an
email address, an order reference, financial alerts and a health-related subject
line. No credentials were ever committed and the repository was private
throughout, so nothing was exposed and no rotation was required.

Runtime-generated files were removed and gitignored by name, since they
legitimately contain real data; `secret.json` and `token.json` were gitignored
too, the OAuth client filename in particular having had no prior rule. Real data
was replaced with `sample_data.json`, a synthetic fixture matching the original
schema and exercising every detection pattern.

This repository was created with fresh history rather than rewriting the old one,
so the data cannot be present rather than merely believed removed. The 2023
repository is retained privately as a provenance record.

## 8. Known limitations

- **Names, street addresses and other prose identifiers are not detected.** The
  matching is regex-based, so it finds values with a recognisable *shape*. This is
  why the review step exists and why it is not optional.
- **Parenthesised area codes** such as `(03) 9000 1234` are missed.
- **Unseparated 5–6 digit numbers** fall between the phone and long-number rules.
- **Message bodies come from Gmail's `snippet` field** — a preview, not full text.
- **The LLM step is manual.** The tool does not talk to any model itself.
- **Tokens carry no type information**, so the model cannot tell a phone number
  from an address, and its output quality suffers accordingly.
