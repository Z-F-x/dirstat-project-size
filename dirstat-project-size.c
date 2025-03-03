#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <strings.h>  // for strcasecmp
#include <limits.h>   // for PATH_MAX
#include <ctype.h>    // for isdigit() and tolower()

// ANSI escape definitions
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"

// Maximum length for the bar chart (in characters)
// 100% is represented by 20 blocks.
#define MAX_BAR_LENGTH 20
#define NUM_STOPS 3  // Number of gradient stops for the bar

// Global flags
int useColor = 1;
int toggleAscii = 0;   // When true, use ASCII '#' for filled and '-' for empty
int onlyBarColor = 0;  // When true, text is not colored except the bars



// ---------------------------------------------------------------------------
// Structures
// ---------------------------------------------------------------------------
typedef struct {
    long long bytes;   // total bytes
    long long lines;   // total lines (number of '\n')
    long long chars;   // total characters read
} FileStats;

typedef struct {
    long numFiles;
    long numDirs;
    FileStats stats;
} ProjectStats;

typedef struct {
    char ext[64];      // file extension (e.g., "c", "cpp", "py")
    long count;
} ExtCount;



// ---------------------------------------------------------------------------
// Function Declarations
// ---------------------------------------------------------------------------
FileStats get_file_stats(const char *filepath);
void update_extension_counts(const char *filename, ExtCount **extCounts, int *extCount, int *extCapacity);
void process_path(const char *path, ProjectStats *projStats, ExtCount **extCounts, int *extCount, int *extCapacity, const char **excludes, int num_excludes);
int compare_ext_desc(const void *a, const void *b);
void print_bar(double percentage, const char *color);
void get_gradient_color(int rank, int total, char *buffer, size_t buflen);
int is_excluded(const char *path, const char **excludes, int num_excludes);
void print_help(void);


// Add to global variables
typedef enum {
    SORT_DESCENDING = 0,    // Default
    SORT_ASCENDING,
    SORT_ALPHA_ASC,
    SORT_ALPHA_DESC,
    SORT_NUM_ASC,
    SORT_NUM_DESC,
    SORT_NATURAL_ASC,
    SORT_NATURAL_DESC,
    SORT_SIZE_ASC,
    SORT_SIZE_DESC
} SortType;

SortType sortType = SORT_DESCENDING;
int caseSensitive = 0;  // 0 for case-insensitive, 1 for case-sensitive

// New comparison functions
int compare_ext_asc(const void *a, const void *b) {
    const ExtCount *ea = (const ExtCount*) a;
    const ExtCount *eb = (const ExtCount*) b;
    return caseSensitive ? strcmp(ea->ext, eb->ext) : strcasecmp(ea->ext, eb->ext);
}

int compare_ext_num_asc(const void *a, const void *b) {
    const ExtCount *ea = (const ExtCount*) a;
    const ExtCount *eb = (const ExtCount*) b;
    if (ea->count == eb->count)
        return caseSensitive ? strcmp(ea->ext, eb->ext) : strcasecmp(ea->ext, eb->ext);
    return (ea->count > eb->count) ? 1 : -1;
}

int compare_ext_num_desc(const void *a, const void *b) {
    return compare_ext_num_asc(b, a);
}

// Simple natural sort comparison (basic implementation)
int strnatcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (isdigit(*a) && isdigit(*b)) {
            long na = 0, nb = 0;
            while (isdigit(*a)) na = na * 10 + (*a++ - '0');
            while (isdigit(*b)) nb = nb * 10 + (*b++ - '0');
            if (na != nb) return (na > nb) ? 1 : -1;
        } else {
            if (tolower(*a) != tolower(*b))
                return caseSensitive ? (*a - *b) : (tolower(*a) - tolower(*b));
            a++; b++;
        }
    }
    return caseSensitive ? (*a - *b) : (tolower(*a) - tolower(*b));
}

