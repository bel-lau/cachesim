#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <math.h>

struct cacheLine {
    int valid;
    int tag;
    struct cacheLine *next;
};

void evictCacheLine(int set, int tag, struct cacheLine **used) {
    struct cacheLine *currentUsed = used[set];
    used[set]->tag = tag; 
    while (currentUsed->next != NULL) {
        currentUsed = currentUsed->next;
    }
    // Evict the first line to have entered the cache, at the front of the list (FIFO)
    currentUsed->next = used[set];
    used[set] = used[set]->next;
    currentUsed->next->next = NULL;
}

void getFreeCacheLine(int set, int tag, struct cacheLine **used, struct cacheLine **free) {
    // If a free block in the set is available, use it
    if (free[set] != NULL) {
        free[set]->valid = 1;
        free[set]->tag = tag;
        if (used[set] != NULL) {
            struct cacheLine *currentUsed = used[set];
            while (currentUsed->next != NULL) {
                currentUsed = currentUsed->next;
            }
            currentUsed->next = free[set];
            free[set] = free[set]->next;
            currentUsed->next->next = NULL;
        } else {
            used[set] = free[set];
            free[set] = free[set]->next;
            used[set]->next = NULL; 
        }
    // If a free block is not available, evict a used line
    } else {
        evictCacheLine(set, tag, used);
    }
}


int cacheCheck(int numSets, int offsetBits, int setBits, int address, struct cacheLine **used, struct cacheLine **free) {
    unsigned offset = address & ((1 << offsetBits) - 1);
    unsigned set = (address >> offsetBits) & ((1 << setBits) - 1);
    unsigned tag = (address >> (offsetBits + setBits)) & ((1 << 20) - 1);
    struct cacheLine *current = used[set];
    // Check to see if tag is in the set of cache (Hit or Miss)
    while (current != NULL) {
        if (current->tag == tag) {
            printf("   %d: HIT  (Tag/Set#/Offset: %d/%d/%d)\n", address, tag, set, offset);
            return 0;
        }
        current = current->next;
    }
    // If a miss, add data to cache
    printf("   %d: MISS (Tag/Set#/Offset: %d/%d/%d)\n", address, tag, set, offset);
    getFreeCacheLine(set, tag, used, free);
    return 0;
}

int main(int argc, char *argv[])
{
    int s, l, n, i;
   
    // Initialize cache storage
    struct cacheLine cache[10000];
    for (i = 0; i < 10000; i++) {
        cache[i].valid = 0;
    }

    // Collect cache information from user
    printf("Cache total size (in bytes)?: ");
    scanf("%d", &s);
    printf("Bytes per block?: ");
    scanf("%d", &l); 
    printf("Number of lines per set?: ");
    scanf("%d", &n);
    getchar();
    int numSets = s / (l * n);
    int setBits = log2(numSets);
    int offsetBits = log2(l);

    struct cacheLine *freeList[numSets];
    struct cacheLine *usedList[numSets];
    
    // Initialize list of free and used cache blocks
    int set, index;
    for (set = 0; set < numSets; set++) {
        usedList[set] = NULL;
        freeList[set] = &cache[set * n];
    }
    for (set = 0; set < numSets; set++) {
        for (index = set * n; index < (set + 1) * n; index++) {
            cache[index].next = &cache[index+1];
        }
        cache[(set + 1) * n - 1].next = NULL;
    }
    
    // Collect addresses to be submitted to cache from user
    int addresses[100];
    printf("Enter list of byte addresses: ");
    int cnt;
    char ch[1000];
    i = 0; 
    while (scanf("%d",  &cnt) == 1) {
        addresses[i] = cnt;
        i++;
        if (getchar() == '\n') break;
    }
    int numAddresses = i;

    printf("\n");   

    // Pass each given address to the cache acting as the CPU
    int blocks = s/l;
    int address, rc; 
    for (i = 0; i < numAddresses; i++) {
        cacheCheck(numSets, offsetBits, setBits, addresses[i], usedList, freeList);
    }

    printf("\nCache contents: \n");    
    for (i = 0; i < blocks; i++) {
        printf("   %d: Valid: %d ;  Tag: %d  (Set #:  %d)\n", i, cache[i].valid, cache[i].tag, i/n);
    }

    return 0;
}
