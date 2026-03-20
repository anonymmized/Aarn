# Issue 0001: Preview Polish And Marked Workflow

Status: Closed

## Summary

Current `preview` mode needs a full quality pass:

- fix frame rendering and right-edge clipping
- replace ASCII separators with continuous box-drawing UI
- improve marked mode so it feels like a real workflow, not a temporary state
- reduce visual clutter in the status area
- make viewer/search/marked transitions predictable
- remove laggy confirmation flow inside preview interactions

## Resolution

Completed in the current patch set:

- reworked preview layout into a framed TUI pane with dedicated content and status zones
- replaced frame drawing with continuous box-drawing characters
- restructured status/help lines into a cleaner two-line footer
- expanded marked mode with `m`, `u`, `i`, `x`, `Esc`
- changed delete confirmation inside preview to single-key `y/n/esc`
- fixed viewer/search/marked mode transitions and state restoration
- hardened preview rendering and edge handling near terminal borders

## Closing Comment

Closed after implementation and TTY verification.

Verified:

- `make all`
- interactive launch of `preview`
- marked actions (`m/u/i/x`)
- single-key confirmation path
- viewer enter/exit flow
