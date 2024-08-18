#include <common.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void remove_file(const char *filename) {
    short found = 0;
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_index[i].filename, filename) == 0) {
            for (int j = i; j < file_count - 1; j++) {
                file_index[j] = file_index[j + 1];
            }
            file_count--;
            found = 1;
            break;
        }
    }
    if (found) {
        printf("File %s removed successfully from file index.\n", filename);
    } else {
        printf("File %s not found in file index.\n", filename);
    }
}

void upload_file(const char *filepath) {
    if (access(filepath, F_OK) == -1) {
        printf("No file found at location %s.\n", filepath);
        return;
    }
    File file;
    strcpy(file.filepath, filepath);
    strcpy(file.filename, strrchr(filepath, '/') + 1);
    file_index[file_count++] = file;
    printf("File %s uploaded successfully to file index.\n", file.filename);
}

void search_file(const char *filename) {
}

void download_file(const char *filename, const char *peer_ip, const int peer_port) {
}
