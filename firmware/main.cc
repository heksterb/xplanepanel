/*

	main

	X-Plane Simulator Panel

*/

#include <nrf_rtc.h>


#include "LED.h"
#include "Panel.h"


// (0, 7)/D6 appears not to work electrically


extern void
	ConfigurePowerAndClock(),
	ConfigureUSBD();


extern "C" void _exit(int)
{
NVIC_SystemReset();
}


/*	main
	Entry point
*/
int main(void)
{
// configure peripherals
ConfigureLEDs();
ConfigurePowerAndClock();
ConfigureUSBD();

Panel().Loop();

// don't expect to reach here
return 0;
}
