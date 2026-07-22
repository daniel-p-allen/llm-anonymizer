# anonymizer v1 (2023) — design record

A record of what this project was, how far the original thinking got, and where it stopped.
Written 2026-07 by reading the 2023 source, so that the original design is preserved as its own
artefact before any modernisation happens.

**Status:** feature-complete prototype. The full round trip was built and demonstrated end to end.
Several latent defects (§6) meant it only worked reliably on small, simple inputs.

---

## 1. The problem it set out to solve

In 2023, using ChatGPT on anything real meant pasting real data into someone else's server. There was
no practical way to get the *usefulness* of an LLM on personal correspondence without surrendering the
correspondence itself.

The insight behind this project: you don't actually need the model to see the sensitive values. You
need it to see the *structure*. So strip the identifying values out, let the model work on what's left,
then put the values back into whatever it returns.

Stated in the source itself (`program.cpp:10-21`):

> This program is designed to anonymize data contained in a JSON file (output data received from gmail
> perhaps) ... The user will then use that anonymous JSON data in something like chatGPT and then
> output the data to a CSV file ... The program will also allow the user to revert the anonymized data
> back to the original data.

## 2. The core idea — a reversible airlock

Anonymising data is easy. Anonymising it *reversibly*, so the model's output is still usable, is the
hard half — and that reverse leg is what the project committed to building.

```
  Gmail ──► data.json ──► [ anonymize ] ──► anonymized_data.json ──► human review (GUI)
                                │                                            │
                                ▼                                            ▼
                          swapped.json                                 paste into ChatGPT
                        (the mapping)                                        │
                                │                                            ▼
                                └──────────► [ revert ] ◄────────── finished_data.csv
                                                  │
                                                  ▼
                                           output_data.csv
```

Two directions, one mapping file (`swapped.json`) as the pivot. The mapping never leaves the machine.

## 3. Architecture as built

Deliberately separated into single-responsibility translation units — a design choice, not an accident:

| File | Responsibility |
|---|---|
| `program.cpp` | Entry point, menu, orchestration |
| `fetch_emails.py` | Gmail API retrieval (Python, because the Google client libraries are Python) |
| `gmail_connector.cpp` | Bridges C++ to the Python script via `popen` |
| `email_logic.cpp` | Thin coordination layer over the connector |
| `email_processing.cpp` | JSON ↔ `email_content` struct serialisation |
| `anonymizer.cpp` | Pattern matching, token generation, mapping capture (the forward leg) |
| `revert.cpp` | Mapping reload and substitution back into LLM output (the reverse leg) |
| `validation.cpp` | Drives anonymisation across every field, prints before/after |
| `gui.cpp` | SplashKit window for human review |

**Language split:** C++ for the core, Python only where an external API forced it. The bridge is a
`popen` subprocess call (`gmail_connector.cpp:6`) — crude, but it kept the Python dependency contained
to one file rather than letting it spread.

**SplashKit** provides the GUI layer. It was the framework in use at the time, and the visual review
step was considered essential enough to justify a graphics dependency in an otherwise headless tool.

**Data model** (`email_processing.h:7`): an email is `{from, to, subject, content}` — four strings.
Every field gets anonymised independently (`validation.cpp:25-28`).

## 4. Design decisions worth recording

- **Human-in-the-loop by default.** The GUI is not decoration; it exists so a person confirms what's
  been scrubbed *before* anything is exposed. The trust model assumes the automated detection is
  imperfect and a human is the last check.
- **Local-first.** Retrieval, anonymisation, mapping and reversal all happen on the machine. Only the
  scrubbed text is ever intended to leave it.
- **Read-only Gmail scope** (`fetch_emails.py:10`): `gmail.readonly`. The tool is structurally
  incapable of modifying the mailbox — a deliberate blast-radius limit.
- **Random opaque tokens.** Each detected value is replaced with a random 10-character alphanumeric
  string (`anonymizer.cpp:69-80`). The reasoning: a token that carries no information can't leak
  anything by its shape.
