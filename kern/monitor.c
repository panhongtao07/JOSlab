// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display a backtrace information about the stack", mon_backtrace },
	{ "showmappings", "Display a map information in given range", mon_showmappings },
	{ "setperm", "Set a page's permission", mon_setperm },
	{ "addperm", "Add a page's permission", mon_addperm },
	{ "rmperm", "Remove a page's permission", mon_rmperm },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	// written by pht - 21.9.30
	uint32_t *ebp;
	uintptr_t eip;
	struct Eipdebuginfo eip_info;
	ebp = (uint32_t *)read_ebp();
	
	cprintf("Stack backtrace:\n");

	const char stackfmt[] = \
		"  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n";
	const char debugfmt[] = \
		"        %s:%d: %.*s+%d\n";
	while (ebp)
	{
		eip = ebp[1];
		cprintf(stackfmt, ebp, eip, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
		ebp = (uint32_t *)ebp[0];
		if(debuginfo_eip(eip, &eip_info) >= 0)
			cprintf(debugfmt,
					eip_info.eip_file, eip_info.eip_line,
					eip_info.eip_fn_namelen, eip_info.eip_fn_name,
					eip - eip_info.eip_fn_addr);
	}
	
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    char *errChar;
    if (argc != 3) {
        cprintf("Requires 2 virtual addresses.\n");
        return -1;
    }
    uintptr_t start_addr = strtol(argv[1], &errChar, 16);
    if (*errChar) {
        cprintf("Invalid virtual address: %s.\n", argv[1]);
        return -1;
    }
    uintptr_t end_addr = strtol(argv[2], &errChar, 16);
    if (*errChar) {
        cprintf("Invalid virtual address: %s.\n", argv[2]);
        return -1;
    }
    
	cprintf("Show mappings: %s - %s.\n", argv[1], argv[2]);
	const char empty_fmt[] = \
		"Virtual address [%08x] - not mapped\n";
	const char page_fmt[] = \
		"Virtual address [%08x] - mapped to [%08x], "\
		"permission: -%c----%c%cP\n";
    for (
		uintptr_t cur_addr = ROUNDUP(start_addr, PGSIZE);	// 按页对齐
		cur_addr <= end_addr && cur_addr >= start_addr; 	// 避免溢出
		cur_addr += PGSIZE
	) {
        pte_t *pte = pgdir_walk(kern_pgdir, (void *) cur_addr, 0);
        if (!pte || !(*pte & PTE_P)) 
            cprintf(empty_fmt, cur_addr);
        else {
            cprintf(page_fmt,
					cur_addr,
					PTE_ADDR(*pte),
					(*pte & PTE_PS) ? 'S':'-',
					(*pte & PTE_U) ? 'U':'-',
					(*pte & PTE_W) ? 'W':'-');
        }
    }
    return 0;
}

#define PERM_SET 0
#define PERM_ADD 1
#define PERM_RM 2
int
mon_changeperm(int argc, char **argv, struct Trapframe *tf, int type)
{
    char *errChar;
    if (argc != 3) {
        cprintf("Requires virtual address and permission.\n");
        return -1;
    }
    uintptr_t addr = strtol(argv[1], &errChar, 16);
    if (*errChar) {
        cprintf("Invalid virtual address: %s.\n", argv[1]);
        return -1;
    }
    int perm = strtol(argv[2], &errChar, 16);
    if (*errChar) {
        cprintf("Invalid permission: %s.\n", argv[2]);
        return -1;
    }

	addr = ROUNDDOWN(addr, PGSIZE);
	perm &= PTE_PS | PTE_W | PTE_U;
	pte_t *pte = pgdir_walk(kern_pgdir, (void *) addr, 0);
	if (!pte || !(*pte & PTE_P)) {
		cprintf("Virtual address [%08x] is not mapped\n", addr);
		return -1;
	}
	cprintf("Virtual address [%08x] - mapped to [%08x]\n", addr, PTE_ADDR(*pte));
	cprintf("permission: -%c----%c%cP",
			(*pte & PTE_PS) ? 'S':'-',
			(*pte & PTE_U) ? 'U':'-',
			(*pte & PTE_W) ? 'W':'-');
	
	int real_perm = (*pte) & 0xFFF;
	if (type == PERM_SET)
		real_perm = perm;
	else if (type == PERM_ADD)
		real_perm |= perm;
	else if (type == PERM_RM)
		real_perm &= ~perm;
	*pte = PTE_ADDR(*pte) | real_perm | PTE_P;
	cprintf("  ->  -%c----%c%cP\n",
			(*pte & PTE_PS) ? 'S':'-',
			(*pte & PTE_U) ? 'U':'-',
			(*pte & PTE_W) ? 'W':'-');
	return 0;
}

int mon_setperm(int argc, char **argv, struct Trapframe *tf){
	return mon_changeperm(argc, argv, tf, PERM_SET);
}
int mon_addperm(int argc, char **argv, struct Trapframe *tf){
	return mon_changeperm(argc, argv, tf, PERM_ADD);
}
int mon_rmperm(int argc, char **argv, struct Trapframe *tf){
	return mon_changeperm(argc, argv, tf, PERM_RM);
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
