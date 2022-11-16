
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>     /* atoi */

#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include "FIM.h"

#define OVP_LINUX
int main(int argc, char **argv) {
    
    int code = atoi(argv[1]);
    int returned;
    
    /***************************************FIM***************************************************/
    int fd = open("/dev/mem", O_SYNC);
    unsigned char *mem  = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, baseAddress);
    if (mem == NULL) {
        printf("Can't map memory\n");
        return -1;
    }
    /*********************************************************************************************/
    
    printf("Code Input %d\n",code);
    
    switch(code) {
        case FIM_SERVICE_SEG_FAULT: {
            printf("force finishing...\n");
            returned = mem[FIM_SERVICE_SEG_FAULT];
            break;
        } 
        case FIM_SERVICE_BEGIN_APP: {
            printf("Application Begin\n");
            returned = mem[FIM_SERVICE_BEGIN_APP];
            break;
        }
        default:printf("Code not find\n"); break;
    }
	//~ munmap(baseAddress,len);
    return 0;
}
