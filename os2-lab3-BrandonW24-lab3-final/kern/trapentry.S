/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)                                          \
        .text;                                                          \
        .globl name;            					\
        .type name, @function;  					\
        .align 2;               					\
        name:                   					\
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;                                                          \
        .long name;

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)                                     \
        .text;                                                          \
        .globl name;                                                    \
        .type name, @function;                                          \
        .align 2;                                                       \
        name:                                                           \
        pushl $0;                                                       \
        pushl $(num);                                                   \
        jmp _alltraps;                                                  \
        .data;                                                          \
        .long name;


.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

// HINT 1 : TRAPHANDLER_NOEC(t_divide, T_DIVIDE);
//          Do something like this if there is no error code for the trap
// HINT 2 : TRAPHANDLER(t_dblflt, T_DBLFLT);
//          Do something like this if the trap includes an error code..
// HINT 3 : READ Intel's manual to check if the trap includes an error code
//          or not...

//Do stuff call trap
//
.data
.globl vectors
vectors:
TRAPHANDLER_NOEC(vector0, T_DIVIDE);
TRAPHANDLER_NOEC(vector1, T_DEBUG);
TRAPHANDLER_NOEC(vector2, T_NMI);
TRAPHANDLER_NOEC(vector3, T_BRKPT);
TRAPHANDLER_NOEC(vector4, T_OFLOW);
TRAPHANDLER_NOEC(vector5, T_BOUND);
TRAPHANDLER_NOEC(vector6, T_ILLOP);
TRAPHANDLER_NOEC(vector7, T_DEVICE);
TRAPHANDLER(vector8, T_DBLFLT);
//gap
TRAPHANDLER_NOEC(vector9, 9); 
TRAPHANDLER(vector10, T_TSS);
TRAPHANDLER(vector11, T_SEGNP);
TRAPHANDLER(vector12, T_STACK);
TRAPHANDLER(vector13, T_GPFLT);
TRAPHANDLER(vector14, T_PGFLT);
//gap
TRAPHANDLER(vector15, 15);
TRAPHANDLER_NOEC(vector16, T_FPERR);
TRAPHANDLER(vector17, T_ALIGN);
TRAPHANDLER_NOEC(vector18, T_MCHK);
TRAPHANDLER_NOEC(vector19, T_SIMDERR);
TRAPHANDLER_NOEC(vector20, T_SYSCALL); 


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
  pushl %ds;
  pushl %es;
  pushal;
  movw $GD_KD, %ax;
  movw %ax, %ds;
  movw %ax, %es;
  pushl %esp;
  call trap;

