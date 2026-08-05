/* t_up.c - isolate up_printf behavior. */

#include "uprint.h"

int main(void)
{
	int i;

	up_printf("fork test pid=3 ppid=2\n");
	up_printf("second line abcdefghij\n");
	for (i = 0; i < 3; i++)
		up_printf("loop %d of 3\n", i);
	up_printf("done\n");
	return 0;
}
