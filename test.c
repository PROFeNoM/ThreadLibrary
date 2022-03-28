
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, char* argv[])
{
    float time;
    clock_t t1, t2;

    t1 = clock();

    //
    // Programme
    //
    for(int i=0; i<10000000; i++)
    {

    }

    t2 = clock();

    time = (float) (t2 - t1)/CLOCKS_PER_SEC;

    // Transform float to string
    char s[50] = "";
    sprintf(s, "%f", time);

    FILE* f = fopen("time.txt", "w+");
    fputs(s, f);
    //fprintf(f, "%s\n", s);
    fclose(f);

    return EXIT_SUCCESS;
}
