#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    for (int i = 1; i < argc; i++){
        FILE* file = fopen(argv[i], "r");
        if (file == NULL){
            printf("wcat: cannot open file\n");
            exit(1);
        }

        char buffer[50];
        while (fgets(buffer, 50, file)){
            printf("%s", buffer);
        }
        fclose(file);
    }
    exit(0);
}