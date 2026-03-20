# Aarn

Terminal file shell with a built-in preview mode.

## Build

```sh
make
```

Debug build with sanitizers:

```sh
make debug
```

## Commands

- `pwd`
- `cd [path]`
- `ls [-a] [path...]`
- `cat [-h] file...`
- `touch [-a] [-m] [-t STAMP] file...`
- `mkdir [-p] [-v] [-m MODE] dir...`
- `rm [-f] [-i] [-I] [-r|-R] [-v] path...`
- `clear`
- `preview`
- `exit`

Quoted arguments and escaped spaces are supported, for example:

```sh
mkdir "dir with spaces"
cat some\ file.txt
```

## Preview Mode

- `Up` / `Down`: move selection
- `Left` / `Backspace`: go to parent directory
- `Right` / `Enter`: open directory
- `Enter` on text file: open viewer
- `Space`: mark or unmark item
- `:`: search
- `a` / `A`: sort by name ascending / descending
- `d` / `D`: sort by date ascending / descending
- `s`: refresh view
- `q`: leave preview

Viewer mode:

- `Up` / `Down`: scroll file
- `q` or `Esc`: close viewer

## Stability Notes

- Terminal state is restored on normal exit and common termination signals.
- Preview selection is tied to files, not list positions, so sorting and filtering do not corrupt marked items.
- Binary and control characters are sanitized before printing to avoid terminal artifacts.
