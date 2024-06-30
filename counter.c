// 2021041023 김문기

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid_t pid;

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    else if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    close(pipefd[1]);

    char buffer[BUFFER_SIZE];
    int lines = 0;
    int characters = 0;
    ssize_t count;
    while ((count = read(pipefd[0], buffer, BUFFER_SIZE)) > 0)
    {
        for (int i = 0; i < count; i++)
        {
            if (buffer[i] == '\n')
            {
                lines++;
            }
            characters++;
        }
    }

    if (count == -1)
    {
        perror("read");
        return 1;
    }

    close(pipefd[0]);

    waitpid(pid, NULL, 0);

    printf("%d characters and %d lines\n", characters, lines);

    return 0;
}