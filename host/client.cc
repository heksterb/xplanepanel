/*

	main

	USB test client application

*/

#include <assert.h>
#include <stdio.h>

#include "hid.h"


/*	main
	Command-line interface
*/
int main(
	int		argc,
	char		*argv[]
	)
{
Panel panel;

// loop forever, showing incrementing values
for (unsigned value = 121500;; value += 1001) {
	const bool changed = panel.Set(value, value + 110110);
	if (changed)
		fprintf(stderr, "%u %u\n", panel.Value0(), panel.Value1());
	
	Sleep(1000 /* ms */);
	}

return 0;
}
