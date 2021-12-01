#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>

int main(int argc, char **argv)
{
    char *access = argv[1];

    for (int i = 2; i < argc; i++)
    {
        struct stat sb;
        int result = stat(argv[i], &sb);

        int all_permissions = sb.st_mode & 07;
        int user_permissions = (sb.st_mode & 0777) >> 6;
        int group_permissions = (sb.st_mode & 0070) >> 3;

        printf("%o%o%o\n", user_permissions, all_permissions, group_permissions);

        int passed_permissions = 0;
        for (int j = 0; j < 3; j++)
        {
            if (access[j] == 'r')
                passed_permissions += 04;
            else if (access[j] == 'w')
                passed_permissions += 02;
            else if (access[j] == 'x')
                passed_permissions += 01;
        }

        if (all_permissions == passed_permissions)
            printf("%s %jo %jd %s", argv[i], (uintmax_t)sb.st_mode, sb.st_size, ctime(&sb.st_mtime));
    }

    exit(0);
}