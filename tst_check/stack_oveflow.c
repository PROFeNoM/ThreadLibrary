#include "thread.h"

struct heavy
{
    long long int Alexandre;
    long long int Louis;
    long long int Mathilde;
    long long int Enzo;
    long long int Sylvain;
};

void lets_go(struct heavy big_param)
{
    lets_go(big_param);
}

int main(int argc, char ** argv)
{
    struct heavy heavy_struct = {0, 1, 2, 3, 4};
    lets_go(heavy_struct);
}