int compare_ext_natural_asc(const void *a, const void *b) {
    const ExtCount *ea = (const ExtCount*) a;
    const ExtCount *eb = (const ExtCount*) b;
    return strnatcmp(ea->ext, eb->ext);
}

int compare_ext_natural_desc(const void *a, const void *b) {
    return strnatcmp(((const ExtCount*)b)->ext, ((const ExtCount*)a)->ext);
}

// ---------------------------------------------------------------------------
// Print help message and exit.
// ---------------------------------------------------------------------------
/* printf("  -h, --h, -H, --H, --HH, -help, --help, -HELP\n"); */
/*     printf("      Display this help message\n"); */
/*     printf("  --no-color\n"); */
/*     printf("      Disable colorized output\n"); */
/*     printf("  --toggle-ascii\n"); */
/*     printf("      Use ASCII characters (# for filled, - for empty) instead of Unicode blocks\n"); */
/*     printf("  --only-bar-color\n"); */
/*     printf("      Turn off colors for all text, but keep colored bars\n"); */
/*     printf("  --exclude=pattern\n"); */
/*     printf("      Exclude any file or folder whose path contains the given pattern\n"); */
/*     printf("\nExamples:\n"); */
/*     printf("  dirstat-project-size /path/to/project\n"); */
/*     printf("  dirstat-project-size /path/to/project --exclude=node_modules --exclude=.git\n"); */
/*     printf("  dirstat-project-size --toggle-ascii\n"); */
/*     printf("  dirstat-project-size --only-bar-color\n"); */
/*     exit(0); */
/* } */

// Modified print_help function
void print_help(void) {
    printf("Usage: dirstat-project-size [directory] [options]\n");
    printf("Options:\n");
    printf("  -h, --help           Display this help message\n");
    printf("  --no-color          Disable colorized output\n");
    printf("  --toggle-ascii      Use ASCII instead of Unicode blocks\n");
    printf("  --only-bar-color    Color only bars, not text\n");
    printf("  --exclude=pattern   Exclude paths containing pattern\n");
    printf("Sorting Options:\n");
    printf("  --sort-descending   Sort by count descending (default)\n");
    printf("  --sort-ascending    Sort by count ascending\n");
    printf("  --sort-alpha-asc    Sort alphabetically A-Z\n");
    printf("  --sort-alpha-desc   Sort alphabetically Z-A\n");
    printf("  --sort-num-asc      Sort numerically ascending\n");
    printf("  --sort-num-desc     Sort numerically descending\n");
    printf("  --sort-natural-asc  Sort with natural ordering ascending\n");
    printf("  --sort-natural-desc Sort with natural ordering descending\n");
    printf("  --case-sensitive    Make alphabetical sorting case-sensitive\n");
    printf("\nExamples:\n");
    printf("  dirstat-project-size /path/to/project --sort-alpha-asc\n");
    printf("  dirstat-project-size --sort-natural-desc --case-sensitive\n");
    exit(0);
}

