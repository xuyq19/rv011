#include <unistd.h>
#include <string.h>

int main(void)
{
	const char * msg = "Hello, RISC-V Linux 0.11!\n\r";

	write(1, (char *) msg, strlen(msg));
	return 0;
}
