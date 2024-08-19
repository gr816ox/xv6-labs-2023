#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 35

int main(int argc, char const *argv[])
{
    int pipes[N][2];
    int last_pipe = 0;
    int prime = 0;
    int pid;

    int num_childread = 0;

    pipe(pipes[0]);
    if ((pid = fork()) == 0)
    {
        int ret;

        close(pipes[0][0]);
        for (int i = 2; i <= N; i++)
        { 
            if ((ret=write(pipes[0][1], &i, 4)) != 4)
            {   
                fprintf(2, "error write 2,3,... to pipe: %d\n", ret);
                exit(1);            
            }

        }
        // printf("all numbers are sent!\n");
        close(pipes[0][1]);
        exit(0);
    }
    else if (pid == -1)
    {
        fprintf(2, "error fork\n");
        exit(1);            
    }
    else
    {
        close(pipes[0][1]);
        while (1)
        {
            if (read(pipes[last_pipe][0], &prime, 4) == 0) { break; }
            printf("prime %d\n",prime);
            last_pipe++;
            pipe(pipes[last_pipe]);
            // printf("piped pipes[%d]\n", last_pipe);
            pid = fork();
            if (pid == 0)
            {
                // printf("created child num[%d]\n", prime);
                close(pipes[last_pipe][0]);
                while (1)
                {
                    if (read(pipes[last_pipe-1][0], &num_childread, 4) == 0) {  break; }
                    else if (num_childread % prime != 0)
                    {
                        write(pipes[last_pipe][1], &num_childread, 4);
                    }
                    // else if (num_childread % prime == 0)
                    // {
                    //     printf("drop num: %d\n", num_childread);
                    // }
                    
                }
                close(pipes[last_pipe][1]);
                close(pipes[last_pipe-1][0]);
                // printf("child exit\n");
                exit(0);
            }
            else if ( pid > 0 )
            {
                close(pipes[last_pipe][1]);
                close(pipes[last_pipe-1][0]);
            }
        }
        close(pipes[last_pipe][0]);
    }
    return 0;
}
