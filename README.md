# dirstat-project-size

## Show statistics of current directory
The idea is to get a quick overview of a new codebase to assess the project size, history and scope.

## Installation

### Prebuilt binaries
Ready-to-run binaries live in [`bin/`](bin/):

| Platform | File |
|---|---|
| macOS (Apple Silicon) | `bin/dirstat-project-size-macos-arm64` |
| Linux x86_64 (Arch, etc.) | `bin/dirstat-project-size-linux-x86_64` |
| Windows x86_64 | `bin/dirstat-project-size-windows-x86_64.exe` |

On macOS/Linux, copy it somewhere on your `PATH` and make it executable:
```sh
install -m755 bin/dirstat-project-size-macos-arm64 ~/.local/bin/dirstat-project-size
```
macOS may quarantine downloaded binaries; if blocked, run `xattr -d com.apple.quarantine <file>` or build from source (it's one file).

### Build from source

#### Arch Linux (and other Linux distros)
```sh
sudo pacman -S --needed base-devel   # gcc; on Debian/Ubuntu: sudo apt install build-essential
git clone https://github.com/Z-F-x/dirstat-project-size.git
cd dirstat-project-size
gcc -O2 -o dirstat-project-size dirstat-project-size.c
sudo install -m755 dirstat-project-size /usr/local/bin/
```

#### macOS
```sh
xcode-select --install               # once, for the C compiler
git clone https://github.com/Z-F-x/dirstat-project-size.git
cd dirstat-project-size
cc -O2 -o dirstat-project-size dirstat-project-size.c
install -m755 dirstat-project-size ~/.local/bin/   # or /usr/local/bin
```

#### Windows
Use a MinGW-w64 toolchain, e.g. [MSYS2](https://www.msys2.org/) (`pacman -S mingw-w64-ucrt-x86_64-gcc`) or [w64devkit](https://github.com/skeeto/w64devkit):
```sh
gcc -O2 -o dirstat-project-size.exe dirstat-project-size.c
```
Put the `.exe` in a folder on your `PATH`. (MSVC is not supported: the code uses POSIX `dirent.h`.)

## Usage

### Options:
  `-h`, `--help`           Display this help message\
  `--no-color`          Disable colorized output\
  `--toggle-ascii`      Use ASCII instead of Unicode blocks\
  `--only-bar-color`    Color only bars, not text\
  `--exclude=pattern`   Exclude paths containing pattern\
  `--dirs`              List immediate subdirectories ranked by total size\
  `--fast`              Sizes from stat only; skips line/char counting (much faster on big trees)\
  `--all-dirs[=N]`      Rank every directory at every depth by size; show top N (default 50)

### Sorting Options:
  `--sort-descending`   Sort by count descending (default)\
  `--sort-ascending`    Sort by count ascending\
  `--sort-alpha-asc`    Sort alphabetically A-Z\
  `--sort-alpha-desc`   Sort alphabetically Z-A\
  `--sort-num-asc`      Sort numerically ascending\
  `--sort-num-desc`     Sort numerically descending\
  `--sort-natural-asc`  Sort with natural ordering ascending\
  `--sort-natural-desc` Sort with natural ordering descending\
  `--case-sensitive`    Make alphabetical sorting case-sensitive

### Examples and explanation
#### General Usage and Options
- ```dirstat-project-size /path/to/project```\
Runs the program in the specified directory (e.g., /path/to/project), analyzing all files and subdirectories within it. Provides a breakdown of file types, counts, and project statistics for that specific location instead of the current directory.

- ```dirstat-project-size --exclude=node_modules --exclude=.git```\
Excludes specific folders or files matching the given patterns (e.g., node_modules and .git) from the analysis. Skips these paths entirely, ensuring the statistics focus only on relevant project files and not dependencies or version control data.

- ```dirstat-project-size --toggle-ascii```\
Uses ASCII characters (# for filled, - for empty) instead of Unicode blocks (█ and ▒) for the bar chart display. Simplifies the output for terminals that don’t support Unicode or for a more minimalist presentation.

- ```dirstat-project-size --only-bar-color```\
Applies color only to the percentage bars in the output, leaving all other text (headers, counts, etc.) in plain format. Keeps the visual emphasis on the bars while maintaining readable, uncolored text elsewhere.

- ```dirstat-project-size --dirs```\
Adds a table of the immediate subdirectories ranked by their total size on disk, with a size column in MB and percentage bars. Useful for finding what is taking up space.

- ```dirstat-project-size --dirs --fast ~```\
Fast mode takes sizes from file metadata instead of reading file contents, skipping the line and character counts. Recommended for very large trees (a whole home directory scans in a couple of minutes instead of hours). A live file counter is shown on the terminal while scanning.

- ```dirstat-project-size --all-dirs --fast ~```\
Ranks every directory at every depth by its total recursive size and shows the biggest ones (top 50 by default, `--all-dirs=200` for more). Because a parent always weighs at least as much as its children, the list reads as a drill-down chain — follow it to find exactly where the space is going in one run.

##### Sorting Options
- ```dirstat-project-size /path/to/project --sort-alpha-asc```\
Sorts the list of file extensions alphabetically from A to Z (e.g., .c before .py). Organizes the output in a familiar, dictionary-like order based on extension names, making it easy to locate specific file types.

- ```dirstat-project-size /path/to/project --sort-alpha-desc```\
Sorts the list of file extensions alphabetically from Z to A (e.g., .py before .c). Reverses the alphabetical order, placing extensions starting with later letters at the top of the table.

- ```dirstat-project-size /path/to/project --sort-ascending```\
Sorts the list of file extensions by count in ascending order (smallest to largest, e.g., 1 file before 10 files). Highlights less common file types first, with more frequent ones appearing lower in the table.

- ```dirstat-project-size /path/to/project --sort-descending```\
Sorts the list of file extensions by count in descending order (largest to smallest, e.g., 10 files before 1 file). Prioritizes the most common file types at the top (default behavior), emphasizing the project’s dominant file categories.

- ```dirstat-project-size /path/to/project --sort-num-asc```\
Sorts the list of file extensions numerically by count in ascending order (e.g., 1 before 10), with ties broken alphabetically. Ensures a consistent numerical progression, useful for focusing on rarity first.

- ```dirstat-project-size /path/to/project --sort-num-desc```\
Sorts the list of file extensions numerically by count in descending order (e.g., 10 before 1), with ties broken alphabetically. Places the most numerous file types at the top, similar to --sort-descending but with explicit numerical priority.

- ```dirstat-project-size /path/to/project --sort-natural-asc```\
Sorts the list of file extensions in a “human-friendly” way, considering numbers within extension names (e.g., .file2 before .file10). Mimics how people naturally order mixed alphanumeric strings, avoiding strict alphabetical pitfalls.

- ```dirstat-project-size /path/to/project --sort-natural-desc```\
Sorts the list of file extensions in reverse “human-friendly” order (e.g., .file10 before .file2). Applies natural sorting in descending order, placing higher-numbered or later extensions first in a way that feels intuitive.

- ```dirstat-project-size /path/to/project --sort-natural-desc --case-sensitive```\
Combines natural descending sort with case-sensitive comparison (e.g., .C before .c if present). Ensures that uppercase letters are prioritized over lowercase ones within the natural sort, adding precision to extension name ordering.

#### Additional Notes
- ```--case-sensitive```\
Modifies alphabetical and natural sorting to distinguish between uppercase and lowercase letters (e.g., .C before .c instead of treating them as equal). Adds granularity to sorting when case matters in your project’s file naming conventions.

```-h``` or ```--help```\
Displays the help message with all available options and examples. Stops execution and provides a quick reference for usage, perfect for learning or troubleshooting the tool.


## Example Usage:
```dirstat-project-size /path/to/project``` Run in a specific path \
```dirstat-project-size --exclude=node_modules --exclude=.git``` Exclude specific folders\
```dirstat-project-size --toggle-ascii``` Use ascii characters for the bar chart instead of unicode\
```dirstat-project-size --only-bar-color``` Only colorize the bars \
```dirstat-project-size /path/to/project --sort-alpha-asc``` Sort the list alfabetically from Z to A\
```dirstat-project-size --sort-natural-desc --case-sensitive```Sort by smal distinguishing between uppercase and lowercase letters

```dirstat-project-size  --sort-descending --only-bar-color```

![image](https://github.com/user-attachments/assets/288a7dff-27b2-4292-97c8-6d172c76ee7d)

Result:

![image](https://github.com/user-attachments/assets/11652dba-ebd3-4fbd-a347-a7cb266baf9b)


