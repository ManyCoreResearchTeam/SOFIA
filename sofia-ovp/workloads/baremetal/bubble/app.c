void bubble_sort(long [], long);

#define SIZE 100

#include "FIM.h"

int main()
{
  long array[SIZE], n, c, d, swap;

  n = SIZE;

  for ( c = 0 ; c < n ; c++ ){
     array[c]=n-c;
  }

  bubble_sort(array, n);


FIM_exit();
  return 0;
}

void bubble_sort(long list[], long n)
{
  long c, d, t;
FIM_Instantiate();
  for (c = 0 ; c < ( n - 1 ); c++)
  {
    for (d = 0 ; d < n - c - 1; d++)
    {
      if (list[d] > list[d+1])
      {
        /* Swapping */

        t         = list[d];
        list[d]   = list[d+1];
        list[d+1] = t;
      }
    }
  }
}
