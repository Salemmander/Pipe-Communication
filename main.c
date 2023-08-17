#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define BUFFER_SIZE 256
// Locking variables
int turn;
int flag[2];
int turn2;
int flag2[2];

char *readFilesInDirectory(const char *directoryPath);

char *getDirectoryFileNames(const char *directory);

int createFileInDirectory(const char *directory, const char *filename, const char *fileContents);

void lock_init();

void lock(int self); 

void unlock(int self);

void lock_init2();

void lock2(int self);

void unlock2(int self);

int main() {

//  Variables that contain the directory names for the children
    char childOneDir[] = "d1";
    char childTwoDir[] = "d2";

//  Buffers for the pipes
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];

//  Creating pipes 1 and 2
    int pipe1[2]; // pipe1[0] - Read end for child 2 - pipe1[1] Write end for child 1
    int pipe2[2]; // pipe2[0] - Read end for child 1 - pipe2[1] Write end for child 2
    pid_t p1, p2;

//  Creating the pipes and checking if they fail to open
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        fprintf(stderr, "Failed to open a pipe");
        exit(EXIT_FAILURE);
    }

//  Create first child process
    lock_init();
    lock_init2();

    p1 = fork();
//  Check if process 1 failed
    if (p1 < 0) {
        printf("Fork for p1 failed.\n");
        exit(EXIT_FAILURE);
    }
//  PROCESS 1
    else if (p1 == 0) {
        close(pipe1[0]); // Close read end of pipe 1
        close(pipe2[1]); // Close write end of pipe 2

        // Getting Files Contents
        char *fileContent = readFilesInDirectory(childOneDir);
        //printf("%s\n", fileContent);
        // Get File Names
        char *files = getDirectoryFileNames(childOneDir);
        //printf("%s\n", files);
        // Concatenate into one string
        strcat(files, fileContent);
        //printf("%s\n", files);
        lock(0);
        write(pipe1[1], files, strlen(files) + 1);
        unlock(0);

        lock2(0);
        read(pipe2[0], buffer2, BUFFER_SIZE);
        unlock2(0);

        //printf("BUFFER: %s\n", buffer2);

        const char *fileName1 = strtok(buffer2, "\n");
        //printf("NAME: %s\n", fileName1);
        const char *fileName2 = strtok(NULL, "\n");
        //printf("NAME: %s\n", fileName2);
        const char *fileName3 = strtok(NULL, "\n");
        //printf("NAME: %s\n\n", fileName3);
        const char *fileContent1 = strtok(NULL, "\n");
        //printf("%s\n", fileContent1);
        const char *fileContent2 = strtok(NULL, "\n");
        //printf("%s\n", fileContent2);
        const char *fileContent3 = strtok(NULL, "\n");
        //printf("%s\n", fileContent3);
 
        
        
        createFileInDirectory(childOneDir, fileName1, fileContent1);
        createFileInDirectory(childOneDir, fileName2, fileContent2);
        createFileInDirectory(childOneDir, fileName3, fileContent3);
        
        close(pipe1[1]); // Close write end of pipe 1
        close(pipe2[0]); // Close read end of pipe 2

        exit(EXIT_SUCCESS);
    } else {
//  Parent
//  Create second child process
        p2 = fork();
//  Check if process 2 failed
        if (p2 < 0) {
            fprintf(stderr, "Fork for p2 failed\n");
            exit(EXIT_FAILURE);
        }
//  INSIDE PROCESS 2
        else if (p2 == 0) {
            close(pipe1[1]); // Close write end of pipe 1
            close(pipe2[0]); // Close read end of pipe 2

            //Get Content of Files
            char *fileContent = readFilesInDirectory(childTwoDir);
            //printf("%s\n", fileContent);
            //Get File Names
            char *files = getDirectoryFileNames(childTwoDir);
            //printf("%s\n", files);
            // Concatenate into one string
            strcat(files, fileContent);
            //printf("%s\n", files);
            lock2(1);
            write(pipe2[1], files, strlen(files) + 1);
            unlock2(1);

            lock(1);
            read(pipe1[0], buffer1, BUFFER_SIZE);
            unlock(1);

            const char *fileName1 = strtok(buffer1, "\n");
           // printf("NAME: %s\n", fileName1);
            const char *fileName2 = strtok(NULL, "\n");
           // printf("NAME: %s\n", fileName2);
            const char *fileName3 = strtok(NULL, "\n");
           // printf("NAME: %s\n", fileName3);
            const char *fileContent1 = strtok(NULL, "\n");
           // printf("CONTENT: %s\n", fileContent1);
            const char *fileContent2 = strtok(NULL, "\n");
           // printf("CONTENT: %s\n", fileContent2);
            const char *fileContent3 = strtok(NULL, "\n");
           // printf("CONTENT: %s\n", fileContent3);
        
            createFileInDirectory(childTwoDir, fileName1, fileContent1);
            createFileInDirectory(childTwoDir, fileName2, fileContent2);
            createFileInDirectory(childTwoDir, fileName3, fileContent3);
       

            close(pipe1[0]); // CLose read end of pipe 1
            close(pipe2[1]); // Close write end of pipe 2

            exit(EXIT_SUCCESS);

        } else {

          //Close all ends of both pipes

            close(pipe1[0]);
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);

          //Wait for both children
            wait(NULL);
            wait(NULL);
        }
    }
    return 0;
}


