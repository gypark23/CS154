#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

char error_message[30] = "An error has occurred\n";
char delim[] = " \n\t";
char dirdelim[] = " \n\t;>";
char enddelim[] = "\n;>";
char chunkdelim[] = ";\n";
char filedelim[] = ">+";
char whitedelim[] = " \t";

const int MAXCHAR = 1500;

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

char *trimWhitespace(char *str)
{
    if (str == NULL)
        return NULL;
    else if(str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != 0)
        return str;
    else
    {
        char *dup;
        char *next;
        char *temp;
        dup = strdup(str);
        temp = strtok_r(dup, whitedelim, &next);

        return temp;
    }
}

char* trimTail(char *str)
{
    if(str == NULL)
        return NULL;

    char* dup;
    char* next;
    char* temp;
    dup = strdup(str);
    temp = strtok_r(dup, whitedelim, &next);

    return temp;
}


void pwd()
{
    char dir[MAXCHAR];
    char *pdir = NULL;

    pdir = getcwd(dir, MAXCHAR);
    myPrint(pdir);
    myPrint("\n");
}

void cd(char *dir)
{
    char* trimdir = trimWhitespace(dir);
    if (trimdir == NULL)
    {
        if (chdir(getenv("HOME")) == -1)
            myPrint(error_message);
        else
            return;
    }

    if (trimdir[0] == '>')
        myPrint(error_message);

    if (chdir(trimdir) == -1)
        myPrint(error_message);
    return;
}

