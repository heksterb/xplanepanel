/*

	Panel
	
	Flight simulator radio panel
	
*/

#include "Panel.h"
#include "USB.h"

#include <nrf_gpio.h>
#include <nrf_gpiote.h>


/*	Panel
	Constructor
*/
Panel::Panel() :
	fAccumulate(0),
	fValue(121500),
	fValueStandby(122900),
	fSPIM(
		NRF_GPIO_PIN_MAP(1, 8),
		NRF_GPIO_PIN_MAP(0, 14),
		NRF_GPIO_PIN_MAP(0, 13),
		NRF_GPIO_PIN_MAP(0, 15)
		),
	fMAX(fSPIM),
	fQDEC(
		NRF_GPIO_PIN_MAP(0, 6),
		NRF_GPIO_PIN_MAP(0, 8)
		)
{
fMAX.ScanLimit(5 /* digit pairs 0/0a through 5/5a only */);
fMAX.GlobalIntensity(0);
fMAX.DigitType(0x00 /* all 7-segment displays */);
fMAX.DecodeMode(0xff /* hexadecimal decoding */);
fMAX.Configure(MAX6954::Configuration { .shutdownOff = true });
fMAX.PortConfigure(0x20 /* 8 keys scanned; P1,2,3 are left as output; P4 becomes IRQ */);
fMAX.KeyMask(0, 1 << 1); // enable interrupt on 1
(void) fMAX.DebouncedKey(0); // reset IRQ

// initialize display
UpdateDisplay();
}


/*	UpdateOneDisplay
	Update one of the displayed values
*/
void Panel::UpdateOneDisplay(
	uint8_t		base,
	unsigned	value
	)
{
fMAX.Digit(base + 0, (value / 100000) % 10);
fMAX.Digit(base + 1, (value / 10000) % 10);
fMAX.Digit(base + 2, 0x80 /* decimal point */ + (value  / 1000) % 10);
fMAX.Digit(base + 3, (value / 100) % 10);
fMAX.Digit(base + 4, (value / 10) % 10);
fMAX.Digit(base + 5, (value / 1) % 10);
}


/*	UpdateDisplay
	Update displayed values based on state
*/
void Panel::UpdateDisplay()
{
UpdateOneDisplay(0, fValue);
UpdateOneDisplay(8, fValueStandby);
}


/*	SetValue
	Set panel values
*/
void Panel::SetValue(
	unsigned	value0,
	unsigned	value1
	)
{
// update state
fValue = value0;
fValueStandby = value1;

// update displayed values
UpdateDisplay();

// don't send updated values back through USB
}


/*	ProcessQDEC
	Respond to rotary encoder
*/
void Panel::ProcessQDEC()
{
// MAX can't interrupt when our 'decimals' key is released, so instead we must check synchronously
/* Use the instantaneous value, not the debounced one (which looks to be reset after reading) */
uint8_t noopr = fMAX.KeyPressed(0);
bool decimals = noopr & 1;

// trigger reading the accumulator
/* Each indent is four samples; we really only care about those multiples of four.
   However, we may get a 'report' on just one of the samples; so we must accumulate them. */
fAccumulate += fQDEC.operator int32_t();
signed indents = fAccumulate / 4;	// integer division truncates towards zero, for negative numbers as well

if (indents != 0) {
	fAccumulate -= indents * 4;
	fValueStandby += indents * (decimals ? 25 : 1000);
	
	// display updated values immediately
	UpdateDisplay();
	
	// send updated values through USB
	USBEndpointIN1(fValue, fValueStandby);
	}
}


void Panel::ProcessMAXKeyPress()
{
// read debounced register resets MAX IRQ
uint8_t noopr = fMAX.DebouncedKey(0);

if (noopr & 2) {
	/* Adds 72 bytes in Debug build; zero in Release. */
	std::swap(fValue, fValueStandby);
	
	// display updated values immediately
	UpdateDisplay();
	
	// send updated values through USB
	USBEndpointIN1(fValue, fValueStandby);
	}
}


/*	Loop
	Event loop
*/
void Panel::Loop()
{
// event loop
for (;;) {
	// wait for event
	__WFE();
	
	// USB power detected?
	if (gUSBDDetected) {
		gUSBDDetected = false;
		
		// enable USB on the device
		StartUSB();
		}
	
	// Endpoint 0 OUT SETUP?
	/* read or write transfer on USB Control Endpoint 0 */
	if (gUSBDEndpointOUT0SETUP) {
		gUSBDEndpointOUT0SETUP = false;

		USBSetup0();
		}
	
	// 
	if (gUSBDEndpointOUT1) {
		gUSBDEndpointOUT1 = false; // *** check whether this pattern has a race condition
		
		USBEndpointOUT1(*this);
		}
	
	// MAX key pressed?
	if (fMAX.WasKeyPressed())
		ProcessMAXKeyPress();
	
	// quadrature decoder report?
	if (fQDEC)
		ProcessQDEC();
	}
}
