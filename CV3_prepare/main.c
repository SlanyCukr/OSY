#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>

int main(int argc, char **argv)
{
    char *file_name = argv[1];

    struct stat statbuf;
    int result = stat(file_name, &statbuf);

    if (result == -1)
    {
        fprintf(stderr, "Failure\n");
        exit(EXIT_FAILURE);
    }

    printf("File type:                ");

    switch (statbuf.st_mode & S_IFMT)
    {
    case S_IFBLK:
        printf("block device\n");
        break;
    case S_IFCHR:
        printf("character device\n");
        break;
    case S_IFDIR:
        printf("directory\n");
        break;
    case S_IFIFO:
        printf("FIFO/pipe\n");
        break;
    case S_IFLNK:
        printf("symlink\n");
        break;
    case S_IFREG:
        printf("regular file\n");
        break;
    case S_IFSOCK:
        printf("socket\n");
        break;
    default:
        printf("unknown?\n");
        break;
    }

    printf("Mode:                     %d\n", statbuf.st_mode);
    printf("Mode:                     %jo (octal)\n", (uintmax_t)statbuf.st_mode);

    int statchmod = statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    printf("chmod:                    %o\n", statchmod);

    char symbolic_chars[] = {'x', 'w', 'r'};
    for (int i = 8; i != -1; i--)
    {
        int bit_value = ((statbuf.st_mode & 0777) >> i) & 0b1;
        if (bit_value == 0)
            printf("-");
        else
            printf("%c", symbolic_chars[i % 3]);
    }
    printf("\n");

    char statchmod_str[4];
    snprintf(statchmod_str, 4, "%o", statchmod);
    for (int i = 0; i < 3; i++)
    {
        if (statchmod_str[i] == '7')
            printf("rwx");
        else if (statchmod_str[i] == '6')
            printf("rw-");
        else if (statchmod_str[i] == '5')
            printf("r-x");
        else if (statchmod_str[i] == '4')
            printf("r--");
        else if (statchmod_str[i] == '3')
            printf("-wx");
        else if (statchmod_str[i] == '2')
            printf("-w-");
        else if (statchmod_str[i] == '1')
            printf("-x-");
        else if (statchmod_str[i] == '0')
            printf("---");
    }
    printf("\n");

    printf("Link count:               %ju\n", (uintmax_t)statbuf.st_nlink);
    printf("Ownership:                UID=%ju   GID=%ju\n",
           (uintmax_t)statbuf.st_uid, (uintmax_t)statbuf.st_gid);

    printf("Preferred I/O block size: %jd bytes\n",
           (intmax_t)statbuf.st_blksize);
    printf("File size:                %jd bytes\n",
           (intmax_t)statbuf.st_size);
    printf("Blocks allocated:         %jd\n",
           (intmax_t)statbuf.st_blocks);

    printf("Last status change:       %s", ctime(&statbuf.st_ctime));
    printf("Last file access:         %s", ctime(&statbuf.st_atime));
    printf("Last file modification:   %s", ctime(&statbuf.st_mtime));

    return 0;
}