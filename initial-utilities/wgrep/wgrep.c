#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    if (argc == 1){
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }

    int hasString = 0;
    FILE* file = fopen(argv[1], "r");
    if (file == NULL){
        hasString = 1;
    } else fclose(file);

    if (hasString == 1){
        char* searchString = argv[1];

        // if no file is specified
        if (argc == 2){
            char buffer[100];
            while (fgets(buffer, 100, stdin)){
                char* found = strstr(buffer, searchString);
                if (found != NULL){
                    printf("%s", buffer);
                }
            }
        }
        else { // handle all other files
            for (int i = 2; i < argc; i++){
                FILE* fp = fopen(argv[i], "r");

                if (fp == NULL){
                    printf("wgrep: cannot open file\n");
                    exit(1);
                }

                char* line = NULL;
                size_t len = 0;

                while (getline(&line, &len, fp) != -1){
                    char* found = strstr(line, searchString);
                    if (found != NULL){
                        printf("%s", line);
                    }
                }
                fclose(fp);
            }
        }
    }

    exit(0);
    // for (int i = 2; i < argc; i++){
    //     FILE* fp = fopen(argv[i], "r");

    //     if (fp == NULL){
    //         printf("wgrep: cannot open file\n");
    //         exit(1);
    //     }

    //     char* line = NULL;
    //     ssize_t len = 0;

    //     while (getline(&line, &len, fp) != 1){
    //         char* found = strstr(line, searchString);
    //         if (found != NULL){
    //             printf("%s", line);
    //         }
    //     }
    //     fclose(argv[i]);
    // }


}