/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


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
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(hdl0, T_DIVIDE)
TRAPHANDLER_NOEC(hdl1, T_DEBUG)
TRAPHANDLER_NOEC(hdl2, T_NMI)
TRAPHANDLER_NOEC(hdl3, T_BRKPT)
TRAPHANDLER_NOEC(hdl4, T_OFLOW)
TRAPHANDLER_NOEC(hdl5, T_BOUND)
TRAPHANDLER_NOEC(hdl6, T_ILLOP)
TRAPHANDLER_NOEC(hdl7, T_DEVICE)
TRAPHANDLER(hdl8, T_DBLFLT)
// TRAPHANDLER_NOEC(hdl9, 9) 	// 9 deprecated since 386
TRAPHANDLER(hdl10, T_TSS)
TRAPHANDLER(hdl11, T_SEGNP)
TRAPHANDLER(hdl12, T_STACK)
TRAPHANDLER(hdl13, T_GPFLT)
TRAPHANDLER(hdl14, T_PGFLT)
// TRAPHANDLER_NOEC(hdl15, 15) 	// 15 reserved by intel
TRAPHANDLER_NOEC(hdl16, T_FPERR)
TRAPHANDLER(hdl17, T_ALIGN)
TRAPHANDLER_NOEC(hdl18, T_MCHK)
TRAPHANDLER_NOEC(hdl19, T_SIMDERR)
// interrupts
TRAPHANDLER(hdl32, IRQ_OFFSET + IRQ_TIMER)
TRAPHANDLER(hdl33, IRQ_OFFSET + IRQ_KBD)
TRAPHANDLER(hdl36, IRQ_OFFSET + IRQ_SERIAL)
TRAPHANDLER(hdl39, IRQ_OFFSET + IRQ_SPURIOUS)
TRAPHANDLER(hdl46, IRQ_OFFSET + IRQ_IDE)
TRAPHANDLER(hdl51, IRQ_OFFSET + IRQ_ERROR)
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
