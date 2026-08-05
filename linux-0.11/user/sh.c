/*
 * A tiny shell for the RISC-V Linux 0.11 port.
 */

#include <unistd.h>
#include <string.h>

int main(void)
{
	char buf[128];
	char * argv[2];
	int n, pid, st;
	const char * banner = "Linux 0.11 for RISC-V (QEMU virt)\n\r";
	const char * prompt = "sh# ";
	const char * notfound = "? command not found\n\r";
	const char * forkfail = "fork failed\n\r";
	const char * logout = "logout\n\r";
	const char * hello = "/bin/hello";

	write(1, (char *) banner, strlen(banner));
	for (;;) {
		write(1, (char *) prompt, strlen(prompt));
		n = read(0, buf, sizeof(buf) - 1);
		if (n <= 0)
			break;
		buf[n] = 0;
		while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
			buf[--n] = 0;
		if (!n)
			continue;
		if (!strcmp(buf, "exit"))
			break;
		if (!strcmp(buf, "hello")) {
			if ((pid = fork()) < 0) {
				write(1, (char *) forkfail, strlen(forkfail));
				continue;
			}
			if (!pid) {
				argv[0] = (char *) hello;
				argv[1] = 0;
				execve((char *) hello, argv, 0);
				_exit(1);
			}
			wait(&st);
			continue;
		}
		write(1, (char *) notfound, strlen(notfound));
	}
	write(1, (char *) logout, strlen(logout));
	return 0;
}
