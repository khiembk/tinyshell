#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <fcntl.h>
#include <errno.h>

#define PATH_LEN 1024
#define TOKEN_SIZE 100
#define TOKEN_NUM 100
#define MAX_PIPE 20
#define TOKEN_DELIM "\t\r\n\a "


 extern char** environ;
 
int execute_command(char** args)
{  
    if (args[0] == NULL)
    {
        return 0;
    }
     
    // pwd: print current directory
    if (strcmp(args[0], "pwd") == 0)
    {
        char cwd[PATH_LEN];
        getcwd(cwd, sizeof(cwd));
        printf("%s\n", cwd);
    }

    // cd: change directory
    else if (strcmp(args[0], "cd") == 0)
    {
        // Go to home directory if no argument provided
        if (args[1] == NULL)
        {
            // Get home directory from $HOME environment variable
            const char* home_dir;
            if ((home_dir = getenv("HOME")) == NULL)
                // If $HOME doesn't exist, try getpwuid
                home_dir = getpwuid(getuid())->pw_dir;

            if (chdir(home_dir) == -1)
                perror("cd");
        }

        else if (chdir(args[1]) == -1)
            perror("cd");
    }

    // exit: exit shell
    else if (strcmp(args[0], "exit") == 0)
    {
        
         FILE *file;
        file=fopen("bg.txt","r");
        char line[40];
        while ( fgets( line, sizeof(line), file)!=NULL){
             pid_t processId= atoi(line);
             kill(processId,SIGTERM);
        }
       fclose(file);
        file =fopen("bg.txt","w");
        fclose(file);
      exit(EXIT_FAILURE);
    }

    // mkdir: create new directory
    else if (strcmp(args[0], "mkdir") == 0)
    {
        // Handle no argument passed
        if (args[1] == NULL)
        {
            printf("mkdir: missing operand\n");
            return 0;
        }

        for (int i = 1; args[i] != NULL; i++)
        {
            mode_t process_mask = umask(0);
            int status = mkdir(args[i], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            umask(process_mask);

            if (status == -1)
                perror("mkdir");
        }

    }

    // rmdir: delete directory and all contents
    else if (strcmp(args[0], "rmdir") == 0)
    {
        // Handle no argument passed
        if (args[1] == NULL)
        {
            printf("rmdir: missing operand\n");
            return 0;
        }

        for (int i = 1; args[i] != NULL; i++)
        {
            int status = rmdir(args[i]);
            if (status == -1)
                perror("rmdir");
        }
    }

    // cp: copy file1 to file2 if file1 was modified after file2
    else if (strcmp(args[0], "cp") == 0)
    {
        // File stats
        struct stat file1_stat;
        struct stat file2_stat;

        char buf[1024];
        int file1, file2;
        ssize_t count;

        // Default modification time for file2 (if does not exist already)
        __time_t file2_mod_time = 0;

        if (args[1] == NULL)
        {
            printf("cp: missing source and destination file\n");
            return 0;
        }

        else if (args[2] == NULL)
        {
            printf("cp: missing destination file\n");
            return 0;
        }

        // Handle stat error
        if(stat(args[1], &file1_stat) == -1)
        {
            perror("cp");
            return 0;
        }

        // Handle un-created file2 modification time issue separately
        if(stat(args[2], &file2_stat) == -1)
            file2_mod_time = 0;
        else
            file2_mod_time = file2_stat.st_ctim.tv_sec;

        // Compare file modification dates
        // No action if modification time of file2 is more recent
        if (file1_stat.st_ctim.tv_sec <= file2_mod_time)
            return 0;

        // Open files, check permissions and errors
        if ((file1 = open(args[1], O_RDONLY)) == -1)
        {
            perror("cp");
            return 0;
        }

        if ((file2 = open(args[2], O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1)
        {
            perror("cp");
            return 0;
        }

        while ((count = read(file1, buf, sizeof(buf))) != 0)
            write(file2, buf, (size_t) count);
    }

    // ls, ls -l: list current directory contents
    else if (strcmp(args[0], "ls") == 0)
    {
        char cwd[PATH_LEN];
        getcwd(cwd, sizeof(cwd));
        DIR* current_dir = opendir(cwd);
        struct dirent* dir_file;
        struct stat dir_stat;

        while ((dir_file = readdir(current_dir)) != NULL)
        {
            // Do not print hidden files (.name)
            if (dir_file->d_name[0] == '.')
                continue;

            // ls without any option flags
            if (args[1] == NULL)
                printf("%s\n", dir_file->d_name);

            // ls with "-l" option flag
            else if (strcmp(args[1], "-l") == 0)
            {
                // Handle stat error
                if(stat(dir_file->d_name, &dir_stat) == -1)
                {
                    perror("ls");
                    return 0;
                }

                char permissions[11];
                char* month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

                // Get file permission flags
                strcpy(permissions, S_ISDIR(dir_stat.st_mode) ? "d" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IRUSR) ? "r" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IWUSR) ? "w" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IXUSR) ? "x" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IRGRP) ? "r" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IWGRP) ? "w" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IXGRP) ? "x" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IROTH) ? "r" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IWOTH) ? "w" : "-");
                strcat(permissions, (dir_stat.st_mode & S_IXOTH) ? "x" : "-");

                // Get time stamp into components
                struct tm* time_stamp;
                time_stamp = localtime(&dir_stat.st_atim.tv_sec);

                // Get user and group ID
                struct passwd* user = getpwuid(dir_stat.st_uid);
                struct group* gp = getgrgid(dir_stat.st_gid);

                printf("%s %2d %s %s %8d ", permissions, (int) dir_stat.st_nlink, user->pw_name, gp->gr_name,
                       (int) dir_stat.st_size);
                printf("%s %d %02d:%02d ", month[time_stamp->tm_mon], time_stamp->tm_mday, time_stamp->tm_hour,
                       time_stamp->tm_min);
                printf("%s\n", dir_file->d_name);
            }

            // Default case
            else
                printf("ls: Unrecognized option\n");
        }
        closedir(current_dir);
    }
    else if(strcmp(args[0],"run") == 0 && strcmp(args[1],"fg") == 0){
        printf("Root process: %d\n",getpid());
        pid_t child_pid;
        pid_t child;
        int status;
        child_pid= fork();
        if (child_pid< 0) {
        // Fork failed
        perror("Fork failed");
        exit(1);
    }
       
        if(child_pid==0){
               child= getpid();
               printf("child process!\n");
               printf("child PID =  %d, parent pid = %d\n", child, getppid());
               if( execv(args[2],args)<0){
                    perror("Execv failed");
                     exit(1);
               }}else{
                  
         
             while (1) {
            char c;
            scanf("%c", &c);
            if (c == 'c') {
                kill(child_pid, SIGTERM);
                break;
            }
            if(waitpid(child,&status,WNOHANG)!=0){
                break;
            }
        }

        // Wait for the child process to terminate
        waitpid(child_pid, &status, 0);
           
                 
            }}

     else if(strcmp(args[0],"run") == 0 && strcmp(args[1],"bg") == 0){
       printf("Root process: %d\n",getpid());
        pid_t child_pid;
        int status;
       
        child_pid= fork();
             if (child_pid<0){   
            perror("Error while creating a new process");
              return 1;
            }  
       
            if(child_pid==0){
                 
                  pid_t processID= getpid();
                  pid_t parentId= getppid();
                  FILE *file;
                  file= fopen("bg.txt","a");
                  if (file == NULL) {
                     printf("Unable to open the file.\n");
                     fclose(file);
                     exit(1);
                       }
                  fprintf(file,"%d\n",processID);
                 fclose(file);
               printf("child PID =  %d, parent pid = %d\n", processID, parentId);
             if(  execv(args[2],args)<0){
                perror("Exec failed");
                 exit(1);
               }}
            
    return 0;
}
   else if(strcmp(args[0],"print") == 0 && strcmp(args[1],"bg") == 0){
     FILE *file;
     file= fopen("bg.txt","r");
       if (file == NULL) {
        printf("Unable to open the file.\n");
        exit(1);
    }
     char line[20];
     printf("PID         Status     CMD\n");
     while(fgets(line,sizeof(line),file)!=NULL){
         int status;
         pid_t pid= atoi(line);
        if( waitpid(pid,&status,WNOHANG)!=0)
                continue;
         char path[256];
         snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
         char path1[256];
         snprintf(path1, sizeof(path1), "/proc/%d/exe", pid);
         FILE * file1= fopen (path,"r");
         if(file1==NULL){
             perror("Failed to open file");
             return 1;
         }
         char cmdName[256];
         ssize_t length = readlink(path1, cmdName, sizeof(cmdName) - 1);
         cmdName[length]='\0';
         char cmdline[256];
         fgets(cmdline,sizeof(cmdline),file1);
         printf("%d         %s     %s\n",pid,cmdline,cmdName );

     }
     fclose(file);
   }
                   
    else if(strcmp(args[0],"list") == 0 && strcmp(args[1],"bg") == 0){
        FILE *file;
        file=popen("ps","r");
        char line[40];
        fgets( line, sizeof line, file);
        printf("%s\n",line);

        while ( fgets( line, sizeof line, file))

       {
        
           printf("%s\n",line);
      }
           
    
       pclose(file);
    }
    else if(strcmp(args[0],"kill") ==0){
         pid_t processId= atoi (args[1]);
         pid_t root= getpid();
         if(root!=processId){
         kill(processId,SIGSEGV);
         int status;
         int i=0;
         while (1){
         pid_t result = waitpid(processId, &status, WNOHANG);
         if (result == -1) {
                perror("Error in waitpid");
                break;  // Break the while loop if there is an error
            }
            if(result>0){
                FILE *file;
                file= fopen("bg.txt","r");
                pid_t String[100];
                char buffer[30];
                int i=0;
                while (fgets(buffer,sizeof(buffer),file)!=NULL){
                   
                    pid_t thisId= atoi(buffer);
                    if(thisId!=processId){
                        String[i]=atoi(buffer);
                        i++;
                    }
                }
                fclose(file);
                file=fopen("bg.txt","w");
                for(int j=0;j<i;j++){
                    fprintf(file,"%d\n",String[j]);
                }
                fclose(file);

                 printf("process %d is killed with status %d\n",processId,status);
                 break;
            }
            i++;
            if(i>999999){
                printf("process %d  can't be  killed because it is paused\n",processId);
                 break;
            }
            
            }}
            else {
                FILE *file;
                file= fopen("bg.txt","r");
                char line[30];
                while(fgets(line,sizeof(line),file)!=NULL){
                       pid_t childid= atoi(line);
                       kill(childid,SIGSEGV);                      
                }
                fclose(file);
                file=fopen("bg.txt","w");
                fclose(file);
                exit(EXIT_SUCCESS);
            }
    }
    else if (strcmp(args[0],"stop") ==0){
         pid_t processId= atoi (args[1]);
         int result = kill(processId,SIGSTOP);
         if(result==0){
             printf("Process with PID %d stopped successfully\n", processId);
            return 0;
         }else{
             printf("Failed to stop process\n");
             return -1;
         }

    }
     else if (strcmp(args[0],"resume") ==0){
         pid_t processId= atoi (args[1]);
         int result = kill(processId,SIGCONT);
         if(result==0){
             printf("Process with PID %d resumed successfully\n", processId);
            return 0;
         }else{
             printf("Failed to resume process\n");
             return -1;
         }

    }
       else if(strcmp(args[0],"help") ==0){
         printf("***********************Welcome to my shell**********************************\n");
         printf("1. run fg -program.bin : run a bin file in foreground\n");
         printf("2. run bg -program.bin : run a bin file in background\n");
         printf("3. kill -PID           : kill a chill background process\n");
         printf("4. stop -PID           : stop a chill background process\n");
         printf("5. resume -PID         : resume a chill background process\n");
         printf("6. print bg            : show child bg process of this shell\n");
         printf("7. ls                  : list file in this folder\n");
         printf("8. ls -l               : list file in this folder with more information\n");
         printf("9. cp                  : copy file\n");
         printf("10.list bg             : print all child process of this terminal\n");
         printf("11.cp                  : coppy file\n");
         printf("12.exit                : exit shell\n");
         printf("13.date                : print date \n");
         printf("14.time                : print time\n");
         printf("15.cp -file1 -file2    : create file2 coppy of file1\n");
         printf("16.mkdir -dir -[dirN]  : create new directories\n");
         printf("17.rmdir -dir -[dirN]  : remove directories\n");
         printf("18.cd -[dir]           : change directory\n");
         printf("19.pwd                 : print current directory\n");
         printf("20.print environ       : print environment variable\n");
         printf("21.set environ -name -value :set new value for environment varriable\n");
         printf("22.cat -file           : print content of file into screen\n");
         printf("---------------------------------------------------------------------------\n");
       }
  
     else if (strcmp(args[0],"date") ==0){
          time_t currentTime;
    struct tm* timeInfo;
    char buffer[80];

    // Get current time
    currentTime = time(NULL);
    timeInfo = localtime(&currentTime);

    // Format the date string
    strftime(buffer, sizeof(buffer), "%d-%m-%Y", timeInfo);

    // Print the date
    printf("Current date: %s\n", buffer);
     }
    else if(strcmp(args[0],"time") ==0){
        time_t currentTime;

    // Get current time
    currentTime = time(NULL);

    // Convert to local time string
    char* timeString = ctime(&currentTime);

    // Print the current time
    printf("Current time: %s\n", timeString);
    }
    else if(strcmp(args[0],"print") ==0 && strcmp(args[1],"environ") ==0){
                char** env = environ;
                  while (*env != NULL) {
                 printf("%s\n", *env);
                env++;
    }
    }
    else if (strcmp(args[0],"set")==0 && strcmp(args[1],"environ")==0){
          char* oldValue= getenv(args[2]);
          printf("Old value of environment variable:%s\n",oldValue);
          if (setenv(args[2], args[3], 1) == 0) {
          printf("MY_VARIABLE set successfully\n");
       } else {
          printf("Failed to set MY_VARIABLE\n");
    }
    char* newValue = getenv(args[2]);
    printf("New value of MY_VARIABLE: %s\n", newValue);
    }
    else if (strcmp(args[0],"cat")==0){
       FILE *file;
       file=  fopen(args[1],"r");
       if(file==NULL){
           fclose(file);
           printf("can't open file!\n");
           exit(1);
       }
       char line[30];
       while(fgets(line,sizeof(line),file)!=NULL){
           if(line[strlen(line)-1]=='\n')
              line[strlen(line)-1]='\0';
             printf("%s\n",line);
            
           // int status = execute_command(parse_command(command));
       }
       
       fclose(file);
    }


    return 0;
}
 

