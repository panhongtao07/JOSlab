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
#define TRAPHANDLER(name, num)						\
	.data;									\
	.long name;								\
	.text;									\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.data;									\
	.long name;								\
	.text;									\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

#define SPACEHOLDER(name)					\
	.data;									\
	.long 0;

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
.data
	.globl idt_entries
idt_entries:
TRAPHANDLER_NOEC(hdl0, T_DIVIDE)
TRAPHANDLER_NOEC(hdl1, T_DEBUG)
TRAPHANDLER_NOEC(hdl2, T_NMI)
TRAPHANDLER_NOEC(hdl3, T_BRKPT)
TRAPHANDLER_NOEC(hdl4, T_OFLOW)
TRAPHANDLER_NOEC(hdl5, T_BOUND)
TRAPHANDLER_NOEC(hdl6, T_ILLOP)
TRAPHANDLER_NOEC(hdl7, T_DEVICE)
TRAPHANDLER(hdl8, T_DBLFLT)
SPACEHOLDER(hdl9) 	// 9 deprecated since 386
TRAPHANDLER(hdl10, T_TSS)
TRAPHANDLER(hdl11, T_SEGNP)
TRAPHANDLER(hdl12, T_STACK)
TRAPHANDLER(hdl13, T_GPFLT)
TRAPHANDLER(hdl14, T_PGFLT)
SPACEHOLDER(hdl15) 	// 15 reserved by intel
TRAPHANDLER_NOEC(hdl16, T_FPERR)
TRAPHANDLER(hdl17, T_ALIGN)
TRAPHANDLER_NOEC(hdl18, T_MCHK)
TRAPHANDLER_NOEC(hdl19, T_SIMDERR)
.data
	.space 112	// 20-47, 28*4=112
// system call (interrupt)
TRAPHANDLER_NOEC(hdl48, T_SYSCALL)




/*
 * Lab 3: Your code here for _alltraps
 */
.text
.globl _alltraps
_alltraps:
	pushl %ds
	pushl %es
	pushal

	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap
