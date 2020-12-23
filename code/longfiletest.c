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
    int inodeno = touch(0, "a");
    for (int i=0; i<4096; ++i)
        appendbyte(inodeno, i % 26 + 'A');
    touch(0, "b");
    appendbyte(inodeno, '#');
    do
    {
        printf("$ ");
        char ch;
        ret = fgets(buffer, N, stdin);
        char* p = buffer;
        while (*p != '\n' && *p != EOF)
            ++p;
        *p = '\0';
        exec(buffer);
    } while (ret != 0);
    return 0;
    return 0;
}
