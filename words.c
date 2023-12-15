#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>



struct WordCount {
    char* word;
    int count;
};

struct WordCount* globalWordCounts = NULL;
size_t globalSize = 0;
size_t globalCapacity = 0;


void tokenizeAndCount(struct WordCount** wordCounts, size_t* size, size_t* capacity, char* line, int index);

void tokenizeLine(char* line, struct WordCount** wordCounts, size_t* size, size_t* capacity) {
    int filteredTokenIndex = 0;
    int isPartOfWord = 0;

    for (int i = 0; line[i]; i++) {
        char currentChar = line[i];

        if (isalpha(currentChar) || currentChar == '\'') {
            isPartOfWord = 1;
            line[filteredTokenIndex++] = currentChar;
        } else if (currentChar == '-') {
            if (i > 0 && isalpha(line[i - 1]) && isalpha(line[i + 1])) {
                // If the hyphen is between letters, consider it part of the word
                isPartOfWord = 1;
                line[filteredTokenIndex++] = currentChar;
            } else {
                // Hyphen is treated as a word separator
                tokenizeAndCount(&(*wordCounts), size, capacity, line, filteredTokenIndex);
                filteredTokenIndex = 0;
                isPartOfWord = 0;
            }
        } else {
            // Character is not part of the word, check if there's a word to tokenize
            tokenizeAndCount(&(*wordCounts), size, capacity, line, filteredTokenIndex);
            filteredTokenIndex = 0;
            isPartOfWord = 0;
        }
    }

    // Tokenize any remaining text in the line if it's part of a word
    if (isPartOfWord) {
        tokenizeAndCount(&(*wordCounts), size, capacity, line, filteredTokenIndex);
    }
}

void tokenizeAndCount(struct WordCount** wordCounts, size_t* size, size_t* capacity, char* line, int index) {
    if (index > 0) {
        line[index] = '\0';
        char* filteredToken = strdup(line);
        int found = 0;
        for (size_t i = 0; i < *size; ++i) {
            if (strcmp((*wordCounts)[i].word, filteredToken) == 0) {
                (*wordCounts)[i].count++;
                found = 1;
                break;
            }
        }

        if (!found) {
            if (*size == *capacity) {
                *capacity = (*capacity == 0) ? 1 : *capacity * 2;
                *wordCounts = (struct WordCount*)realloc(*wordCounts, *capacity * sizeof(struct WordCount));
            }
            (*wordCounts)[*size].word = strdup(filteredToken);
            (*wordCounts)[*size].count = 1;
            (*size)++;
        }
    }
}

int compareWordCounts(const void* a, const void* b) {
    const struct WordCount* wordCountA = (const struct WordCount*)a;
    const struct WordCount* wordCountB = (const struct WordCount*)b;

    if (wordCountA->count > wordCountB->count) {
        return -1;
    } else if (wordCountA->count < wordCountB->count) {
        return 1;
    }

    // Case-sensitive comparison for words with the same count
    return strcmp(wordCountA->word, wordCountB->word);
}



void wordCount(const char* file) {
    int fd = open(file, O_RDONLY);
    if (fd == -1) {
        printf("Unable to open file: %s\n", file);
        return;
    }

    // Create an array of WordCount structures for each file
    struct WordCount* wordCounts = NULL;
    size_t capacity = 0;  // Initial capacity
    size_t size = 0;      // Current number of word counts

    char buffer[BUFSIZ];  // Buffer for reading data
    ssize_t bytesRead;
    char line[BUFSIZ * 2];  // Allocate memory for a line

    // Initialize the line buffer
    line[0] = '\0';

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytesRead; ++i) {
            if (isspace(buffer[i]) || buffer[i] == '\n') {
                if (strlen(line) > 0) {
                    tokenizeLine(line, &wordCounts, &size, &capacity);
                    line[0] = '\0';  // Reset the line for the next word
                }
            } else {
                if (strlen(line) + 1 < BUFSIZ * 2) {
                    strncat(line, &buffer[i], 1);  // Append the current character
                }
            }
        }
    }

    // Tokenize any remaining text in the line
    if (strlen(line) > 0) {
        tokenizeLine(line, &wordCounts, &size, &capacity);
    }

    // Close the file
    close(fd);

    // Sort the wordCounts array for each file
    qsort(wordCounts, size, sizeof(struct WordCount), compareWordCounts);

    // Accumulate word counts globally
    for (size_t i = 0; i < size; ++i) {
        int found = 0;
        for (size_t j = 0; j < globalSize; ++j) {
            if (strcmp(globalWordCounts[j].word, wordCounts[i].word) == 0) {
                globalWordCounts[j].count += wordCounts[i].count;
                found = 1;
                break;
            }
        }

        if (!found) {
            if (globalSize == globalCapacity) {
                globalCapacity = (globalCapacity == 0) ? 1 : globalCapacity * 2;
                globalWordCounts = (struct WordCount*)realloc(globalWordCounts, globalCapacity * sizeof(struct WordCount));
            }
            globalWordCounts[globalSize].word = strdup(wordCounts[i].word);
            globalWordCounts[globalSize].count = wordCounts[i].count;
            globalSize++;
        }
    }

    // Free memory for the array of word counts for each file
    free(wordCounts);
}


