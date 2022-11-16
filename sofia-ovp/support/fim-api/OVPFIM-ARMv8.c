#include <stdio.h>
#include <stdlib.h>     /* atoi */

void __ovp_init_external()     	{ asm volatile("nop \n ");}
void __ovp_segfault_external() 	{ asm volatile("nop \n ");}

int main(int argc, char **argv) {
    int code = atoi(argv[1]);
    printf("Code Input %d\n",code);
    
    switch(code) {
        case 2: {
            printf("force finishing...\n");
            __ovp_segfault_external();
            //~ asm volatile("bl __ovp_segfault_external\n");
            break;} 
        case 0: {
            printf("Application Begin\n");
            //~ asm volatile("bl __ovp_init_external\n");
            __ovp_init_external();
            break;}
        default:printf("Code not found\n"); break;
    }
    return 0;
}
