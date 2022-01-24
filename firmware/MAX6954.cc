/*

	MAX6954
	
	Maxim 6954 LED Display Driver interface
	
*/

#include <nrf_gpio.h>
#include <nrf_gpiote.h>

#include "MAX6954.h"


#define PIN_KEY_INTERRUPT NRF_GPIO_PIN_MAP(0, 26)


/*	gKey
	Key pressed
*/
std::atomic<bool> MAX6954::gKey;


/*	GPIOTE_IRQHandler
	This overrides a weak definition of a default interrupt handler in gcc_startup_nrf52840.S
	(if you remove this the project will still link)
*/
extern "C" void GPIOTE_IRQHandler()
{
// key-pressed event?
if (nrf_gpiote_in_event_get(0 /* channel */)) {
	// must clear the event, or the interrupt keeps recurring
	nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_0);
	
	MAX6954::gKey = true;
	}
}


/*	MAX6954

*/
MAX6954::MAX6954(
	SPIM		&spim
	) :
	fSPIM(spim)
{
// disable CPU interrupt and task interrupts
NVIC_DisableIRQ(GPIOTE_IRQn);
nrf_gpiote_int_disable(~0 /* NRF_SPIM_ALL_INTS_MASK */);

// configure GPIO pin for input from MAX IRQ
/* Seems redundant [nRF52840 6.10.3] but needed for the pull-up */
/* Don't see the need for pull-up documented anywhere in [MAX6954], but it's reasonable (and P4 is always low otherwise). */
nrf_gpio_cfg(PIN_KEY_INTERRUPT, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1 /* dummy */, NRF_GPIO_PIN_NOSENSE);

// configure GPIOTE Channel 0 as an event on a transition of this pin
nrf_gpiote_event_configure(0 /* channel */, PIN_KEY_INTERRUPT, NRF_GPIOTE_POLARITY_HITOLO);
nrf_gpiote_event_enable(0 /* channel */);

// enable CPU interrupt and task interrupt
NVIC_SetPriority(GPIOTE_IRQn, 7 /* priority */);
NVIC_ClearPendingIRQ(GPIOTE_IRQn);
NVIC_EnableIRQ(GPIOTE_IRQn);
nrf_gpiote_int_enable(NRF_GPIOTE_INT_IN0_MASK);
}


/*	DecodeMode

*/
void MAX6954::DecodeMode(
	const uint8_t	mode
	)
{
(void) fSPIM(Message {{ .data = mode, .registre = kRegisterDecodeMode }});
}


/*	GlobalIntensity

*/
void MAX6954::GlobalIntensity(
	const uint8_t	intensity
	)
{
(void) fSPIM(Message {{ .data = intensity, .registre = kRegisterGlobalIntensity }});
}


/*	ScanLimit

*/
void MAX6954::ScanLimit(
	const uint8_t	limit
	)
{
(void) fSPIM(Message {{ .data = limit, .registre = kRegisterScanLimit }});
}


/*	Configure

*/
void MAX6954::Configure(
	const Configuration configuration
	)
{
(void) fSPIM(Message {{ .data = configuration.i, .registre = kRegisterConfiguration }});
}

MAX6954::Configuration MAX6954::Configure()
{
// issue read request
(void) fSPIM(Message {{ .registre = kRegisterConfiguration, .read = true }});

// issue dummy request to retrieve response
Message result = { .i = fSPIM(Message {{ .registre = kRegisterNoOperation, .read = true }}) };

return { .i = static_cast<uint8_t>(result.data) };
}


/*	PortConfigure

*/
void MAX6954::PortConfigure(
	uint8_t		configuration
	)
{
(void) fSPIM(Message {{ .data = configuration, .registre = kRegisterPortConfiguration }});
}


/*	KeyMask

*/
void MAX6954::KeyMask(
	unsigned char	keys,
	uint8_t		value
	)
{
// configure key scan interrupt mask
(void) fSPIM(Message {{ .data = value, .registre = static_cast<uint8_t>(kRegisterKeyAMaskDebounce + keys) }});
}


/*	DebouncedKey

*/
uint8_t MAX6954::DebouncedKey(
	unsigned char	keys
	)
{
// issue read request
(void) fSPIM(Message {{ .registre = static_cast<uint8_t>(kRegisterKeyAMaskDebounce + keys), .read = true }});

// issue dummy request to retrieve response
Message result = { .i = fSPIM(Message {{ .registre = kRegisterNoOperation, .read = true }}) };

return { static_cast<uint8_t>(result.data) };
}


/*	DigitType

*/
void MAX6954::DigitType(
	const uint8_t	type
	)
{
(void) fSPIM(Message {{ .data = type, .registre = kRegisterDigitTypeKeyAPressed }});
}


/*	KeyPressed

*/
uint8_t MAX6954::KeyPressed(
	unsigned char	keys
	)
{
// issue read request
(void) fSPIM(Message {{ .registre = static_cast<uint8_t>(kRegisterDigitTypeKeyAPressed + keys), .read = true }});

// issue dummy request to retrieve response
Message result = { .i = fSPIM(Message {{ .registre = kRegisterNoOperation, .read = true }}) };

return { static_cast<uint8_t>(result.data) };
}


/*	Digit

*/
void MAX6954::Digit(
	unsigned char	digit,
	const uint8_t	value
	)
{
Message data = { 0 };
data.registre = 0x20 + digit;
data.data = value;
(void) fSPIM(data.i);
}

uint8_t MAX6954::Digit(
	unsigned char	digit
	)
{
// issue read request
(void) fSPIM(Message {{ .registre = static_cast<uint8_t>(0x20 + digit), .read = true }});

// issue dummy request to retrieve response
Message result = { .i = fSPIM(Message {{ .registre = kRegisterNoOperation, .read = true }}) };

return { static_cast<uint8_t>(result.data) };
}