/**
 * Tokenize and parse command line inputs
 */
char** parse_command(char* command)
{
    char** args = malloc(TOKEN_SIZE * sizeof(char*));
    char* token;

    // Argument indices and token size
    int pos = 0;
    int total_token_size = TOKEN_SIZE;

    // Parse and tokenize command input
    token = strtok(command, TOKEN_DELIM);
    while (token != NULL)
    {
        args[pos] = token;
        pos++;

        // Expand args buffer when full
        if (pos >= total_token_size)
        {
            total_token_size += TOKEN_SIZE;
            args = realloc(args, total_token_size * sizeof(char*));
        }

        token = strtok(NULL, TOKEN_DELIM);
    }

    args[pos] = NULL;
    return args;
}


void init_shell()
{
    char cwd[PATH_LEN]; // Current working directory
    char* command; // Command line input
    char** parsed_command; // Parsed and tokenized input
    
    while(true)
    {
        
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s> ", cwd);
        
        }
        else
        {
            perror("cwd:");
            exit(EXIT_FAILURE);
        }

        size_t bufsize = 0;
        getline(&command, &bufsize, stdin);
        parsed_command = parse_command(command);
        int status = execute_command(parsed_command);

        free(command);
        free(parsed_command);
        
        // Execute command fails/error occurs
        if (status == -1)
            exit(EXIT_FAILURE);
    }
}


int main(int argc, char* argv[])
{  
    
    init_shell();
    exit(EXIT_SUCCESS);
}
