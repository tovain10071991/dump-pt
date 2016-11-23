#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/user.h>

struct user_regs_struct regs;
long breakpoint;

void cont_break(int pid, unsigned int addr)
{
	//设置断点
	//将addr的头一个字节(第一个字的低字节)换成0xCC
	breakpoint=ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	ptrace(PTRACE_GETREGS, pid, 0, &regs);
	long temp = (breakpoint & 0xFFFFFFFFFFFFFF00) | 0xCC;
	ptrace(PTRACE_POKETEXT, pid, addr, temp);

	//执行子进程
	ptrace(PTRACE_CONT, pid, 0, 0);
	wait(NULL);
	printf("meet breakpoint: ");

	//恢复断点
	ptrace(PTRACE_GETREGS, pid, NULL, &regs);
	//软件断点会在断点的下一个字节停住,所以还要将EIP向前恢复一个字节
	regs.rip-=1;
	printf("0x%llx\n", regs.rip);
	ptrace(PTRACE_SETREGS, pid, NULL, &regs);
	ptrace(PTRACE_POKETEXT, pid, regs.rip, breakpoint);
}