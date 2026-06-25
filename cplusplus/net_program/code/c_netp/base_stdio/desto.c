#include <stdio.h>
#include <fcntl.h>

int main()
{
    FILE* fp;
    int fd;
    fd = open("data.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd == -1)
    {
        fputs("file open error", stdout);
        return -1;
    }

    fp = fdopen(fd, "w");
    fputs("Net C\n", fp);
    printf("%d\n", fileno(fp));

    printf("%d\n", fp->_fileno);
    
    fclose(fp);

    return 0;
}