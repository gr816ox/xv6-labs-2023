#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int 
main(int argc, char *argv[])
{

    int sec;

    if(argc != 2){
        fprintf(2, "usage: sleep NUMBER\n");
        exit(1);
    }

    if (!(sec = atoi(argv[1])))
    {
        fprintf(2, "usage: sleep NUMBER\n");
        exit(1);
    }

    // printf("time:%d\n",sec);

    sleep(sec*10);
    exit(0);
    
}