/*
 * t_fs.c - filesystem functional test: create/write/read/lseek/unlink,
 * mkdir/rmdir, link, chdir, stat, access, dup2, fcntl.
 */

#define __LIBRARY__
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "uprint.h"

_syscall1(int, close, int, fd)
_syscall3(int, read, int, fd, char *, buf, int, count)
_syscall3(int, write, int, fd, const char *, buf, int, count)
_syscall2(int, creat, const char *, file, mode_t, mode)
_syscall1(int, unlink, const char *, file)
_syscall2(int, link, const char *, old, const char *, new)
_syscall1(int, chdir, const char *, dir)
_syscall2(int, mkdir, const char *, dir, mode_t, mode)
_syscall1(int, rmdir, const char *, dir)
_syscall2(int, access, const char *, file, int, mode)
_syscall2(int, stat, const char *, file, struct stat *, st)
_syscall2(int, fstat, int, fd, struct stat *, st)
_syscall3(int, lseek, int, fd, int, off, int, whence)
_syscall2(int, dup2, int, oldfd, int, newfd)
_syscall0(int, sync)

static unsigned char pat[10240];

static int verify(int fd, int len)
{
	unsigned char buf[512];
	int got = 0, i, n;

	for (;;) {
		n = read(fd, buf, sizeof(buf));
		if (n <= 0)
			break;
		for (i = 0; i < n; i++)
			if (buf[i] != pat[got + i]) {
				up_printf("fs: data mismatch at %d\n",
					got + i);
				return 1;
			}
		got += n;
	}
	if (got != len) {
		up_printf("fs: short read %d != %d\n", got, len);
		return 1;
	}
	return 0;
}

int main(void)
{
	int fd, i, n, err = 0;
	struct stat st;
	unsigned char buf[32];

	for (i = 0; i < 10240; i++)
		pat[i] = (unsigned char) ((i * 7 + 3) & 0xff);

	/* 10 KB file: exercises 7 direct + 3 indirect blocks */
	fd = creat("/big.dat", 0644);
	if (fd < 0) {
		up_printf("fs: creat /big.dat failed %d errno=%d\n", fd, errno);
		return 1;
	}
	if (write(fd, pat, sizeof(pat)) != sizeof(pat)) {
		up_printf("fs: write failed\n");
		err = 1;
	}
	if (fstat(fd, &st) < 0 || st.st_size != 10240) {
		up_printf("fs: fstat size %d\n", st.st_size);
		err = 1;
	}
	close(fd);
	sync();

	fd = open("/big.dat", O_RDONLY, 0);
	if (fd < 0) {
		up_printf("fs: open failed\n");
		return 1;
	}
	if (verify(fd, 10240))
		err = 1;
	if (stat("/big.dat", &st) < 0 || st.st_size != 10240) {
		up_printf("fs: stat size wrong\n");
		err = 1;
	}
	/* seek to the middle and check */
	if (lseek(fd, 5000, SEEK_SET) != 5000 ||
	    read(fd, buf, 8) != 8 || memcmp(buf, pat + 5000, 8)) {
		up_printf("fs: lseek/read wrong\n");
		err = 1;
	}
	if (lseek(fd, -4, SEEK_END) != 10236 ||
	    read(fd, buf, 4) != 4 || memcmp(buf, pat + 10236, 4)) {
		up_printf("fs: seek-end wrong\n");
		err = 1;
	}
	close(fd);

	/* hard link + access */
	if (link("/big.dat", "/big2.dat") < 0 ||
	    access("/big2.dat", R_OK) < 0 ||
	    stat("/big2.dat", &st) < 0 || st.st_size != 10240) {
		up_printf("fs: link/access failed\n");
		err = 1;
	}
	if (unlink("/big2.dat") < 0) {
		up_printf("fs: unlink failed\n");
		err = 1;
	}
	if (access("/big2.dat", R_OK) == 0) {
		up_printf("fs: unlink left file\n");
		err = 1;
	}

	/* mkdir / chdir / relative file / rmdir */
	if (mkdir("/tdir", 0755) < 0) {
		up_printf("fs: mkdir failed\n");
		err = 1;
	}
	if (chdir("/tdir") < 0) {
		up_printf("fs: chdir failed\n");
		err = 1;
	}
	fd = creat("rel.txt", 0644);
	if (fd < 0) {
		up_printf("fs: creat in subdir failed\n");
		err = 1;
	} else {
		close(fd);
		if (stat("/tdir/rel.txt", &st) < 0) {
			up_printf("fs: stat subdir file failed\n");
			err = 1;
		}
	}
	chdir("/");
	if (unlink("/tdir/rel.txt") < 0 || rmdir("/tdir") < 0) {
		up_printf("fs: cleanup failed\n");
		err = 1;
	}

	/* dup2 redirection to a file */
	fd = creat("/out.txt", 0644);
	if (fd < 0) {
		up_printf("fs: creat out failed\n");
		return 1;
	}
	if (dup2(fd, 1) != 1)
		up_printf2("fs: dup2(fd,1) failed\n");
	{
		const char * msg = "redirected line\n";
		int wr = 0;

		for (i = 0; i < 17; i++) {
			int r = write(1, msg + i, 1);
			if (r != 1)
				up_printf2("fs: write %d ret %d errno=%d\n", i, r, errno);
			else
				wr++;
		}
		up_printf2("fs: writes ok=%d\n", wr);
	}
	close(fd);
	if (dup2(0, 1) != 1)
		up_printf2("fs: dup2(0,1) restore failed\n");
	fd = open("/out.txt", O_RDONLY, 0);
	n = read(fd, buf, sizeof(buf));
	if (fstat(fd, &st) == 0)
		up_printf2("fs: out.txt size=%d\n", st.st_size);
	close(fd);
	if (n <= 0 || memcmp(buf, "redirected line\n", 17)) {
		up_printf2("fs: dup2 redirect wrong n=%d data=", n);
		for (i = 0; i < n && i < 24; i++)
			up_printf2("%x ", buf[i]);
		up_printf2("\n");
		err = 1;
	}

	/* fcntl F_DUPFD */
	fd = open("/out.txt", O_RDONLY, 0);
	n = fcntl(fd, F_DUPFD, 10);
	if (n != 10) {
		up_printf("fs: F_DUPFD wrong %d\n", n);
		err = 1;
	} else
		close(n);
	close(fd);

	if (unlink("/big.dat") < 0 || unlink("/out.txt") < 0) {
		up_printf("fs: final unlink failed\n");
		err = 1;
	}
	sync();
	up_printf2("fs test %s\n", err ? "FAILED" : "passed");
	return err ? 1 : 0;
}
