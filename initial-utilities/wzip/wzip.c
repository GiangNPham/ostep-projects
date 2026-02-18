#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    if (argc == 1){
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    char cur;
    char prev = '.';
    int cnt = 0;

    for (int i = 1; i < argc; i++){
        FILE* fp = fopen(argv[i], "r");

        while ((cur = fgetc(fp)) != EOF){
            if (cur != prev){
                if (prev != '.'){
                    fwrite(&cnt, sizeof(int), 1, stdout);
                    fwrite(&prev, sizeof(char), 1, stdout);
                }
                cnt = 0;
            }
            cnt++;
            prev = cur;
        }
        fclose(fp);
    }

    if (cnt){
        fwrite(&cnt, sizeof(int), 1, stdout);
        fwrite(&prev, sizeof(char), 1, stdout);
    }
    exit(0);
}