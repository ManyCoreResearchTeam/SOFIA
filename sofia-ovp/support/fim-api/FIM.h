#ifndef FIM_INCLUDE_H
#define FIM_INCLUDE_H

// prevent the compiler to strip the function symbols
//~ #pragma GCC push_options
//~ #pragma GCC optimize ("O0")

// define the symbols to be interrupt and interface
// Test to g++ and call extern "C" to avoid name mangling
#ifdef ovparmv8
    #ifdef __cplusplus
    //~ extern "C" __attribute__ ((naked)) void __ovp_init()     { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
    extern "C" __attribute__ ((naked)) void __ovp_init()     { asm volatile("nop \n ret \n");}
    extern "C" __attribute__ ((naked)) void __ovp_exit()     { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
    extern "C" __attribute__ ((naked)) void __ovp_segfault() { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
    #else
              __attribute__ ((naked)) void __ovp_segfault()  { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
              //~ __attribute__ ((naked)) void __ovp_init()      { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
              __attribute__ ((naked)) void __ovp_init()      { asm volatile("nop \n ret \n");}
              __attribute__ ((naked)) void __ovp_exit()      { asm volatile("nop \n ldp x29, x30, [sp],#64 \n ret \n");}
    #endif
#elif ovparmv7 //ARM 32bits version
    #ifdef __cplusplus
    extern "C" __attribute__ ((naked,optimize("O0"))) void __ovp_init()     { asm volatile("nop \n bx lr\n");}
    extern "C" __attribute__ ((naked,optimize("O0"))) void __ovp_exit()     { asm volatile("nop \n bx lr\n");}
    extern "C" __attribute__ ((naked,optimize("O0"))) void __ovp_segfault() { asm volatile("nop \n bx lr\n");}
    #else
               __attribute__ ((naked,optimize("O0"))) void __ovp_init()     { asm volatile("nop \n bx lr\n");}
               __attribute__ ((naked,optimize("O0"))) void __ovp_exit()     { asm volatile("nop \n bx lr\n");}
               __attribute__ ((naked,optimize("O0"))) void __ovp_segfault() { asm volatile("nop \n bx lr\n");}
    #endif
#elif riscv //RISCV
    #ifdef __cplusplus
    extern "C" __attribute__ ((naked)) void __ovp_init()     { asm volatile("nop \n jr a0 \n");}
    extern "C" __attribute__ ((naked)) void __ovp_exit()     { asm volatile("nop \n jr a0 \n");}
    extern "C" __attribute__ ((naked)) void __ovp_segfault() { asm volatile("nop \n jr a0 \n");}
    #else
               __attribute__ ((naked)) void __ovp_init()     { asm volatile("nop \n jr a0 \n");}
               __attribute__ ((naked)) void __ovp_exit()     { asm volatile("nop \n jr a0 \n");}
               __attribute__ ((naked)) void __ovp_segfault() { asm volatile("nop \n jr a0 \n");}
    #endif
#endif

#if defined (gem5armv7) || defined (gem5armv8)
    #include <m5op.h>
    #define  FIM_Instantiate()
    #define  FIM_exit()             m5_exit(0);

#elif ovparmv7
    #ifdef BAREMETAL
        #define  FIM_Instantiate()  asm volatile("nop \nmov r1, #0 \n mov r0, #0 \n bl __ovp_init\n");
    #else
        #define  FIM_Instantiate()  //asm volatile("nop \n bl __ovp_init\n"); 
    #endif
    
    #define  FIM_exit()             asm volatile("mov r1, #0 \n mov r0, #0 \n bl __ovp_exit\n");

#elif ovparmv8
    #ifdef BAREMETAL
        #define  FIM_Instantiate()  asm volatile("mov x1, #0 \n mov x0, #0 \n bl __ovp_init\n");
    #else
        #define  FIM_Instantiate()  
    #endif
    
    #define  FIM_exit()             asm volatile("mov x1, #0 \n mov x0, #0 \n bl __ovp_exit\n");

#elif riscv
    #ifdef BAREMETAL
//        #define  FIM_Instantiate()  asm volatile("nop \n jal a0, __ovp_exit\n");
        #define  FIM_Instantiate()
    #endif

        #define  FIM_exit()             asm volatile("nop \n jal a0, __ovp_exit\n");

#endif

//~ #pragma GCC pop_options
#endif // ---- File End ---- //
