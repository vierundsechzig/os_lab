#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "commands.h"

#define N 1024

char buffer[N];

int main()
{
    char ch;
    char filename[3] = "00";
    char* ret;
    if (!exists())
    {
        printf("No file system found on your disk. Do you want to create one? (1 for yes)");
        ch = getchar();
        getchar();
        if (ch == '1')
        format_c();
    }
    do
    {
        printf("$ ");
        char ch;
        ret = fgets(buffer, N, stdin);
        char* p = buffer;
        while (*p != '\n' && *p != EOF && p < buffer + N)
            ++p;
        *p = '\0';
        exec(buffer);
    } while (ret != 0);
    return 0;
}
