#ifndef STATEAM_COMMON
#define STATEAM_COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MAXSIZE 1024

typedef struct Item_Cache_Control
{
   char suffix[20];
   char dest_domain[MAXSIZE];
   int  status;
   unsigned int  maxage;
   int hash_val;
   int flags;
   struct Item_Cache_Control *next;
}Conf_Cache_Control;

typedef struct Item_Cache_Controls
{
   int capacity;
   Conf_Cache_Control *item;
}Conf_Cache_Controls;


extern int prime[28];
unsigned int hash(char * key);
int next_prime(int current);


#endif
