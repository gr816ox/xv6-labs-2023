#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int find_in_path(char *path, char *word){
    char * path_end = path+strlen(path)-strlen(word);
    if (strlen(path)<strlen(word)) {return 0;}
    for (char *p = path; p <= path_end; p++)
    {
        if(strncmp(p, word, strlen(word)) == 0){
            printf("%s\n", path);
            return 1;
        }
    }
    return 0;
}

int
is_directory(char *path)
{
    int fd;
    struct stat st;
    char * name;
    for(name=path+strlen(path); name >= path && *name != '/'; name--)
    ;
    name++;
    if ((strcmp(name,".") == 0) || (strcmp(name,"..") == 0))
    {
        return 0;
    }
    

    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return 0;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return 0;
    }

    if (st.type == T_DIR) {
        close(fd);
        return 1;
    }
    else {
        close(fd);
        return 0;
    }

}

void 
find(char *path, char *word)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;



    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        fprintf(2, "find [starting-point...] [expression] starting-point must be a directory.\n");
        close(fd);
        return;
    }
    else {
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: starting-point too long\n");
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            find_in_path(buf, word);
            if (is_directory(buf) == 1)
            {
                find(buf, word);
            }
        }
        close(fd);
    }
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "find [starting-point...] [expression]\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}