void newFork(char *cur, char **args, int redir, char *fname)
{

    if(redir == 3)
    {
        myPrint(error_message);
        return;
    }

    int pid = fork();
    int status;

    if (pid == -1)
    {
        myPrint(error_message);
        exit(0);
    }

    // parent process
    if (pid > 0)
    {
        int waitpid = wait(&status);
        if (waitpid == -1)
        {
            myPrint(error_message);
            exit(0);
        }
    }

    // child process
    else
    {
        if (redir == 1)
        {
            if (access(fname, F_OK) == 0)
            {
                // file exists
                myPrint(error_message);
                exit(0);
            }
            else
            {
                int fd = open(fname, O_RDWR | O_CREAT, 0644);
                if (fd < 0)
                {
                    myPrint(error_message);
                    exit(0);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }
        else if (redir == 2)
        {
            if (access(fname, F_OK) == 0)
            {
                // file exists
                FILE *file;
                file = fopen(fname, "r");
                if (file == NULL)
                {
                    myPrint(error_message);
                    exit(0);
                }
                FILE *temp;
                temp = fopen("temp", "w");
                if (temp == NULL)
                {
                    myPrint(error_message);
                    exit(0);
                }

                char c = fgetc(file);
                while (c != EOF)
                {
                    fputc(c, temp);
                    c = fgetc(file);
                }
                fclose(file);
                fclose(temp);

                remove(fname);

                int fd = open(fname, O_RDWR | O_CREAT, 0644);
                if (fd < 0)
                {
                    myPrint(error_message);
                    exit(0);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            else
            {
                int fd = open(fname, O_RDWR | O_CREAT, 0644);
                if (fd < 0)
                {
                    myPrint(error_message);
                    exit(0);
                }

                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }

        if (execvp(cur, args) == -1)
        {
            myPrint(error_message);
            exit(0);
        }
    }
}

char **arglist(char *cur, char *arguments)
{
    char **list = (char **)calloc(1, sizeof(char *));
    list[0] = cur;

    char *nextarg;
    char *arg;
    char *delimspace = " ";
    int counter = 1;

    if (arguments == NULL)
    {
        list[0] = cur;
        list = realloc(list, sizeof(char *));
        list[1] = NULL;
        return list;
    }

    arg = strtok_r(arguments, delimspace, &nextarg);
    if (arg != NULL)
    {
        while (arg)
        {
            list = realloc(list, (counter + 1) * sizeof(char *));
            list[counter] = arg;
            arg = strtok_r(NULL, delimspace, &nextarg);
            counter++;
        }
        list = realloc(list, (counter + 1) * sizeof(char *));
        list[counter] = NULL;
    }
    return list;
}

int isRedir(char *chunk)
{
    if(chunk == NULL)
        return 0;

    int counter = 0;
    int index = 0;
    for (int i = 0; i < strlen(chunk); i++)
    {
        if (chunk[i] == '>')
        {
            counter++;
            index = i;
        }
    }
    if (counter > 1 && index == strlen(chunk))
        return -1;

    if (counter == 1)
    {
        if (chunk[index + 1] == '+')
            return 2;
        else
            return 1;
    }
    if (counter > 0)
        return 3;

    return 0;
}


void commandLoop(char *pinput)
{
    char *cur = NULL;
    char *next = NULL;
    char *next2 = NULL;

    char *commandchunk = NULL;
    char *trimname;


    // exit
    if (!pinput || strcmp(pinput, "exit\n") == 0 || strcmp(pinput, "exit") == 0)
    {
        exit(0);
    }
    if (strcmp(pinput, "\n") == 0)
        return;

    commandchunk = strtok_r(pinput, chunkdelim, &next);
    int redir = isRedir(commandchunk);
    cur = strtok_r(commandchunk, delim, &next2);


    while (next != NULL)
    {
        if(cur == NULL)
        {
            //prevent cur being NULL; otherwise seg fault
        }
        // exit
        else if (strcmp(cur, "exit") == 0)
        {
            if (isRedir(next2) != 0 || trimWhitespace(next2) != NULL)
                myPrint(error_message);
        }

        // pwd
        else if (strcmp(cur, "pwd") == 0)
        {
            if (isRedir(next2) != 0 || trimWhitespace(next2) != NULL)
                myPrint(error_message);
            else
                pwd();
        }

        // cd
        else if (strcmp(cur, "cd") == 0)
        {
            if (isRedir(next2) != 0)
                myPrint(error_message);
            else
                cd(next2);
        }

        else if (strcmp(cur, ">") == 0)
            myPrint(error_message);

        // fork
        else
        {

            char *arguments;
            char *fname = NULL;
            trimname = NULL;

            // no arguments but redirect
            if (next2[0] == '>')
                arguments = NULL;
            else
                arguments = strtok_r(NULL, enddelim, &next2);

            if (redir == 1)
            {
                fname = strtok_r(NULL, enddelim, &next2);
                trimname = trimWhitespace(fname);
                trimname = trimTail(trimname);
            }
            if (redir == 2)
            {
                fname = strtok_r(NULL, filedelim, &next2);
                trimname = trimWhitespace(fname);
                trimname = trimTail(trimname);
            }

            char **list = arglist(cur, arguments);
            newFork(cur, list, redir, trimname);
        }

        if (redir == 2)
        {
            FILE *file;
            file = fopen(trimname, "a");

            FILE *temp;
            temp = fopen("temp", "r");

            if (temp == NULL)
            {
                //prevent temp being NULL; otherwise segfault
            }
            else
            {

                char c = fgetc(temp);
                while (c != EOF)
                {
                    fputc(c, file);
                    c = fgetc(temp);
                }
                fclose(file);
                fclose(temp);

                if (remove("temp") != 0)
                {
                    myPrint(error_message);
                    exit(0);
                }

            }

        }

        commandchunk = strtok_r(NULL, chunkdelim, &next);
        if (commandchunk == NULL)
            break;
        cur = strtok_r(commandchunk, delim, &next2);
    }

    return;
}

int tooLong(char *line)
{
    if (line != NULL)
        return (strlen(line) >= 512);
    else
        return 0;
}

int allSpace(char* line)
{
    if(line == NULL)
        return -1;

    for(int i = 0; i < strlen(line); i++)
    {
        if(!isspace(line[i]))
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    // newline and null-terminator MAXCHAR bytes
    char cmd_buff[MAXCHAR];
    cmd_buff[513] = '\0';

    // normal mode
    if (argc == 1)
    {
        while (1)
        {
            cmd_buff[513] = '\0';
            char *pinput = NULL;
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, MAXCHAR, stdin);
            if (tooLong(pinput))
            {
                myPrint(cmd_buff);
                myPrint(error_message);
                continue;
            }
            else
            {
                if (!allSpace(cmd_buff))
                    myPrint(cmd_buff);
                else
                {
                    char *temp = trimWhitespace(pinput);
                    pinput = temp;
                }
                commandLoop(pinput);
            }
        }
    }

    // batch mode
    else if (argc == 2)
    {
        FILE *batch = fopen(argv[1], "r");
        if (batch == NULL)
        {
            myPrint(error_message);
            exit(0);
        }

        while (fgets(cmd_buff, MAXCHAR, batch) != NULL)
        {
            char *pinput = NULL;
            pinput = strdup(cmd_buff);
            if (tooLong(pinput))
            {
                myPrint(cmd_buff);
                myPrint(error_message);
                continue;
            }
            else
            {
                if (!allSpace(cmd_buff))
                    myPrint(cmd_buff);
                else
                {
                    char *temp = trimWhitespace(pinput);
                    pinput = temp;
                }
            }

            if (pinput == NULL)
                break;

            if (strcmp(pinput, "exit\n") == 0 || strcmp(pinput, "exit") == 0)
            {
                return 0;
                exit(0);
            }
            else
                commandLoop(pinput);
        }

        fclose(batch);
        exit(0);
    }
    else
    {
        myPrint(error_message);
        exit(0);
    }

    exit(0);
}
