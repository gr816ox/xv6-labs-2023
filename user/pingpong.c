#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char const *argv[])
{
    int pipe1[2];
    int pipe2[2];

    char byte = 'A';

    if ((pipe(pipe1) == -1) || (pipe(pipe2) == -1))
    {
        fprintf(2, "error creating pipe\n");
        exit(1);
    }

    int pid = fork();
    if (pid == -1)
    {
        fprintf(2, "error fork\n");
        exit(1);
    }

    else if (pid == 0)
    {
        read(pipe1[0], &byte, 1);
        printf("%d: received ping\n", getpid());
        write(pipe2[1], &byte, 1);
    }
    
    else
    {
        write(pipe1[1], &byte, 1);
        read(pipe2[0], &byte, 1);
        printf("%d: received pong\n", getpid());
    }
    
    return 0;
}