// ---------------------------------------------------------------------------
// Check if the given path matches any of the exclude patterns.
// ---------------------------------------------------------------------------
int is_excluded(const char *path, const char **excludes, int num_excludes) {
    for (int i = 0; i < num_excludes; i++) {
        if (strstr(path, excludes[i]) != NULL)
            return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Count lines & characters for a single file
// ---------------------------------------------------------------------------
FileStats get_file_stats(const char *filepath) {
    FileStats s = {0, 0, 0};
    FILE *fp = fopen(filepath, "rb");
    if (!fp)
        return s;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        s.bytes++;
        s.chars++;
        if (c == '\n')
            s.lines++;
    }
    fclose(fp);
    return s;
}

// ---------------------------------------------------------------------------
// Update dynamic extension count list based on filename
// ---------------------------------------------------------------------------
void update_extension_counts(const char *filename, ExtCount **extCounts, int *extCount, int *extCapacity) {
    const char *dot = strrchr(filename, '.');
    char extension[64];
    if (dot && dot != filename) {
        strncpy(extension, dot + 1, sizeof(extension));
        extension[sizeof(extension) - 1] = '\0';
    } else {
        strcpy(extension, "no_ext");
    }
    for (int i = 0; i < *extCount; i++) {
        if (strcmp((*extCounts)[i].ext, extension) == 0) {
            (*extCounts)[i].count++;
            return;
        }
    }
    if (*extCount >= *extCapacity) {
        *extCapacity = (*extCapacity == 0) ? 8 : (*extCapacity * 2);
        *extCounts = realloc(*extCounts, (*extCapacity) * sizeof(ExtCount));
        if (!*extCounts) {
            perror("realloc");
            exit(1);
        }
    }
    strncpy((*extCounts)[*extCount].ext, extension, sizeof((*extCounts)[*extCount].ext));
    (*extCounts)[*extCount].ext[sizeof((*extCounts)[*extCount].ext) - 1] = '\0';
    (*extCounts)[*extCount].count = 1;
    (*extCount)++;
}

// ---------------------------------------------------------------------------
// Recursively process a file or directory path.
// Excludes any path containing one of the provided patterns.
// ---------------------------------------------------------------------------
void process_path(const char *path, ProjectStats *projStats, ExtCount **extCounts, int *extCount, int *extCapacity, const char **excludes, int num_excludes) {
    if (is_excluded(path, excludes, num_excludes))
        return;
    struct stat st;
    if (stat(path, &st) != 0)
        return;
    if (S_ISREG(st.st_mode)) {
        projStats->numFiles++;
        FileStats fstats = get_file_stats(path);
        projStats->stats.bytes += fstats.bytes;
        projStats->stats.lines += fstats.lines;
        projStats->stats.chars += fstats.chars;
        const char *filename = strrchr(path, '/');
        if (filename)
            filename++; // Skip '/'
        else
            filename = path;
        update_extension_counts(filename, extCounts, extCount, extCapacity);
    } else if (S_ISDIR(st.st_mode)) {
        projStats->numDirs++;
        DIR *dir = opendir(path);
        if (!dir)
            return;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char subpath[2048];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            process_path(subpath, projStats, extCounts, extCount, extCapacity, excludes, num_excludes);
        }
        closedir(dir);
    }
}

// ---------------------------------------------------------------------------
// Comparison function for sorting extensions in descending order (highest % first)
// ---------------------------------------------------------------------------
int compare_ext_desc(const void *a, const void *b) {
    const ExtCount *ea = (const ExtCount*) a;
    const ExtCount *eb = (const ExtCount*) b;
    if (ea->count < eb->count)
        return 1;
    else if (ea->count > eb->count)
        return -1;
    else
        return strcmp(ea->ext, eb->ext);
}

// ---------------------------------------------------------------------------
// Print a horizontal percentage bar in the format:
// [████████▒▒▒▒▒▒▒▒▒▒▒▒]  40.00%
// The filled part uses the gradient color, while the empty part is static gray.
// When --toggle-ascii is used, '#' replaces '█' and '-' replaces '▒'.
// ---------------------------------------------------------------------------
void print_bar(double percentage, const char *color) {
    int filledLength = (int)((percentage / 100.0) * MAX_BAR_LENGTH);
    if (percentage > 0 && filledLength == 0)
        filledLength = 1;
    int emptyLength = MAX_BAR_LENGTH - filledLength;
    printf("[");
    for (int i = 0; i < filledLength; i++) {
        if (useColor) {
            if (toggleAscii)
                printf("%s#", color);
            else
                printf("%s█", color);
        } else {
            if (toggleAscii)
                printf("#");
            else
                printf("█");
        }
    }
    // Print empty portion in static gray (RGB 128,128,128) if color enabled.
    if (useColor)
        printf("\033[38;2;128;128;128m");
    for (int i = 0; i < emptyLength; i++) {
        if (toggleAscii)
            printf("-");
        else
            printf(" "); //▒
    }
    if (useColor)
        printf(ANSI_RESET);
    printf("]");
    printf(" %6.2f%%", percentage);
}

