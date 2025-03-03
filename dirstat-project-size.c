
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// ANSI escape definitions
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"

// Maximum length for the bar chart (in characters)
#define MAX_BAR_LENGTH 50
#define NUM_STOPS 3  // Number of gradient stops for the bar

// Global flag for color output (enabled by default)
int useColor = 1;

// ---------------------------------------------------------------------------
// Structures
// ---------------------------------------------------------------------------
typedef struct {
    long long bytes;  // total bytes
    long long lines;  // total lines (number of '\n')
    long long chars;  // total characters read
} FileStats;

typedef struct {
    long numFiles;
    long numDirs;
    FileStats stats;
} ProjectStats;

typedef struct {
    char ext[64];    // file extension (e.g., "c", "cpp", "py")
    long count;
} ExtCount;

// ---------------------------------------------------------------------------
// Function Declarations
// ---------------------------------------------------------------------------
FileStats get_file_stats(const char *filepath);
void update_extension_counts(const char *filename, ExtCount **extCounts, int *extCount, int *extCapacity);
void process_path(const char *path, ProjectStats *projStats, ExtCount **extCounts, int *extCount, int *extCapacity);
int compare_ext_desc(const void *a, const void *b);
void print_bar(double percentage, const char *color);
void get_gradient_color(int rank, int total, char *buffer, size_t buflen);

// ---------------------------------------------------------------------------
// Count lines & characters for a single file
// ---------------------------------------------------------------------------
FileStats get_file_stats(const char *filepath) {
    FileStats s = {0, 0, 0};
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return s;
    }
    int c;
    while ((c = fgetc(fp)) != EOF) {
        s.bytes++;
        s.chars++;
        if (c == '\n') {
            s.lines++;
        }
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
// Recursively process a file or directory path
// ---------------------------------------------------------------------------
void process_path(const char *path, ProjectStats *projStats, ExtCount **extCounts, int *extCount, int *extCapacity) {
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
            process_path(subpath, projStats, extCounts, extCount, extCapacity);
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
// Print a horizontal bar (fixed-width column) representing the percentage.
// The bar is always MAX_BAR_LENGTH characters wide.
// ---------------------------------------------------------------------------
void print_bar(double percentage, const char *color) {
    int filledLength = (int)((percentage / 100.0) * MAX_BAR_LENGTH);
    if (percentage > 0 && filledLength == 0)
        filledLength = 1;
    for (int i = 0; i < filledLength; i++) {
        if (useColor)
            printf("%s█", color);
        else
            printf("█");
    }
    for (int i = filledLength; i < MAX_BAR_LENGTH; i++) {
        printf(" ");
    }
    if (useColor)
        printf(ANSI_RESET);
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
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    const char *root = ".";
    int sortOrder = -1; // Force descending sort for gradient mapping.

    // Process command-line arguments.
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0) {
            if (strcmp(argv[i], "--no-color") == 0)
                useColor = 0;
        } else {
            root = argv[i];
        }
    }
    
    ProjectStats projStats = {0, 0, {0, 0, 0}};
    ExtCount *extCounts = NULL;
    int extCount = 0, extCapacity = 0;
    
    process_path(root, &projStats, &extCounts, &extCount, &extCapacity);
    
    // Sort file types descending by count.
    if (extCount > 0)
        qsort(extCounts, extCount, sizeof(ExtCount), compare_ext_desc);
    
    // Set text color for headers and statistics using #65984B (RGB 101,152,75).
    const char *headerColor = useColor ? (ANSI_BOLD "\033[38;2;101;152;75m") : "";
    const char *statColor   = useColor ? "\033[38;2;101;152;75m" : "";
    const char *resetColor  = useColor ? ANSI_RESET : "";
    
    // Print overall statistics.
    printf("%sProject Statistics for directory: %s%s\n", headerColor, root, resetColor);
    printf("%s-----------------------------------------------------%s\n", useColor ? "\033[38;2;101;152;75m" : "", resetColor);
    printf("Total number of folders: %ld\n", projStats.numDirs);
    printf("Total number of files  : %ld\n", projStats.numFiles);
    double totalMB = projStats.stats.bytes / (1024.0 * 1024.0);
    printf("Total project size     : %.2f MB\n", totalMB);
    printf("Total lines of code    : %lld\n", projStats.stats.lines);
    printf("Total characters       : %lld\n", projStats.stats.chars);
    
    // Print file type table rows in the desired format.
    printf("\n");
    for (int i = 0; i < extCount; i++) {
        double percentage = (projStats.numFiles > 0) ? ((extCounts[i].count * 100.0) / projStats.numFiles) : 0.0;
        char gradColor[32] = "";
        if (useColor)
            get_gradient_color(i, extCount, gradColor, sizeof(gradColor));
        // Format: "<extension>   <count> files (<percentage>%)  <bar>"
        printf("%-10s %8ld files (%.2f%%)  ", extCounts[i].ext, extCounts[i].count, percentage);
        print_bar(percentage, gradColor);
        printf("\n");
    }
    
    free(extCounts);
    return 0;
}
