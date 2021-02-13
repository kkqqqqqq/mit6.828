#include <inc/mmu.h>
#include <inc/memlayout.h>

// Page fault upcall entrypoint.

// This is where we ask the kernel to redirect us to whenever we cause
// a page fault in user space (see the call to sys_set_pgfault_handler
// in pgfault.c).
//
// When a page fault actually occurs, the kernel switches our ESP to
// point to the user exception stack if we're not already on the user
// exception stack, and then it pushes a UTrapframe onto our user
// exception stack:
//
//	trap-time esp
//	trap-time eflags
//	trap-time eip
//	utf_regs.reg_eax
//	...
//	utf_regs.reg_esi
//	utf_regs.reg_edi
//	utf_err (error code)
//	utf_fault_va            <-- %esp
//
// If this is a recursive fault, the kernel will reserve for us a
// blank word above the trap-time esp for scratch work when we unwind(展开)
// the recursive call.
//
// We then have call up to the appropriate page fault handler in C
// code, pointed to by the global variable '_pgfault_handler'.

.text
.globl _pgfault_upcall
_pgfault_upcall:
	// Call the C page fault handler.
	pushl %esp			// function argument: pointer to UTF
	movl _pgfault_handler, %eax
	call *%eax
	addl $4, %esp			// pop function argument
	
	// Now the C page fault handler has returned and you must return
	// to the trap time state.
	// Push trap-time %eip onto the trap-time stack.
	//
	// Explanation:
	//   We must prepare the trap-time stack for our eventual return to
	//   re-execute the instruction that faulted.
	//   我们必须为最终返回重新执行出错的指令准备trap-time stack。
	
	//   Unfortunately, we can't return directly from the exception stack:
	//   We can't call 'jmp', since that requires that we load the address
	//   into a register, and all registers must have their trap-time
	//   values after the return.
	//   We can't call 'ret' from the exception stack either, since if we
	//   did, %esp would have the wrong value.
	//   So instead, we push the trap-time %eip onto the *trap-time* stack!
	//   Below we'll switch(切换) to that stack and call 'ret', which will
	//   restore(恢复) %eip to its pre-fault(故障前) value.
	//
	//   In the case of a recursive fault on the exception stack,
	//   note that the word we're pushing now will fit in the
	//   blank word that the kernel reserved for us.
	//
	// Throughout the remaining code, think carefully about what
	// registers are available for intermediate calculations(中间计算).  You
	// may find that you have to rearrange your code in non-obvious
	// ways as registers become unavailable as scratch space.
	//
	// LAB 4: Your code here.

	//Push trap-time %eip onto the trap-time stack.
	// trap-time esp point to trap-time stack
	addl $8, %esp  //pass the fault_va and tf_err
	movl 0x20(%esp), %eax // 0x20(%esp)==*(%esp+32)==trap-time eip;
	
	/* 这样写trap-time esp没修改，虽然eip放对了，但是esp还指着上边，是不对的
	movl 0x28(%esp), %ebx // 0x28(%esp)==*(%esp+40)==trap-time esp;
	subl $4, %ebx // point to the blank word*/

	subl $4, 0x28(%esp)
	movl 0x28(%esp), %ebx
	movl %eax, (%ebx) // Push trap-time %eip onto the blank word of the trap-time stack

	
	// Restore the trap-time registers.  After you do this, you
	// can no longer modify any general-purpose registers.
	// LAB 4: Your code here.
	popal // Restore the trap-time registers
	
	// Restore eflags from the stack.  After you do this, you can
	// no longer use arithmetic operations or anything else that
	// modifies eflags.
	// LAB 4: Your code here.
	addl $4, %esp // pass the trap-time eip
	popfl //将标志寄存器的值出栈
	
	// Switch back to the adjusted trap-time stack.
	// LAB 4: Your code here.
	popl %esp 
	
	// Return to re-execute the instruction that faulted.
	// LAB 4: Your code here.
	ret //将当前(%esp)内容pop到eip寄存器
