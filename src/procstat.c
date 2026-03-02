#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

/*
    Simple Linux process monitor.
    Reads /proc directory and shows top processes by memory usage (VmRSS).
    Built for practicing:
    - directory handling (opendir/readdir)
    - file parsing
    - dynamic memory allocation
    - basic sorting with qsort
*/

typedef struct {
    int pid;
    char name[128];
    long memory_kb;
} ProcessInfo;


/* Check if folder name inside /proc is numeric (PIDs are numeric) */
int is_number(const char *str) {
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i]))
            return 0;
    }
    return 1;
}


/* Read VmRSS value from /proc/<pid>/status */
long read_memory(int pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");
    if (!file)
        return -1;  // process might have ended or permission denied

    char line[256];
    long memory = -1;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS:%ld kB", &memory);
            break;
        }
    }

    fclose(file);
    return memory;
}


/* Read process name from /proc/<pid>/comm */
void read_name(int pid, char *buffer, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    FILE *file = fopen(path, "r");
    if (!file) {
        strncpy(buffer, "unknown", size);
        return;
    }

    fgets(buffer, size, file);
    buffer[strcspn(buffer, "\n")] = '\0';  // remove newline
    fclose(file);
}


/* Sort processes by memory usage (descending order) */
int compare_memory(const void *a, const void *b) {
    ProcessInfo *p1 = (ProcessInfo *)a;
    ProcessInfo *p2 = (ProcessInfo *)b;

    return (int)(p2->memory_kb - p1->memory_kb);
}


int main() {
    DIR *dir = opendir("/proc");
    if (!dir) {
        printf("Cannot open /proc directory\n");
        return 1;
    }

    // Start with initial capacity for 100 processes
    int capacity = 100;
    int count = 0;

    ProcessInfo *list = malloc(sizeof(ProcessInfo) * capacity);
    if (!list) {
        printf("Initial memory allocation failed\n");
        closedir(dir);
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (!is_number(entry->d_name))
            continue;

        int pid = atoi(entry->d_name);
        long mem = read_memory(pid);

        if (mem <= 0)
            continue;

        // If capacity exceeded, resize (dynamic array behavior)
        if (count >= capacity) {
            capacity *= 2;

            ProcessInfo *temp = realloc(list, sizeof(ProcessInfo) * capacity);
            if (!temp) {
                printf("Reallocation failed\n");
                free(list);
                closedir(dir);
                return 1;
            }

            list = temp;
        }

        list[count].pid = pid;
        list[count].memory_kb = mem;
        read_name(pid, list[count].name, sizeof(list[count].name));
        count++;
    }

    closedir(dir);

    // Sort processes by memory usage
    qsort(list, count, sizeof(ProcessInfo), compare_memory);

    printf("\nTop 10 processes by memory usage:\n");
    printf("-----------------------------------\n");

    for (int i = 0; i < 10 && i < count; i++) {
        printf("PID: %d | %-20s | %ld kB\n",
               list[i].pid,
               list[i].name,
               list[i].memory_kb);
    }

    /*
        TODO:
        - Add CPU usage calculation
        - Add command line argument for top N
        - Improve error handling for edge cases
    */

    free(list);
    return 0;
}
