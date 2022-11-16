
/* MDH WCET BENCHMARK SUITE */
/*
 * Changes: CS 2006/05/19: Changed loop bound from constant to variable.
 */

int fac (int n)
{
  if (n == 0)
     return 1;
  else
     return (n * fac (n-1));
}

long int factorial ()
{
  int i ;
  long int s = 0;
  volatile int n;

  n = 25;
  for (i = 0;  i <= n; i++)
      s += fac (i);

  return (s);
}

#define LOOP 50
//#define DEBUG

#include "FIM.h"

void main(void)
{

long int i,ans = 0;
for( i = 0; i<LOOP; i++)
  ans = factorial(); 

FIM_exit();
#ifdef DEBUG
printf("\n %ld \n",ans);//712899098
#endif

}
