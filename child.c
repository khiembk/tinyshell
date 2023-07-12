#include  <stdio.h>
#include  <stdlib.h>
#include  <unistd.h>
#include  <time.h>
int main(int argc, char *argv[])
{
 
 // Child process
        for(int i=0;i<100;i++){
            
            FILE *file = fopen("test.txt", "a");
            if (file == NULL) {
                printf("can't find file\n");
                perror("fopen");
                exit(1);
            }
            fprintf(file, "%d. This is the child process writing to the file.\n",i);

            fclose(file);
            // printf ("I am the closing file\n");
            sleep(1);
        }
            // while(1){};

        return 0;
 
}