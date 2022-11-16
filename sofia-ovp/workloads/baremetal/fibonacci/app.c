#include "FIM.h"

#define LOOP 150
#define Fibonacci 38

int OLD1,OLD2,temp;

int main(){

int j;
int i;


FIM_Instantiate();

    for(j=0;j<LOOP;j++) {

#ifdef DEBUG
    printf("starting...\n");
#endif

    OLD1 = 0;
    OLD2 = 1;

#ifdef DEBUG
    printf("fib(0) = %d\n", OLD1);
    printf("fib(1) = %d\n", OLD2);
#endif
        for(i=2; i<Fibonacci+1; i++) {

            temp = OLD2;
            OLD2 = OLD1+ OLD2;
            OLD1 = temp;

#ifdef DEBUG
        printf("fib(%d) = %d\n", i, OLD2);
#endif
        }

    }

#ifdef DEBUG
    printf("fib(%d) = %d\n", i, OLD2);
#endif

FIM_exit();

}
