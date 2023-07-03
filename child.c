#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <time.h>
int main(int argc, char *argv[])
{
 printf ("I am the child process\n");
 // Child process
        FILE *file = fopen("test.txt", "w");
        if (file == NULL) {
            printf("can't find file\n");
            perror("fopen");
            exit(1);
        }
        for(int i=0;i<100;i++){
        fprintf(file, "%d.This is the child process writing to the file.\n",i);}
        fclose(file);
        while(1){};

        return 0;
 
}