// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

void
libmain(int argc, char **argv)
{
	// set thisenv to point at our Env structure in envs[].
	// LAB 3: Your code here.
	
	//这个地方一定要看懂envid_t的结构，用ENVX()取后10bit做索引值就行
	//也要注意这里有kern/syscall.c文件与lib/syscall.c
	//kern/syscall.c是在内核态下调用的，本身在内核态，所以不包括系统调用
	//lib/syscall.c是在用户态下调用的，想让内核帮忙，就得系统调用
	//根据头文件判断这里调用的是lib/syscall.c，根据这里是用户程序，也可以判断是lib/syscall.c
	
	//cprintf("I have enter the libmain!\n");
	thisenv = &envs[ENVX(sys_getenvid())];

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];
	
	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

