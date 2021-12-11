#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cachelab.h"

int c = 0;
int h = 0;
int v = 0;
int s = 0;
int E = 0;
int b = 0;
int S = 0;
int B = 0;
char *t = NULL;
int LRUcounter = 1;
int missCount = 0;
int hitCount = 0;
int evictionCount = 0;

struct cache_line
{
    int valid;
    int tag;
    int LRU;
} typedef cache_line_t;

cache_line_t *line_init()
{
    cache_line_t *t = malloc(sizeof(cache_line_t));
    t->valid = 0;
    t->tag = 0;
    t->LRU = 0;

    return t;
}

void cache_init(cache_line_t cache[S][E])
{
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            cache_line_t *temp;
            temp = line_init();
            cache[i][j] = *temp;
        }
    }
}

void cache_load(cache_line_t cache[S][E], unsigned long long address)
{
    int leastLRU = __INT_MAX__;
    int leastJ;
    unsigned long long int tag = address >> (s + b);
    unsigned long long int set = (address << (64 - s - b)) >> (64 - s);
    // line
    for (int j = 0; j < E; j++)
    {
        if (cache[set][j].tag == tag && cache[set][j].valid == 1)
        {
            cache[set][j].LRU = LRUcounter++;
            hitCount++;
            return;
        }
        if (cache[set][j].LRU <= leastLRU)
        {
            leastLRU = cache[set][j].LRU;
            leastJ = j;
        }
        if (j == E - 1)
        {
            if (leastLRU != 0)
            {
                evictionCount++;
            }
            missCount++;

            cache_line_t *newLine = line_init();
            newLine->valid = 1;
            newLine->tag = tag;
            newLine->LRU = LRUcounter;
            cache[set][leastJ] = *newLine;

            LRUcounter++;
        }
    }
}

int main(int argc, char *argv[])
{
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (c)
        {
        case ('h'):
            h = 1;
            break;
        case ('v'):
            v = 1;
            break;
        case ('s'):
            s = atoi(optarg);
            break;
        case ('E'):
            E = atoi(optarg);
            break;
        case ('b'):
            b = atoi(optarg);
            break;
        case ('t'):
            t = malloc(sizeof(char) * (strlen(optarg)));
            memcpy(t, optarg, strlen(optarg));
            break;
        default:
            break;
        }
    }

    S = 2 << s;
    B = 2 << b;

    cache_line_t cache[S][E];
    cache_init(cache);

    FILE *file;
    // char buffer[13];
    file = fopen(t, "r");

    char instruction;
    unsigned long long address;
    int size;

    while (fscanf(file, " %c %llx,%d", &instruction, &address, &size) == 3)
    {
        // printf("inst %c address %llx, size %d", instruction, address, size);
        switch (instruction)
        {
        // load
        case 'L':
            cache_load(cache, address);
            break;

        // store
        case 'S':
            cache_load(cache, address);
            break;

        // modify
        case 'M':
            cache_load(cache, address);
            cache_load(cache, address);
            break;
        }
    }

    printSummary(hitCount, missCount, evictionCount);
}
