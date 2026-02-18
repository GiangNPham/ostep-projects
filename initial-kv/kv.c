#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]){
    FILE* fp = fopen("database.txt", "a+");
    if (fp == NULL){
        printf("Error: Could not open or create database file\n");
        exit(1);
    }
    char* line = NULL;


    for (int i = 1; i < argc; i++){
        char command = argv[i][0];
        
        if (command == 'g'){
            strsep(&argv[i], ",");
            int key = atoi(strsep(&argv[i], ","));

            size_t size;
            int found = 0;
            fseek(fp, 0, SEEK_SET);
            while (getline(&line, &size, fp) != -1) {
                char* tmp = line;
                int dkey = atoi(strsep(&tmp, ","));
                if (dkey == key){
                    printf("%d,%s", key,strsep(&tmp, ","));
                    found = 1;
                    break;
                }
            }

            if (!found){
                printf("%d not found\n", key);
            }
        }
        
        else if (command == 'p'){
            strsep(&argv[i], ",");
            int key = atoi(strsep(&argv[i], ","));

            fseek(fp, 0, SEEK_END);
            char* value = strsep(&argv[i], ",");
            fprintf(fp, "%d,%s\n", key, value);
            fflush(fp);                
        }

        else if (command == 'd') {
            strsep(&argv[i], ",");
            int key = atoi(strsep(&argv[i], ","));

            int found = 0;
            fseek(fp, 0, SEEK_SET);
            size_t size;
            while (getline(&line, &size, fp) != -1) {
                char* tmp = line;
                int dkey = atoi(strsep(&tmp, ","));
                if (dkey == key){
                    found = 1;
                    break;
                }
            }

            if (!found){
                printf("%d not found\n", key);
                continue;
            }

            FILE* tfp = fopen("tdatabase.txt", "w+");
            fseek(fp, 0, SEEK_SET);
            while (getline(&line, &size, fp) != -1) {
                char* tmp = line;
                int dkey = atoi(strsep(&tmp, ","));
                if (dkey == key) continue;

                fprintf(tfp, "%d,%s", dkey, line);
            }
            fclose(tfp);
            fclose(fp);
            

            remove("database.txt");
            rename("tdatabase.txt", "database.txt");
            fp = fopen("database.txt", "a+");
        }
        else if (command == 'c'){
            FILE* tfp = fopen("database.txt", "w");
            fclose(tfp);
        }
        else if (command == 'a'){
            size_t size;

            while (getline(&line, &size, fp) != -1){
                printf("%s", line);
            }

        }
        else {
            printf("bad command\n");
        }

    }
    if (line) free(line);
    fclose(fp);
}

