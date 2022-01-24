/*

	LED
	
	LED interface
	
*/

#pragma once


#define PIN_LED_RED NRF_GPIO_PIN_MAP(1, 15)
#define PIN_LED_BLUE NRF_GPIO_PIN_MAP(1, 10)


/*	LED
	On-board LEDs
*/
extern void ConfigureLEDs();
