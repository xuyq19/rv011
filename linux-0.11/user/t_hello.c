/* t_hello.c - minimal child program used by fork/stress tests. */

#define __LIBRARY__
#include <unistd.h>

_syscall0(int, getpid)

int main(void)
{
	static const char msg[] = "Hello, RISC-V Linux 0.11!\n";

	write(1, msg, sizeof(msg) - 1);
	return 0;
}
