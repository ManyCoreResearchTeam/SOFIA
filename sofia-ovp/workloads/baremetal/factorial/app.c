
/* MDH WCET BENCHMARK SUITE */
/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */
#include "FIM.h"

#define LOOP 20

int i;
long int j, ans = 0, s = 0;

int fac (int n)
{
  if (n == 0)
     return 1;
  else
     return (n * fac (n-1));
}

long int factorial ()
{

volatile int n;

    n = 25;
    for (i = 0;  i <= n; i++) {
        s += fac (i);
    }

return (s);
}

void main(void)
{
FIM_Instantiate();

    for( j = 0; j<LOOP; j++) {
        ans = factorial();
    }

//    #ifdef DEBUG
//    printf("\n %ld \n",ans);//712899098
//    #endif

FIM_exit();
}
