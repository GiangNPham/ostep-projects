#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char* argv[]){
    if (argc == 1){
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    char cur;
    int cnt = 0;

    for (int i = 1; i < argc; i++){
        FILE* fp = fopen(argv[i], "r");

        while (fread(&cnt, sizeof(int), 1, fp) && fread(&cur, sizeof(char), 1, fp)){
            while (cnt--){
                printf("%c", cur);
            }
        }
        fclose(fp);
    }
    exit(0);
}