/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/monitor.h>
#include <kern/console.h>

#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>

static void boot_aps(void);



// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
	cprintf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		mon_backtrace(0, 0, 0);
	cprintf("leaving test_backtrace %d\n", x);
}


void
i386_init(void)
{
	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();

	cprintf("6828 decimal is %o octal!\n", 6828);


	// Lab 2 memory management initialization functions
	mem_init();


	// Lab 3 user environment initialization functions
	env_init();
	trap_init();

	// Lab 4 multiprocessor initialization functions
	// 从BIOS区域找到MP配置表，找到将要映射到MMIO地址的物理地址lapicaddr、
	// 将要启动的bootcpu、cpu总数，设置好cpu[x].cpu_id
	mp_init();
	// 找到LAPIC单元的MMIO地址(虚拟地址)lapic，设置好。。。
	lapic_init();

	// Lab 4 multitasking initialization functions
	pic_init();
	//cprintf("curcpu:%d\n",cpunum());
	// Acquire the big kernel lock before waking up APs
	// Your code here:
	lock_kernel();
	// Starting non-boot CPUs
	boot_aps();

	// Start fs.
	ENV_CREATE(fs_fs, ENV_TYPE_FS);

#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	// Touch all you want.
<<<<<<< HEAD
	ENV_CREATE(user_icode, ENV_TYPE_USER);
=======
	ENV_CREATE(user_primes, ENV_TYPE_USER);
	
	/*ENV_CREATE(user_yield, ENV_TYPE_USER);
	ENV_CREATE(user_yield, ENV_TYPE_USER);
	ENV_CREATE(user_yield, ENV_TYPE_USER);*/
	
	//ENV_CREATE(user_dumbfork, ENV_TYPE_USER);
>>>>>>> lab4
#endif // TEST*

	// Should not be necessary - drains keyboard because interrupt has given up.
	kbd_intr();

	// Schedule and run the first user environment!
	sched_yield();
	//cprintf("hello world!\n");
	
	// We only have one user environment for now, so just run it.
	env_run(&envs[0]);

	// Test the stack backtrace function (lab 1 only)
	test_backtrace(5);


	// Drop into the kernel monitor.
	while (1)
		monitor(NULL);


}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

// Start the non-boot (AP) processors.
static void
boot_aps(void)
{
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct CpuInfo *c;

	//cprintf("mpentry_start:%08x, mpentry_end:%08x\n",mpentry_start, mpentry_end);
	//结果是mpentry_start:f0105144, mpentry_end:f01051be
	
	// Write entry code to unused memory at MPENTRY_PADDR
	code = KADDR(MPENTRY_PADDR); //memmove的参数得是虚拟地址
	memmove(code, mpentry_start, mpentry_end - mpentry_start); 

	// Boot each AP one at a time
	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus + cpunum())  // We've started already.
			continue;

		// Tell mpentry.S what stack to use 
		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE; //因为栈向下增长？
		//cprintf("percpu_kstacks:%08x\n",percpu_kstacks[c - cpus]);
		cprintf("mpentry_kstack:%08x\n",mpentry_kstack);
		// Start the CPU at mpentry_start
		lapic_startap(c->cpu_id, PADDR(code));  //STARTUP
		// Wait for the CPU to finish some basic setup in mp_main()
		while(c->cpu_status != CPU_STARTED)
			;
	}
}

// Setup code for APs
void
mp_main(void)
{
	// We are in high EIP now, safe to switch to kern_pgdir 
	lcr3(PADDR(kern_pgdir));
	cprintf("SMP: CPU %d starting\n", cpunum());

	lapic_init();
	//所有寄存器，包括系统寄存器，都是CPU私有的。
	//因此，初始化这些寄存器的指令，例如lcr3(), ltr(), lgdt(), lidt(), etc.,都必须在每个CPU上执行一次
	env_init_percpu();
	trap_init_percpu();
	xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up

	// Now that we have finished some basic setup, call sched_yield()
	// to start running processes on this CPU.  But make sure that
	// only one CPU can enter the scheduler at a time!
	//
	// Your code here:
	
	
	lock_kernel();
	//cprintf("cpu:%d\n", cpunum());
	sched_yield();
	
	// Remove this after you finish Exercise 6
	//for (;;);
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	asm volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}
