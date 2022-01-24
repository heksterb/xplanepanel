/*

	LED
	
	LED interface
	
*/

#include <nrf_gpio.h>

#include "LED.h"





/*	ConfigureLEDs
	Configure GPIO for LEDs
*/
void ConfigureLEDs()
{
// configure GPIO for output
nrf_gpio_cfg(PIN_LED_RED, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
nrf_gpio_cfg(PIN_LED_BLUE, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
nrf_gpio_pin_clear(PIN_LED_RED);
nrf_gpio_pin_clear(PIN_LED_BLUE);
}
