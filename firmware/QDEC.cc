/*

	QDEC
	
	Quadrature Decoder interface
	
*/

#include <nrf_gpio.h>
#include <nrf_qdec.h>

#include "QDEC.h"



/*	gQDECReportReady
	QDEC report ready

NOTE
	There seems to be active discussion about whether 'volatile' is needed for std::atomic
*/
std::atomic<bool> QDEC::gReportReady;


/*	QDEC_IRQHandler
	This overrides a weak definition of a default interrupt handler in gcc_startup_nrf52840.S
	(if you remove this the project will still link)

NOTE
	Had to turn Link-Time Optimizations (LTO) off or this gets removed at link time; tried various methods
	of trying to force it to stay, to no avail.
*/
extern "C" void QDEC_IRQHandler()
{
if (nrf_qdec_event_check(NRF_QDEC_EVENT_REPORTRDY)) {
	nrf_qdec_event_clear(NRF_QDEC_EVENT_REPORTRDY);
	
	QDEC::gReportReady = true;
	}
}


/*	QDEC
	Configure quadrature decoder
*/
QDEC::QDEC(
	uint32_t	pinA,
	uint32_t	pinB
	)
{
// disable CPU interrupt and task interrupts
NVIC_DisableIRQ(QDEC_IRQn);
nrf_qdec_int_disable(~0);

// configure GPIO pins
/* "pins used by the QDEC must be configured in the GPIO before enabling the QDEC" */
nrf_gpio_cfg(pinA, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_SENSE_HIGH);
nrf_gpio_cfg(pinB, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_SENSE_HIGH);

// assign GPIO pins to decoder
nrf_qdec_pio_assign(pinA, pinB, NRF_QDEC_LED_NOT_CONNECTED /* LED not needed for mechanical encoder */);

// set sampling period
/* Say maximum detectable rotation speed is 1 full circle rotation per second.
   We have 24 detents per full circle; need to be able to sample the 90 degree phase changes,
   so 96 (say 100) samples per second, i.e. 10000 us period.  Realistically we rotate the knob
   much faster than that; also I'm not sure whether the debouncer requires a faster rate.
   Empirically determined 2048us to be adequate. */
nrf_qdec_sampleper_set(NRF_QDEC_SAMPLEPER_2048us);

// enable debouncer
nrf_qdec_dbfen_enable();

// set number of samples before a report event is considered
nrf_qdec_reportper_set(NRF_QDEC_REPORTPER_10);

nrf_qdec_enable();

// enable CPU interrupt and task interrupt
NVIC_SetPriority(QDEC_IRQn, 7 /* priority */);
NVIC_ClearPendingIRQ(QDEC_IRQn);
NVIC_EnableIRQ(QDEC_IRQn);
nrf_qdec_int_enable(NRF_QDEC_INT_REPORTRDY_MASK);

// start the rotary encoder QDEC decoder
nrf_qdec_task_trigger(NRF_QDEC_TASK_START);
}


/*	operator int32_t
	Read value
*/
QDEC::operator int32_t() const
{
// move ACC into ACCREAD, clear ACC
nrf_qdec_task_trigger(NRF_QDEC_TASK_READCLRACC);

// return ACCREAD
return nrf_qdec_accread_get();
}
