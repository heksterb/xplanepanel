/*

	SPIM
	
	SPI Master interface
	
*/

#include <nrf_gpio.h>
#include <nrf_spim.h>

#include "SPIM.h"



/*	gEnd
	SPI Master transaction end
*/
volatile bool SPIM::gEnd;


/*	SPIM3_IRQHandler
	This overrides a weak definition of a default interrupt handler in gcc_startup_nrf52840.S
	(if you remove this the project will still link)
*/
extern "C" void SPIM3_IRQHandler()
{
if (nrf_spim_event_check(NRF_SPIM3, NRF_SPIM_EVENT_END)) {
	// must clear the event, or the interrupt keeps recurring
	nrf_spim_event_clear(NRF_SPIM3, NRF_SPIM_EVENT_END);
	SPIM::gEnd = true;
	}
}


/*	SPIM
	SPI Master configuration
*/
SPIM::SPIM(
	uint32_t	pinCS,
	uint32_t	pinClock,
	uint32_t	pinMOSI,
	uint32_t	pinMISO
	)
{
// disable CPU interrupt and task interrupts
NVIC_DisableIRQ(SPIM3_IRQn);
nrf_spim_int_disable(NRF_SPIM3, ~0);

// configure GPIO pin for SPI clock
nrf_gpio_pin_clear(pinClock); // clock is active high, so drive low
nrf_gpio_cfg_output(pinClock);

// configure GPIO pin for SPI data out
nrf_gpio_cfg_output(pinMOSI);

// configure GPIO pin for SPI data in
// "DOUT on the MAX6954 is never high impedance"
nrf_gpio_cfg_input(pinMISO, NRF_GPIO_PIN_NOPULL);

// configure GPIO pin for CS (chip select)
nrf_gpio_pin_set(pinCS); // set high 
nrf_gpio_cfg_output(pinCS);

// configure SPI
nrf_spim_pins_set(NRF_SPIM3, pinClock, pinMOSI, pinMISO);
nrf_spim_frequency_set(NRF_SPIM3, NRF_SPIM_FREQ_1M);

/* SPI mode 0 (clock active low, sample on rising edge); data MSB first */
nrf_spim_configure(NRF_SPIM3, NRF_SPIM_MODE_0, NRF_SPIM_BIT_ORDER_MSB_FIRST);

// configure hardware CS (only in SPIM instance 3)
/* Nordic: The value is specified in number of 64 MHz clock cycles (15.625 ns).
   Maxim: tCSW = 19.5 ns*/
nrf_spim_csn_configure(NRF_SPIM3, pinCS, NRF_SPIM_CSN_POL_LOW, 2 /* duration */);

nrf_spim_tx_list_disable(NRF_SPIM3);
nrf_spim_rx_list_disable(NRF_SPIM3);

/* "PSEL can only be configured while SPIM is disabled" */
/* "Pins used by SPIM must be configured in GPIO before SPIM is enabled" */
nrf_spim_enable(NRF_SPIM3);

// enable CPU interrupt and task interrupt
NVIC_SetPriority(SPIM3_IRQn, 7 /* priority */);
NVIC_ClearPendingIRQ(SPIM3_IRQn);
NVIC_EnableIRQ(SPIM3_IRQn);
nrf_spim_int_enable(NRF_SPIM3, NRF_SPIM_INT_END_MASK);
}


/*	()
	Send a 16-bit word through the SPI interface
*/
uint16_t SPIM::operator()(
	const uint16_t	out
	) const
{
// ARM is little-endian; SPI has most significant bits and bytes first
uint16_t spimOut = __builtin_bswap16(out);
uint16_t spimIn;

// set up transmit and receive buffers for SPI transaction
nrf_spim_tx_buffer_set(NRF_SPIM3, (unsigned char*) &spimOut, sizeof spimOut);
nrf_spim_rx_buffer_set(NRF_SPIM3, (unsigned char*) &spimIn, sizeof spimIn);

// start SPI master transaction
/* Don't need atomic here because we are triggering the task synchronously. */
nrf_spim_event_clear(NRF_SPIM3, NRF_SPIM_EVENT_END);
gEnd = false;
nrf_spim_task_trigger(NRF_SPIM3, NRF_SPIM_TASK_START);
while (!gEnd) __WFE();

return __builtin_bswap16(spimIn);
}