int isTxt(char file[]){

    if (strlen(file) > 4 && !strcmp(file + strlen(file) - 4, ".txt")){
        return 0;
    }
    
    return -1;

}

void directorySearch();

int checkIfDir(char* file, char dirPath[]){

    char dest[strlen(dirPath) + strlen(file) + 2];

    strcpy(dest, dirPath);
    strcat(dest, "/");
    strcat(dest, file);

    struct stat path;
    stat(dest, &path);

    if (S_ISREG(path.st_mode) == 0){
        directorySearch(dest);
        return 0;
    } else {
        
        if (isTxt(dest) == 0){
            wordCount(dest);
        }

        return -1;
    }
}

void directorySearch(const char* dirPath) {
    DIR *dir = opendir(dirPath);

    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filePath[PATH_MAX];
            snprintf(filePath, PATH_MAX, "%s/%s", dirPath, entry->d_name);
            
            if (isTxt(entry->d_name) == 0) {
                wordCount(filePath);
            }
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subDirPath[PATH_MAX];
            snprintf(subDirPath, PATH_MAX, "%s/%s", dirPath, entry->d_name);
            directorySearch(subDirPath);
        }
    }

    closedir(dir);
}


void test1(){

    // Test 1 with periods, commas, etc.

    int fileDescriptor = open("test1.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

    if (fileDescriptor == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    const char *textToAppend = "This is. A test, I'm using^ to see if; our. program works! \n";

    ssize_t bytesWritten = write(fileDescriptor, textToAppend, strlen(textToAppend));

    if (bytesWritten == -1) {
        perror("Error writing to file");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    if (close(fileDescriptor) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }

    wordCount("test1.txt"); 


}

void test2(){
    
    // Test 2 with dashes and apostrophes

    int fileDescriptor = open("test2.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

    if (fileDescriptor == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    const char *textToAppend2 = "Ou'r progr-am should prin't thi-s but not te--st or te-'st \n";

    ssize_t bytesWritten = write(fileDescriptor, textToAppend2, strlen(textToAppend2));

    if (bytesWritten == -1) {
        perror("Error writing to file");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    if (close(fileDescriptor) == -1) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }

    wordCount("test2.txt"); 

}

void test3(){

    // Test 3 with directories (no sub directories)

    directorySearch("testdir");

}

void test4(){

    // Test 4 with directories + sub directories

    directorySearch("testdir2");

}


int main(int argc, char* argv[]){

    // Test Cases

    // Uncomment to run test cases

    /*test1();
    test2();
    test3();
    test4();*/
    

        if (argc <= 1){
        printf("Please enter atleast one file or directory as parameters.");
    }

    for (int i = 1; i < argc; i++){
        if(isTxt(argv[i]) == 0){
            printf("Found txt file!");
            wordCount(argv[i]);       // Proceed with calculating word count
        } else {
            directorySearch(argv[i]);                  // Scan directory for .txt files
        }
    }

    

    qsort(globalWordCounts, globalSize, sizeof(struct WordCount), compareWordCounts);

    for (size_t i = 0; i < globalSize; i++) {
        char buffer[100];
        int count = snprintf(buffer, sizeof(buffer), "%s %d\n", globalWordCounts[i].word, globalWordCounts[i].count);
        write(STDOUT_FILENO, buffer, count);
        free(globalWordCounts[i].word);
    }

    free(globalWordCounts);

}