#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef words
#include "words.c"


void test1(){

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

    return 0;

}
#endif