// ---------------------------------------------------------------------------
// Compute a gradient color (ANSI true-color code) for a given rank in the list.
// The gradient spans from #35FE09 (highest) through #2C9918 to #2A5322 (lowest).
// ---------------------------------------------------------------------------
void get_gradient_color(int rank, int total, char *buffer, size_t buflen) {
    int stops[NUM_STOPS][3] = {
        {53, 254, 9},    // #35FE09
        {44, 153, 24},   // #2C9918
        {42, 83, 34}     // #2A5322
    };
    double t = (total > 1) ? ((double) rank / (total - 1)) : 0.0;
    double pos = t * (NUM_STOPS - 1);
    int seg = (int) pos;
    if (seg >= NUM_STOPS - 1) {
        seg = NUM_STOPS - 2;
        pos = NUM_STOPS - 1;
    }
    double frac = pos - seg;
    int r = (int)(((1 - frac) * stops[seg][0]) + (frac * stops[seg+1][0]) + 0.5);
    int g = (int)(((1 - frac) * stops[seg][1]) + (frac * stops[seg+1][1]) + 0.5);
    int b = (int)(((1 - frac) * stops[seg][2]) + (frac * stops[seg+1][2]) + 0.5);
    snprintf(buffer, buflen, "\033[38;2;%d;%d;%dm", r, g, b);
}

// ---------------------------------------------------------------------------
// Main: compute and output overall project statistics with a gradient table.
// Supports excluding files/folders with --exclude=pattern and various help options.
// Also supports --toggle-ascii to use ASCII characters instead of Unicode blocks,
// and --only-bar-color to disable text color for all output except the bars.
// Also, if no directory is provided, uses the absolute current directory name.
// ---------------------------------------------------------------------------
#define MAX_EXCLUDES 100


