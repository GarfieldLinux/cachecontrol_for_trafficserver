#include "common.h"

int prime[28] =  
{  
    57,        97,         193,        389,        769,  
    1543,      3079,       6151,       12289,      24593,  
    49157,     98317,      196613,     393241,     786433,  
    1572869,   3145739,    6291469,    12582917,   25165843,  
    50331653,  100663319,  201326611,  402653189,  805306457,  
    1610612741  
};

unsigned int hash(char*str) 
{ 
    unsigned int hash=1315423911 ; 
     
    while(*str) 
    { 
        hash^=((hash<<5)+(*str++)+(hash>>2)); 
    } 
    
    return(hash % 249997); 
} 
 

int next_prime(int current)
{  
	int i = 0;  
	for( ; i < 28 ; i++ )  
	if(current < prime[i])  
		break;  
	return prime[i]; 
}  


