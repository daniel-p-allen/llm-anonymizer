# Work report — v1 repair

**Date:** 2026-07-22
**Scope:** Repair the 2023 build so it is correct, reproducible and presentable,
without altering its architecture.
**Outcome:** Complete. Builds clean, round trip verified end to end.

The architecture is unchanged: C++ core, SplashKit review window, Python bridge
for Gmail, regex detection, opaque tokens, manual LLM step. Only defects and
presentation were addressed. See [`ORIGINAL-2023.md`](ORIGINAL-2023.md) for the
design record of the version this repairs.

---

## 1. Defects fixed

| # | Defect | Location | Resolution |
|---|--------|----------|------------|
| 1 | All matches of a pattern collapsed to a single token | `anonymizer.cpp:45` | Matches are now replaced individually while walking the string; a repeated value reuses its existing token |
| 2 | Reversal stripped punctuation from all text | `revert.cpp:83` | Tokens are substituted in place; everything else is copied byte for byte |
| 3 | CSV parsing split on every comma | `revert.cpp:70` | Removed — the reverse leg no longer parses CSV, it locates tokens by shape |
| 4 | Digit pattern matched bare years | `anonymizer.cpp:20` | Separators are now required; long references handled by a separate `\d{7,}` rule |
| 5 | Python bridge invoked `python`, which does not exist on current macOS, and failed silently | `gmail_connector.cpp:6` | Prefers `.venv/bin/python`, falls back to `python3`, checks exit status and reports the cause |
| 6 | `rand()` was never seeded, so tokens repeated across runs | `program.cpp` | Seeded from the clock at startup |

Defects 5 and 6 were found during this work; 1–4 were identified in review.

### Detail on defect 1

The original called `std::regex_replace` over the whole string once per match.
The first match's token therefore replaced *every* match of that pattern, while
subsequent iterations generated tokens that never appeared in the text but were
still recorded in the mapping. On reversal, every value in a field returned as
the first value.

This only manifests with more than one match per field, which is why test data
containing a single address per field appeared to work correctly.

### Detail on defect 4

The pattern `\b\d{1,4}[-.\s]?\d{1,4}[-.\s]?\d{1,4}\b` made its separators
optional and allowed single-digit groups, so `2021` satisfied it as `20|2|1`.
Years and quantities were replaced, corrupting ordinary prose.

The replacement set requires a leading `+`, real separators, or a run of seven
or more digits — all of which exceed the shape of a year.

---

## 2. Verification

All results below are from actual runs, not inspection.

**Detection accuracy** — `2021`, `5000`, `4200`, `22 October 2023`,
`14 September 2023`, `Sep 21`, `AU$5` and `48 hours` all survive untouched.
Only contact details and long references are replaced.

**Token consistency** — 10 mappings produced from the sample set, no collisions.
`alex.taylor@example.org` received the same token in every field of every email,
so a model can still infer that separate mentions refer to one person.

**Round trip fidelity** — a deliberately punctuation-heavy reply was passed
through the reverse leg. 8 values restored; parentheses, semicolons, em-dashes,
`$`, `#`, commas inside quoted fields and the quotes themselves all preserved.

```
in:  "Order #PO-012-2LGio7SK0q was confirmed; contact eBWS8Zon20 — phone
      gKcU2ZatFt — for a AU$5 credit, within 48 hours."

out: "Order #PO-012-40000000000000000 was confirmed; contact
      alex.taylor@example.org — phone +123456789 — for a AU$5 credit,
      within 48 hours."
```

**Build** — clean build from scratch via `make`, no warnings surfaced by the
SplashKit wrapper.

### Not verified

- **The review window.** The change to field layout and key handling compiles,
  and the console path is fully exercised, but what is drawn on screen has not
  been confirmed visually. Run `./anonymizer` and choose option 1.
- **The Gmail retrieval path**, which requires a `secret.json` OAuth client that
  is not present. Consequently the historical "5 email limit" remains a
  well-supported theory rather than a confirmed diagnosis: `fetch_emails.py:68`
  defaults to 5 when no argument arrives, and defect 5 meant a failed fetch left
  the previous `data.json` in place — which would present exactly as a stuck
  limit. `maxResults` at `fetch_emails.py:29` imposes no such cap.

---

## 3. Presentation work

- **`Makefile`** — the build recipe previously existed only inside the editor
  configuration. Targets: `all`, `demo`, `deps`, `check`, `clean`.
- **Demo path** — `make demo` runs the full round trip against bundled sample
  data with no Gmail account, credentials or window required.
- **Console output** — the reverse leg printed a line per word processed. It now
  prints a summary of what was found, by category.
- **Menu order** — options previously ran counter to the workflow (restore was
  first, fetch second). Reordered to demo, fetch, restore, exit.
- **Next-step guidance** — the LLM step happens outside the program, so the
  program now states explicitly what to do after anonymizing.
- **Review window** — fields were drawn at fixed vertical positions and
  overlapped whenever text wrapped; they now stack. Advance is on a keypress
  rather than a two-second timer, and text is no longer drawn in random colours.
- **`scripts/check-pii.sh`** — scans tracked files for generated data,
  credentials, and any email address outside the RFC 2606 example domains.
- **`README.md`** — purpose, round-trip diagram, ten-second demo with real
  captured output, prerequisites split by whether Gmail is needed, Gmail setup,
  detection coverage, and known limitations.

---

## 4. Data handling

The 2023 repository had real inbox contents committed across its history:
an email address, an order reference, financial alerts and a health-related
subject line. No credentials were ever committed and the repository was private
throughout, so nothing was exposed and no rotation was required.

Actions taken:

- Removed all runtime-generated files from the working tree and gitignored them
  by name, since they legitimately contain real data.
- Gitignored `secret.json` and `token.json`. The OAuth client filename in
  particular was not covered by any prior ignore rule.
- Replaced the real data with `sample_data.json`, a synthetic fixture matching
  the original schema and exercising every detection pattern.
- Added `make check` so the condition cannot silently recur.

`make check` currently reports failures. This is correct: the generated files
remain in the *old* repository's index. The agreed remedy is a new repository
with fresh history rather than a history rewrite, so that the data cannot be
present rather than merely believed removed. The original repository is retained
privately as a provenance record.

---

## 5. Outstanding

| Item | Owner |
|---|---|
| Visual confirmation of the review window | Manual |
| Screenshot of the window for the README | Manual |
| Create the new repository and push clean history | Next session |
| Confirm the Gmail path once `secret.json` exists | Blocked on credentials |
| Decide whether comment density should match the 2023 style throughout | Open |
