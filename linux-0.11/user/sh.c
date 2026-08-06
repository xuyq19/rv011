/*
 * user/sh.c - minimal shell with a few builtins.
 *
 * Builtins: cd, pwd, echo, ls.  Anything else is looked up as
 * /bin/<cmd> and exec'ed with the remaining arguments.
 */

#define __LIBRARY__
#include <unistd.h>
#include <string.h>

_syscall0(int, fork)
_syscall0(int, getpid)
_syscall3(int, write, int, fd, const char *, buf, int, count)
_syscall3(int, read, int, fd, char *, buf, int, count)
_syscall3(int, execve, const char *, file, char **, argv, char **, envp)
_syscall3(int, waitpid, int, pid, int *, stat, int, options)
_syscall1(int, chdir, const char *, dir)
_syscall3(int, open, const char *, file, int, flag, int, mode)
_syscall1(int, close, int, fd)

static int wait(int * st)
{
	return waitpid(-1, st, 0);
}

static char cwd[256] = "/";

static void wstr(const char * s)
{
	write(1, s, strlen(s));
}

static void wnum(long v)
{
	char b[12];
	char * p = b + sizeof(b);

	if (v < 0) {
		write(1, "-", 1);
		v = -v;
	}
	*--p = 0;
	do {
		*--p = (char) ('0' + (v % 10));
		v /= 10;
	} while (v);
	wstr(p);
}

static int do_cd(const char * arg)
{
	char new[256];
	int n;

	if (!arg || !*arg)
		arg = "/";
	if (arg[0] == '/') {
		if (strlen(arg) >= sizeof(new))
			return -1;
		strcpy(new, arg);
	} else {
		if (strcmp(cwd, "/") == 0)
			new[0] = 0;
		else
			strcpy(new, cwd);
		if (strlen(new) + 1 + strlen(arg) >= sizeof(new))
			return -1;
		strcat(new, "/");
		strcat(new, arg);
	}
	if (chdir(new) < 0)
		return -1;
	n = strlen(new);
	while (n > 1 && new[n - 1] == '/')
		new[--n] = 0;
	if (!*new)
		strcpy(new, "/");
	strcpy(cwd, new);
	return 0;
}

static void do_echo(char ** argv, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		if (i)
			write(1, " ", 1);
		wstr(argv[i]);
	}
	write(1, "\n", 1);
}

static void do_ls(const char * dir)
{
	char buf[1024];
	char name[15];
	int fd, n, off;

	fd = open(dir, 0, 0);		/* O_RDONLY */
	if (fd < 0) {
		wstr("ls: cannot open ");
		wstr(dir);
		write(1, " errno=", 7);
		wnum(errno);
		wstr("\n");
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	for (off = 0; off + 16 <= n; off += 16) {
		unsigned short ino;

		ino = (unsigned short)(buf[off] | (buf[off + 1] << 8));
		if (!ino)
			continue;
		memcpy(name, buf + off + 2, 14);
		name[14] = 0;
		if (name[0] == '.' && (name[1] == 0 || name[1] == '.'))
			continue;
		wstr(name);
		write(1, "  ", 2);
	}
	write(1, "\n", 1);
}

int main(void)
{
	char buf[128];
	char * argv[8];
	int n, i, pid, st;
	char * line, * next;

	for (;;) {
		write(1, "sh# ", 4);
		n = read(0, buf, sizeof(buf) - 1);
		if (n <= 0)
			break;
		buf[n] = 0;
		/* one read may deliver several pasted lines */
		line = buf;
		do {
			next = strchr(line, '\n');
			if (next)
				*next = 0;
			n = strlen(line);
			while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n'))
				line[--n] = 0;

			i = 0;
			for (n = 0; n < 8; n++)
				argv[n] = NULL;
			argv[i] = line;
			for (n = 0; line[n]; n++)
				if (line[n] == ' ' || line[n] == '\t') {
					line[n] = 0;
					if (i < 7 && line[n + 1])
						argv[++i] = line + n + 1;
				}
			if (!*argv[0])
				;
			else if (!strcmp(argv[0], "exit"))
				return 0;
			else if (!strcmp(argv[0], "cd")) {
				if (do_cd(argv[1]) < 0)
					wstr("cd: no such directory\n");
			} else if (!strcmp(argv[0], "pwd")) {
				wstr(cwd);
				write(1, "\n", 1);
			} else if (!strcmp(argv[0], "echo")) {
				do_echo(argv + 1, i);
			} else if (!strcmp(argv[0], "ls")) {
				do_ls(argv[1] ? argv[1] : ".");
			} else if (strlen(argv[0]) < 24) {
				/* try /bin/<cmd> */
				char path[32];

				path[0] = '/';
				path[1] = 'b';
				path[2] = 'i';
				path[3] = 'n';
				path[4] = '/';
				strcpy(path + 5, argv[0]);
				pid = fork();
				if (pid < 0)
					;
				else if (!pid) {
					execve(path, argv, 0);
					wstr("? command not found\n");
					_exit(127);
				} else
					wait(&st);
			} else
				wstr("? command too long\n");
			if (next)
				line = next + 1;
		} while (next);
	}
	wstr("logout\n");
	return 0;
}
