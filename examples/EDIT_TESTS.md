# EDIT Manual Test Matrix

## Task 1 Baseline Fixtures and Smoke Test

- Command: `zde cmake`
- Launch: run `bin/edit examples/SMLBASIC.TXT`
- Keys: `Down`, `Down`, `Up`, `PageDown`, `PageUp`, `Esc`
- Expected:
  - Viewer opens without crash.
  - Status bar remains visible and stable while navigating.
  - Exits cleanly with `Esc`.

## Task 2 Core Refactor Scaffold

- Launch: `bin/edit examples/SMLBASIC.TXT`
- Keys: `Up`, `Down`, `Left`, `Right`, `PageDown`, `PageUp`, `Esc`
- Expected:
  - Navigation behavior matches Task 1.
  - No rendering regressions.

## Task 3 Startup and Arguments

- Launch 1: `bin/edit examples/SMLBASIC.TXT`
- Launch 2: `bin/edit`
- Keys: `Esc`
- Expected:
  - Both flows enter editor cleanly.
  - Optional filename startup works.

## Task 4 Editing v1

- Launch: `bin/edit examples/SMLBASIC.TXT`
- Keys: type `ABC`, `Backspace`, `Delete`, `Insert`, type `Z`, `Insert`, `Esc`
- Expected:
  - Insertions and deletions update visible text at cursor.
  - Insert mode toggle changes behavior and status indicator.
  - Dirty marker appears after first edit.

## Task 5 Save Pipeline

- Launch: `bin/edit examples/SMLBASIC.TXT`
- Keys: type `SAVE`, `F2`, `Esc`
- Reopen: `bin/edit examples/SMLBASIC.TXT`
- Expected:
  - Save reports success.
  - Reopen shows saved bytes.

## Task 6 Open Workflow and Dirty Guard

- Launch: `bin/edit examples/SMLBASIC.TXT`
- Keys: type `X`, `F3`
- Expected:
  - Prompt offers `Save / Discard / Cancel` when dirty.
  - Chosen branch executes correctly and descriptor state remains healthy.

## Task 7 Piece Table Integration

- Launch: `bin/edit examples/SMLBASIC.TXT`
- Keys: type `PT`, `Backspace`, `Delete`, `F2`, `Esc`
- Expected:
  - User-visible editing behavior matches Task 4-6.
  - Save output remains correct.

## Task 8 Source Streaming and Cache

- Launch 1: `bin/edit examples/LRG40K.TXT`
- Launch 2: `bin/edit examples/LRG96K.TXT`
- Keys: repeated `PageDown`, `PageUp`, cursor moves near line starts/ends, `Esc`
- Expected:
  - Smooth scrolling across 16KB boundaries.
  - Viewport remains stable without corruption.

## Task 9 palloc/pfree Memory Management

- Launch: `bin/edit examples/LRG96K.TXT`
- Keys: edit several regions, `F3` switch file, `Esc`
- Expected:
  - Repeated edits and file switches stay stable.
  - No crash on exit.

## Task 10 OOM and Non-printable Rendering

- Launch 1: `bin/edit examples/SMLNPRN.BIN`
- Launch 2: `bin/edit examples/LRG96K.TXT` (under constrained memory test setup)
- Keys: navigation, edit attempts, `F2`, `Esc`
- Expected:
  - Non-printables render as placeholders only in UI.
  - Saved content preserves raw bytes.
  - OOM blocks insertion but keeps navigation/save/exit available.

## Task 11 Final Regression Sweep

- Run all scenarios in this file.
- Expected:
  - Each scenario marked pass/fail with notes.
  - `README.md` behavior docs match implemented keybindings and limits.