int main(int argc, char *argv[]) {
    char *root = NULL;
    char cwd[PATH_MAX];
    const char *excludes[MAX_EXCLUDES];
    int num_excludes = 0;
    
    // First pass: Check if there's a directory argument (not starting with '-')
    int dir_arg_index = -1;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            dir_arg_index = i;
            break;
        }
    }

    // Set root directory
    if (dir_arg_index != -1) {
        root = strdup(argv[dir_arg_index]);
    } else {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            root = strdup(cwd);
        } else {
            perror("getcwd() error");
            root = strdup(".");
        }
    }

    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        if (i == dir_arg_index) continue; // Skip the directory argument
        if (argv[i][0] == '-') {
            // Check for help variants (case-insensitive)
            if (strcasecmp(argv[i], "-h") == 0 ||
                strcasecmp(argv[i], "--h") == 0 ||
                strcasecmp(argv[i], "-help") == 0 ||
                strcasecmp(argv[i], "--help") == 0 ||
                strcasecmp(argv[i], "-hh") == 0 ||
                strcasecmp(argv[i], "--hh") == 0) {
                print_help();
            }
            else if (strcmp(argv[i], "--no-color") == 0)
                useColor = 0;
            else if (strcmp(argv[i], "--toggle-ascii") == 0)
                toggleAscii = 1;
            else if (strcmp(argv[i], "--only-bar-color") == 0)
                onlyBarColor = 1;
            else if (strncmp(argv[i], "--exclude=", 10) == 0) {
                if (num_excludes < MAX_EXCLUDES)
                    excludes[num_excludes++] = argv[i] + 10;
            }
            else if (strcmp(argv[i], "--sort-descending") == 0)
                sortType = SORT_DESCENDING;
            else if (strcmp(argv[i], "--sort-ascending") == 0)
                sortType = SORT_ASCENDING;
            else if (strcmp(argv[i], "--sort-alpha-asc") == 0)
                sortType = SORT_ALPHA_ASC;
            else if (strcmp(argv[i], "--sort-alpha-desc") == 0)
                sortType = SORT_ALPHA_DESC;
            else if (strcmp(argv[i], "--sort-num-asc") == 0)
                sortType = SORT_NUM_ASC;
            else if (strcmp(argv[i], "--sort-num-desc") == 0)
                sortType = SORT_NUM_DESC;
            else if (strcmp(argv[i], "--sort-natural-asc") == 0)
                sortType = SORT_NATURAL_ASC;
            else if (strcmp(argv[i], "--sort-natural-desc") == 0)
                sortType = SORT_NATURAL_DESC;
            else if (strcmp(argv[i], "--case-sensitive") == 0)
                caseSensitive = 1;
        }
    }
    
    ProjectStats projStats = {0, 0, {0, 0, 0}};
    ExtCount *extCounts = NULL;
    int extCount = 0, extCapacity = 0;
    
    // Process the directory
    process_path(root, &projStats, &extCounts, &extCount, &extCapacity, excludes, num_excludes);
    
    // Sort based on selected sort type
    if (extCount > 0) {
        switch (sortType) {
            case SORT_DESCENDING:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_desc);
                break;
            case SORT_ASCENDING:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_num_asc);
                break;
            case SORT_ALPHA_ASC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_asc);
                break;
            case SORT_ALPHA_DESC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_natural_desc);
                break;
            case SORT_NUM_ASC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_num_asc);
                break;
            case SORT_NUM_DESC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_num_desc);
                break;
            case SORT_NATURAL_ASC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_natural_asc);
                break;
            case SORT_NATURAL_DESC:
                qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_natural_desc);
                break;
            // Note: SORT_SIZE_ASC and SORT_SIZE_DESC are defined but not implemented
            // as we don't track size per extension yet
        }
    }
    
    // If --only-bar-color is specified, disable text color for headers
    const char *headerColor = (useColor && !onlyBarColor) ? (ANSI_BOLD "\033[38;2;101;152;75m") : "";
    const char *resetColor = useColor ? ANSI_RESET : "";
    
    // Print overall statistics
    printf("\nProject Statistics for directory: %s\n", root);
    printf("--------------------------------------------------------------\n");
    printf("%sTotal number of folders:%s %ld\n", headerColor, resetColor, projStats.numDirs);
    printf("%sTotal number of files  :%s %ld\n", headerColor, resetColor, projStats.numFiles);
    double totalMB = projStats.stats.bytes / (1024.0 * 1024.0);
    printf("%sTotal project size     :%s %.2f MB\n", headerColor, resetColor, totalMB);
    printf("%sTotal lines of code    :%s %lld\n", headerColor, resetColor, projStats.stats.lines);
    printf("%sTotal characters       :%s %lld\n", headerColor, resetColor, projStats.stats.chars);
    
    // Print table header
    printf("\n%-12s %8s   %s\n", "Type", "Count", "Bar");
    printf("--------------------------------------------------------------\n");
    
    // Print file type table rows
    for (int i = 0; i < extCount; i++) {
        double percentage = (projStats.numFiles > 0) ? ((extCounts[i].count * 100.0) / projStats.numFiles) : 0.0;
        char gradColor[32] = "";
        if (useColor)
            get_gradient_color(i, extCount, gradColor, sizeof(gradColor));
        char extDisplay[16];
        if (strcmp(extCounts[i].ext, "no_ext") == 0)
            snprintf(extDisplay, sizeof(extDisplay), "%s", extCounts[i].ext);
        else
            snprintf(extDisplay, sizeof(extDisplay), ".%s", extCounts[i].ext);
        printf("%s%-12s%s %8ld   ", headerColor, extDisplay, resetColor, extCounts[i].count);
        print_bar(percentage, gradColor);
        printf("\n");
    }
    
    free(extCounts);
    free(root);
    return 0;
}
