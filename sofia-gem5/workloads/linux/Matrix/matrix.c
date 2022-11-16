#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#define ROWS        4
#define COLUMNS     ROWS

//FIM
#include "FIM.h"

int matrixA [ROWS][COLUMNS];
int matrixB [ROWS][COLUMNS];
int matrixC [ROWS][COLUMNS];

// Fill with ones
void fillMatrix(int mat[ROWS][COLUMNS]) {
    for(int i=0;i<ROWS;i++)
        for(int j=0; j<COLUMNS;j++)
            mat[i][j] = 1;
}

//Print out the resulting matrix
void printMatrix(int mat[ROWS][COLUMNS]){
   for(int i = 0; i < ROWS; i++) {
      for(int j = 0; j < COLUMNS; j++)
         printf("%d ", mat[i][j]);
      printf("\n");
   }
}

int main(int argc, char *argv[]) {
    FIM_Instantiate();

    int sum=0;

    struct timeval t1, t2;
    double elapsedTime;

    //Fill the matrix
    for(int i=0;i<ROWS;i++)
        for(int j=0; j<COLUMNS;j++)
            matrixA[i][j] = 1;
            
    for(int i=0;i<ROWS;i++)
        for(int j=0; j<COLUMNS;j++)
            matrixB[i][j] = 1;

    // start timer
    gettimeofday(&t1, NULL);

    //Multiply
    for(int i = 0, j; i < ROWS; i++) {
        for(int  j = 0; j < COLUMNS; j++) {
            for(int k = 0; k< COLUMNS; k++)
                sum += matrixA[i][k] * matrixB[k][j];
            matrixC[i][j] = sum; sum=0;
      }
   }

    // stop timer
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    printf("Elapsed  time %f ms \n",elapsedTime);

    printMatrix(matrixC);

    //FIM
    FIM_exit();
}
