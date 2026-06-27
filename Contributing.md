# PrimedGun Contributing Guide

PrimedGun is a Dolphin ReduX/Dolphin-derived project. Contributions should keep
the repository legal, buildable, and easy to review.

## Legal Requirements

### Licensing

By contributing code, documentation, assets, or other material to PrimedGun, you
agree that your contribution may be distributed under GPL-2.0-or-later unless the
contribution clearly belongs under another license that is compatible with the
existing repository.

Do not remove SPDX headers, copyright notices, `COPYING`, or files in
`LICENSES/`. If you add third-party code or assets, include the source, license,
and attribution information in the same change.

### Clean Source Requirement

Do not contribute code, offsets, symbols, comments, assets, or documentation that
comes from leaked, confidential, NDA-restricted, or otherwise unauthorized
Nintendo, GameCube, Wii, Triforce, Retro Studios, or Metroid Prime material.

Reverse engineering, debugging, decompilation references, and runtime research
must come from sources that you are legally allowed to use.

### No Piracy

Do not commit commercial game files, BIOS files, keys, firmware, proprietary SDK
files, copyrighted save data, or memory cards containing copyrighted content.

PrimedGun does not ship Metroid Prime. Users must provide their own legally
obtained game file and saves.

## Development Expectations

- Keep changes scoped to the problem being solved.
- Preserve the Dolphin-style code layout unless there is a clear reason to
  change it.
- Use the existing CMake build paths and project structure.
- Prefer focused fixes over broad refactors.
- Keep runtime hooks and memory writes conservative, documented, and easy to
  audit.
- Test Windows builds when changing desktop/runtime behavior.
- For Linux changes, keep the README build instructions current.

## Pull Requests

- Explain what changed and why.
- Mention any tested build target.
- Include screenshots or RAM dump context for visual/runtime fixes when useful.
- Do not mix unrelated cleanup with risky runtime hook changes.

Small, reviewable changes are much easier to merge safely.