char *getDirectoryFileNames(const char *directory) {
    DIR *dir;
    struct dirent *entry;
    char *filenames = NULL;
    size_t totalLength = 0;

    // Open the directory
    dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Calculate the total length of the concatenated string
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Check if it's a regular file
            totalLength += strlen(entry->d_name) + 1; // Add 1 for newline character
        }
    }

    // Allocate memory for the concatenated string
    filenames = (char *) malloc(totalLength + 1); // Add 1 for null terminator
    if (filenames == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Reset directory position
    rewinddir(dir);

    // Concatenate the file names separated by newline character
    filenames[0] = '\0'; // Initialize the string as empty
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Check if it's a regular file
            strcat(filenames, entry->d_name);
            strcat(filenames, "\n");
        }
    }

    // Close the directory
    closedir(dir);
    return filenames;
}

char *readFilesInDirectory(const char *directoryPath) {
    DIR *directory = opendir(directoryPath);
    if (directory == NULL) {
        printf("Failed to open directory '%s'\n", directoryPath);
        return NULL;
    }

    size_t totalSize = 0;
    char *result = NULL;

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);

            FILE *file = fopen(filePath, "r");
            if (file == NULL) {
                printf("Failed to open file '%s'\n", filePath);
                continue;
            }

            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            rewind(file);

            char *fileContents = malloc(fileSize + 1);
            if (fileContents == NULL) {
                printf("Failed to allocate memory for file contents\n");
                fclose(file);
                continue;
            }

            size_t bytesRead = fread(fileContents, 1, fileSize, file);
            if (bytesRead != fileSize) {
                printf("Failed to read file '%s' contents\n", filePath);
                free(fileContents);
                fclose(file);
                continue;
            }

            fileContents[fileSize] = '\0';

            size_t newSize = totalSize + 3 + fileSize + 1;
            char *newResult = realloc(result, newSize);
            if (newResult == NULL) {
                printf("Failed to allocate memory for result\n");
                free(fileContents);
                fclose(file);
                continue;
            }

            result = newResult;
            strcat(result, fileContents);
            totalSize = newSize;

            free(fileContents);
            fclose(file);
        }
    }
    closedir(directory);
    free(result);
    return result;
}

int createFileInDirectory(const char *directory, const char *filename, const char *fileContents) {
    size_t dirLen = strlen(directory);
    size_t fileLen = strlen(filename);
    size_t pathLen = dirLen + fileLen + 2;  // +2 for the '/' and null terminator
    
    // Dynamically allocate memory for filepath
    char *filepath = malloc(pathLen);
    if (filepath == NULL) {
        perror("malloc");
        return 1;
    }
    snprintf(filepath, pathLen, "%s/%s", directory, filename);
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("fopen");
        return 1;
    }

    // Populate the file with the given string
    fprintf(file, "%s", fileContents);

    // File created and populated successfully
    printf("File created and populated: %s\n", filepath);

    // Close the file
    fclose(file);
    free(filepath);
    return 0;
}

void lock_init() {
    flag[0] = flag[1] = 0;
    turn = 0;
}

void lock(int self) {
    flag[self] = 1;
    turn = 1 - self;
    while (flag[1 - self] == 1 && turn == 1 - self);
}

void unlock(int self) {
    flag[self] = 0;
}

void lock_init2() {
    flag2[0] = flag2[1] = 0;
    turn2 = 0;
}

void lock2(int self) {
    flag2[self] = 1;
    turn2 = 1 - self;
    while (flag2[1 - self] == 1 && turn2 == 1 - self);
}

void unlock2(int self) {
    flag2[self] = 0;
}