
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    float time;
    clock_t t1, t2;

    t1 = clock();
    // Programme
    printf("Hello World\n");
    t2 = clock();

    time = (float) (t2 - t1)/CLOCKS_PER_SEC;
    char s[50] = "";
    sprintf(s, "%f", time);


    FILE* f = fopen("time.txt", "w+");
    fputs(s, f);
    fclose(f);

    return EXIT_SUCCESS;
}
