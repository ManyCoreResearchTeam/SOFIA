#define LOOP 500
#define Fibonacci 38

#include "FIM.h"

int main(){
	
int j;
int i;
int OLD1,OLD2,temp;

    for(j=0;j<LOOP;j++)
    {

#ifdef DEBUG
    printf("starting...\n");
#endif

    OLD1 = 0;
    OLD2 = 1;

#ifdef DEBUG
    printf("fib(0) = %d\n", OLD1);
    printf("fib(1) = %d\n", OLD2);
#endif
    for(i=2; i<Fibonacci+1; i++)
    {

        temp = OLD2;
        OLD2 = OLD1+ OLD2;
        OLD1 = temp; //Valor antigo

#ifdef DEBUG
    printf("fib(%d) = %d\n", i, OLD2);
#endif
    }

    }

#ifdef DEBUG
    printf("fib(%d) = %d\n", i, OLD2);
#endif

FIM_exit();}