- **Regex-based detection.** Four patterns (`anonymizer.cpp:18-23`) covering email addresses and three
  phone-number shapes. Chosen for having no dependencies and being inspectable.
- **JSON for the machine legs, CSV for the human leg.** JSON in and out of the tool; CSV as the format
  you'd paste to and from a chat window and open in a spreadsheet.

## 5. How far it got

Working, and demonstrated end to end:

- Gmail authentication with token caching and refresh (`fetch_emails.py:13-25`)
- Retrieval of N messages into the four-field structure
- Detection and replacement of emails and phone numbers
- Mapping capture and persistence to `swapped.json`
- SplashKit review window
- Mapping reload and substitution back into CSV
- A menu offering the two legs independently (`program.cpp:26`), so the reverse leg could be run later
  against a saved mapping — a sensible separation, since the LLM step happens outside the program

The final commit is titled *"API Finished Tested Working"*. The round trip genuinely completed.

## 6. Where the thinking stopped

These are the honest limits of v1. Most are visible in the code or in the author's own commit
messages — *"working version with everything needs cleaning"*, *"New API working 5 email limit, wont
do more?"*

**Latent correctness defects (found in 2026 review, present in the 2023 code):**

- **Multiple matches collapse to a single token.** `anonymizer.cpp:45` calls `regex_replace` over the
  whole string on every loop iteration, so the first match's token replaces *every* match. Later
  iterations generate tokens that never appear in the text, while the mapping still records them.
  On reversal, every value in a field returns as the *first* value. Only shows up with more than one
  match per field — which is why single-address test data appeared to work.
- **Reversal strips punctuation from all text.** `revert.cpp:83` removes every non-alphanumeric
  character from every word, whether or not it is a token. Commas, currency symbols and `@` are
  destroyed in passing.
- **CSV parsing is comma-split, not quote-aware.** `revert.cpp:70` splits on every comma, so email
  bodies containing commas fragment across columns.
- **The digit pattern over-matches.** `anonymizer.cpp:20` requires three digit groups with *optional*
  separators, so a bare four-digit year satisfies it (`2021` → `20|2|1`). Years were tokenised and
  sentences mangled.
- **The Python bridge invokes `python`** (`gmail_connector.cpp:6`), which no longer exists as a command
  on modern macOS, and bypasses any virtual environment.
- **The "5 email limit"** was likely never a real cap. `fetch_emails.py:68` defaults to 5 when no
  argument arrives; if the subprocess fails silently the previous `data.json` persists, presenting as
  a stuck limit. `maxResults` (`fetch_emails.py:29`) would happily return far more. *Unconfirmed —
  needs a live run with credentials.*

**Design limits acknowledged at the time or implied by the approach:**

- Regex cannot detect names, street addresses or account numbers — only pattern-shaped values.
- Opaque tokens tell the model nothing about what it is handling, so its output quality suffers and
  reversal has no type information to check against.
- The LLM step is entirely manual: copy out, paste in, save the reply by hand.
- Message bodies come from Gmail's `snippet` field (`fetch_emails.py:40`) — a truncated preview, not
  the full body.
- No test suite, no build file, no dependency manifest. The build recipe existed only inside the
  editor configuration.
- No preview mode: you cannot see what *would* be scrubbed without performing the scrub.

## 7. What v1 established

Even with the defects, the prototype settled the questions that mattered:

- The reversible airlock is a coherent idea and can be built.
- Local-first with human review is the right trust model.
- The two legs should be independently runnable, because the LLM step happens outside the program.
- The hard part is not detection — it is faithful reversal.

Everything after this point is an argument about *execution*: better detection, typed placeholders
instead of opaque tokens, a transport format that survives punctuation, and automating the step that
was manual. The concept did not need revisiting.

---

*This file describes v1 as it stood in 2023. It is kept as a design record; it is not documentation
for the current version.*
