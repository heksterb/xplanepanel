/*
	USB

	USB interface

	GCC intrinsics:
		https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
	
	[DCDHID] Device Class Definition for Human Interface Devices v 1.11
	[HIDUT] HID Usage Tables for USB v1.22
	[nRFPS] nRF5240 Product Specification v1.2
	[USB] Universal Serial Bus Specification v2.0
*/

#include <inttypes.h>

#include <algorithm>

#include <nrf_clock.h>
#include <nrf_gpio.h>
#include <nrf_power.h>
#include <nrf_usbd.h>
#include <nrf52_erratas.h>

#include "Panel.h"
#include "USB.h"


using namespace USB;

#define PIN_LED_RED NRF_GPIO_PIN_MAP(1, 15)
#define PIN_LED_BLUE NRF_GPIO_PIN_MAP(1, 10)


/*	gUSBDDetected
	USB power detected (edge)
*/
volatile bool gUSBDDetected;


/*	gUSBPowerReady
	USB power ready (level)
*/
static volatile bool gUSBPowerReady;


/*	POWER_CLOCK_IRQHandler
	This overrides a weak definition of a default interrupt handler in gcc_startup_nrf52840.S
	(if you remove this the project will still link)
*/
extern "C" void POWER_CLOCK_IRQHandler()
{
/* Obviously all of this is sort of pointless since we're powered (indirectly) off USB anyway. */
if (nrf_power_event_check(NRF_POWER_EVENT_USBDETECTED)) {
	nrf_power_event_clear(NRF_POWER_EVENT_USBDETECTED);

	gUSBDDetected = true;
	}

if (nrf_power_event_check(NRF_POWER_EVENT_USBREMOVED)) {
	nrf_power_event_clear(NRF_POWER_EVENT_USBREMOVED);

	gUSBPowerReady = false;
	}

if (nrf_power_event_check(NRF_POWER_EVENT_USBPWRRDY)) {
	nrf_power_event_clear(NRF_POWER_EVENT_USBPWRRDY);

	gUSBPowerReady = true;
	}
}


/*	ConfigurePowerAndClock
	Configure power and clock management
*/
void ConfigurePowerAndClock()
{
// disable CPU interrupt and task interrupts
NVIC_DisableIRQ(POWER_CLOCK_IRQn);
nrf_power_int_disable(~0);
nrf_clock_int_disable(~0);

// enable CPU interrupt and task interrupt
NVIC_SetPriority(POWER_CLOCK_IRQn, 7 /* priority */);
NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
NVIC_EnableIRQ(POWER_CLOCK_IRQn);
nrf_power_int_enable(
	NRF_POWER_INT_USBDETECTED_MASK |
	NRF_POWER_INT_USBREMOVED_MASK |
	NRF_POWER_INT_USBPWRRDY_MASK
	);
}


/*	gUSBDEndpointOUT0SETUP

*/
volatile bool gUSBDEndpointOUT0SETUP;


/*	gUSBDEndpointOUT1

*/
volatile bool gUSBDEndpointOUT1;


/*	USBD_IRQHandler
	This overrides a weak definition of a default interrupt handler in gcc_startup_nrf52840.S
	(if you remove this the project will still link)
*/
extern "C" void USBD_IRQHandler()
{
if (nrf_usbd_event_check(NRF_USBD_EVENT_USBEVENT)) {
	nrf_usbd_event_clear(NRF_USBD_EVENT_USBEVENT);
	}

// Endpoint 0 SETUP?
if (nrf_usbd_event_check(NRF_USBD_EVENT_EP0SETUP)) {
	nrf_usbd_event_clear(NRF_USBD_EVENT_EP0SETUP);

	gUSBDEndpointOUT0SETUP = true;
	}

// USB Interrupt?
if (nrf_usbd_event_check(NRF_USBD_EVENT_DATAEP)) {
	nrf_usbd_event_clear(NRF_USBD_EVENT_DATAEP);
	
	// which endpoint?
	const uint32_t datastatus = nrf_usbd_epdatastatus_get();
	nrf_usbd_epdatastatus_clear(datastatus);
	if (datastatus & NRF_USBD_EPDATASTATUS_EPOUT1_MASK)
		gUSBDEndpointOUT1 = true;
	
	if (datastatus & NRF_USBD_EPDATASTATUS_EPIN1_MASK);
	}

// *** detect suspend/resume and limit power
}


// from <nrfx_usbd.c>
static inline void usbd_errata_187_211_begin()
{
if (*(volatile uint32_t*) 0x4006EC00 == 0x00000000) {
	*(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	*(volatile uint32_t*) 0x4006ED14 = 0x00000003;
	*(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	}

else
	*(volatile uint32_t*) 0x4006ED14 = 0x00000003;
}


static inline void usbd_errata_187_211_end()
{
if (*(volatile uint32_t*) 0x4006EC00 == 0x00000000) {
	*(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	*(volatile uint32_t*) 0x4006ED14 = 0x00000000;
	*(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	}

else
	*(volatile uint32_t*) 0x4006ED14 = 0x00000000;
}


static inline void usbd_errata_171_begin(void)
{
if (*(volatile uint32_t*) 0x4006EC00 == 0x00000000) {
        *(volatile uint32_t*) 0x4006EC00 = 0x00009375;
        *(volatile uint32_t*) 0x4006EC14 = 0x000000C0;
        *(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	}

else
	*(volatile uint32_t*) 0x4006EC14 = 0x000000C0;
}


static inline void usbd_errata_171_end(void)
{
if (*(volatile uint32_t*) 0x4006EC00 == 0x00000000) {
        *(volatile uint32_t*) 0x4006EC00 = 0x00009375;
        *(volatile uint32_t*) 0x4006EC14 = 0x00000000;
        *(volatile uint32_t*) 0x4006EC00 = 0x00009375;
	}

else
	*(volatile uint32_t*) 0x4006EC14 = 0x00000000;
}


/*	ConfigureUSBD
	Configure USB peripheral
*/
void ConfigureUSBD()
{
// disable CPU interrupt and task interrupts
NVIC_DisableIRQ(USBD_IRQn);
nrf_usbd_int_disable(~0);

// enable CPU interrupt and task interrupt
NVIC_SetPriority(USBD_IRQn, 7 /* priority */);
NVIC_ClearPendingIRQ(USBD_IRQn);
NVIC_EnableIRQ(USBD_IRQn);
nrf_usbd_int_enable(
	NRF_USBD_INT_USBEVENT_MASK |
	NRF_USBD_INT_EP0SETUP_MASK |
	NRF_USBD_INT_DATAEP_MASK
	);
}


/*	StartUSB
	Implements the "USBD Power-Up Sequence" [6.35.4]
*/
void StartUSB()
{
const bool
	erratum171 = nrf52_errata_171(),
	erratum187 = nrf52_errata_187(),
	erratum199 = nrf52_errata_199(),
	erratum223 = nrf52_errata_223();

if (erratum187) usbd_errata_187_211_begin();
if (erratum171) usbd_errata_171_begin();

// enable USBD
nrf_usbd_enable();

// start high-frequency clock
nrf_clock_task_trigger(NRF_CLOCK_TASK_HFCLKSTART);

/* probably should also be an event loop.  The triple busy-waiting loops are clearly wrong. */

// wait for USB peripheral ready
while (!(NRF_USBD_EVENTCAUSE_READY_MASK & nrf_usbd_eventcause_get()));
nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);
		
if (erratum171) usbd_errata_171_end();
if (erratum187) usbd_errata_187_211_end();

// wait for USB power ready
while (!gUSBPowerReady);

// wait for clock
while (!nrf_clock_event_check(NRF_CLOCK_EVENT_HFCLKSTARTED));
nrf_clock_event_clear(NRF_CLOCK_EVENT_HFCLKSTARTED);

// enable Endpoint 0 IN and OUT
nrf_usbd_ep_enable(NRF_USBD_EPIN(0));
nrf_usbd_ep_enable(NRF_USBD_EPOUT(0));

// signal presence of device to host
nrf_usbd_pullup_enable();
}


/*	Send
	Send USB data
*/
static void Send(
	const void	*const data,
	uint16_t	length
	)
{
nrf_usbd_ep_easydma_set(NRF_USBD_EPIN(0), reinterpret_cast<uintptr_t>(data), length);
nrf_usbd_task_trigger(NRF_USBD_TASK_STARTEPIN0);
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_STARTED));
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_ENDEPIN0));
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_EP0DATADONE)); // ***** completely possible for it to get stuck here
nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}


/*	Send
	Send descriptor
*/
void Descriptor::Send() const
{
::Send(this, std::min<uint16_t>(length, nrf_usbd_setup_wlength_get()));
}


/*	gReportDescriptor
	HID report descriptor for the panel
*/
static constexpr struct __attribute__((packed)) {
	using TagMain = HIDReportDescriptorItemPrefix::TagMain;
	using TagGlobal = HIDReportDescriptorItemPrefix::TagGlobal;
	using TagLocal = HIDReportDescriptorItemPrefix::TagLocal;
	
	// [USBHID] Usage 0x01 to 0x20 are for top-level collections
	/* I don't see an alternative to using a 'vendor' page: there doesn't seem to be any 'LC' (linear control) that we can use. */
	HIDReportDescriptorItem<TagGlobal, uint16_t> usagePage { HIDReportDescriptorItemPrefix::kUsageGlobal, 0xffa0 };
	HIDReportDescriptorItem<TagLocal, uint8_t> usage { HIDReportDescriptorItemPrefix::kUsageLocal, 0x01 };
	HIDReportDescriptorItem<TagMain, uint8_t> beginCollectionApplication { HIDReportDescriptorItemPrefix::kCollection, 0x01 /* application */ };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageCollection { HIDReportDescriptorItemPrefix::kUsageLocal, 0x20 };
	HIDReportDescriptorItem<TagMain, uint8_t> beginCollectionPhysical { HIDReportDescriptorItemPrefix::kCollection, 0x00 /* physical */ };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageInput { HIDReportDescriptorItemPrefix::kUsageLocal, 0x21 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMinimumInput { HIDReportDescriptorItemPrefix::kLogicalMinimum, 0 };
	HIDReportDescriptorItem<TagGlobal, uint32_t> logicalMaximumInput { HIDReportDescriptorItemPrefix::kLogicalMaximum, 999999 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportCountInput { HIDReportDescriptorItemPrefix::kReportCount, 2 /* displays */ };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportSizeInput { HIDReportDescriptorItemPrefix::kReportSize, 20 /* bits */ };
	HIDReportDescriptorItem<TagMain, uint8_t> input { HIDReportDescriptorItemPrefix::kInput, 0b10100010 };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageOutput { HIDReportDescriptorItemPrefix::kUsageLocal, 0x22 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMinimumOutput { HIDReportDescriptorItemPrefix::kLogicalMinimum, 0 };
	HIDReportDescriptorItem<TagGlobal, uint32_t> logicalMaximumOutput { HIDReportDescriptorItemPrefix::kLogicalMaximum, 999999 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportCountOutput { HIDReportDescriptorItemPrefix::kReportCount, 2 /* displays */ };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportSizeOutput { HIDReportDescriptorItemPrefix::kReportSize, 20 /* bits */ };
	HIDReportDescriptorItem<TagMain, uint8_t> output { HIDReportDescriptorItemPrefix::kOutput, 0b10100010 };
	HIDReportDescriptorItem<TagMain, void> endCollectionPhysical { HIDReportDescriptorItemPrefix::kCollectionEnd };
	HIDReportDescriptorItem<TagMain, void> endCollectionApplication { HIDReportDescriptorItemPrefix::kCollectionEnd };
	} gReportDescriptor;


#if 0
/*	gReportDescriptor
	HID report descriptor for the panel
*/
static constexpr struct __attribute__((packed)) {
	using TagMain = HIDReportDescriptorItemPrefix::TagMain;
	using TagGlobal = HIDReportDescriptorItemPrefix::TagGlobal;
	using TagLocal = HIDReportDescriptorItemPrefix::TagLocal;
	
	// [USBHID] Usage 0x01 to 0x20 are for top-level collections
	/* I don't see an alternative to using a 'vendor' page: there doesn't seem to be any 'LC' (linear control) that we can use. */
	HIDReportDescriptorItem<TagGlobal, uint16_t> usagePage { HIDReportDescriptorItemPrefix::kUsageGlobal, 0xffa0 };
	HIDReportDescriptorItem<TagLocal, uint8_t> usage { HIDReportDescriptorItemPrefix::kUsageLocal, 0x01 };
	HIDReportDescriptorItem<TagMain, uint8_t> beginCollectionApplication { HIDReportDescriptorItemPrefix::kCollection, 0x01 /* application */ };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageCollection { HIDReportDescriptorItemPrefix::kUsageLocal, 0x20 };
	HIDReportDescriptorItem<TagMain, uint8_t> beginCollectionPhysical { HIDReportDescriptorItemPrefix::kCollection, 0x00 /* physical */ };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageInput { HIDReportDescriptorItemPrefix::kUsageLocal, 0x21 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMinimumInput { HIDReportDescriptorItemPrefix::kLogicalMinimum, 0 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMaximumInput { HIDReportDescriptorItemPrefix::kLogicalMaximum, 9 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportCountInput { HIDReportDescriptorItemPrefix::kReportCount, 6 /* digits */ };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportSizeInput { HIDReportDescriptorItemPrefix::kReportSize, 4 /* bits */ };
	HIDReportDescriptorItem<TagMain, uint8_t> input { HIDReportDescriptorItemPrefix::kInput, 0b10100010 };
	HIDReportDescriptorItem<TagLocal, uint8_t> usageOutput { HIDReportDescriptorItemPrefix::kUsageLocal, 0x22 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMinimumOutput { HIDReportDescriptorItemPrefix::kLogicalMinimum, 0 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> logicalMaximumOutput { HIDReportDescriptorItemPrefix::kLogicalMaximum, 9 };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportCountOutput { HIDReportDescriptorItemPrefix::kReportCount, 6 /* digits */ };
	HIDReportDescriptorItem<TagGlobal, uint8_t> reportSizeOutput { HIDReportDescriptorItemPrefix::kReportSize, 4 /* bits */ };
	HIDReportDescriptorItem<TagMain, uint8_t> output { HIDReportDescriptorItemPrefix::kOutput, 0b10100010 };
	HIDReportDescriptorItem<TagMain, void> endCollectionPhysical { HIDReportDescriptorItemPrefix::kCollectionEnd };
	HIDReportDescriptorItem<TagMain, void> endCollectionApplication { HIDReportDescriptorItemPrefix::kCollectionEnd };
	} gReportDescriptor;
#endif


/*	gConfigurationDescriptor
	'Extended' configuration descriptor
	
	[USB §9.4.3]
		A request for a configuration descriptor returns the configuration descriptor, all interface
		descriptors, and endpoint descriptors for all of the interfaces in a single request.  The first
		interface descriptor follows the configuration descriptor.  The endpoint descriptors for
		the first interface follow the first interface descriptor.  If there are additional interfaces,
		their interface descriptor and endpoint descriptors follow the first interface’ s endpoint descriptors.
		Class-specific and/or vendor-specific descriptors follow the standard descriptors they extend or modify.
	*/
static constexpr struct __attribute__((packed)) {
	USB::ConfigurationDescriptor configuration;
	USB::InterfaceDescriptor interface;
	USB::HIDClassDescriptor<1> hid;
	USB::EndpointDescriptor endpoints[2];
	} gConfigurationDescriptor = {
	/* configuration */ {
		sizeof gConfigurationDescriptor,
		1, // number of interfaces
		1, // configuration value
		0, // no string descriptor
		false, // no remote wake-up
		false, // not self-powered
		40 / 2 // maximum power (in 2mA units)
		},
	
	/* interface */ {
		0, // index
		0, // alternate setting
		2, // number of endpoints
		USB::InterfaceClass::kHID,
		0x00, // no subclass
		0x00, // no protocol
		0 // no string descriptor
		},

	/* HID class */ {
		0x0110, // 01.10
		0, // no localization
		{ USB::DescriptorType::kHIDReport, sizeof gReportDescriptor }
		},

	/* endpoints */ {
		/* [0] */ {
			1, // endpoint number
			USB::EndpointDescriptor::kOUT,
			USB::EndpointDescriptor::kInterrupt,
			USB::EndpointDescriptor::kNoSynchronization, USB::EndpointDescriptor::kData, // not an isochronous endpoint
			2 * 4, /* maximum packet size [nRFPS §6.35.10] multiple of 4 *** */
			10 /* polling interval *** */
			},

		/* [1] */ {
			1, // endpoint number
			USB::EndpointDescriptor::kIN,
			USB::EndpointDescriptor::kInterrupt,
			USB::EndpointDescriptor::kNoSynchronization, USB::EndpointDescriptor::kData, // not an isochronous endpoint
			2 * 4, /* maximum packet size [nRFPS §6.35.10] multiple of 4 *** */
			10 /* polling interval *** */
			}
		}
	};


/*	gDeviceDescriptor

*/
static constexpr DeviceDescriptor gDeviceDescriptor(
	0x00, // [DCDHID §5.1] class type is not defined at the device descriptor but at the interface descriptor
	0x00, // subclass (only for boot interface devices)
	0x00, // protocol (only for boot interface devices) 
	64, // maximum packet size for Endpoint 0
	0xF055, // vendor ID *** (pseudo-officially like "FOSS")
	0x1234, // product ID ***
	0x0001, // device version 00.01
	1, // manufacturer descriptor
	2, // product descriptor
	0, // no serial number descriptor
	1 // number of configurations ***
	);


static constexpr StringDescriptor0<1> gStringDescriptor0({ 0x0409 /* English */ });
static constexpr StringDescriptor<11> gStringManufacturer( std::array<char16_t, 12> { u"Ben Hekster" });
static constexpr StringDescriptor<23> gStringProduct( std::array<char16_t, 24> { u"Simulator Display Panel" });


/*	USBGetDeviceDescriptor

*/
static bool USBGetDeviceDescriptor(
	const uint8_t
	)
{
const auto dd = gDeviceDescriptor;
dd.Send();

return true;
}


/*	USBGetConfigurationDescriptor

*/
static bool USBGetConfigurationDescriptor(
	const uint8_t	index
	)
{
bool handled = false;

if (index == 0) {
	const auto cd = gConfigurationDescriptor;
	::Send(&cd, std::min<unsigned>(sizeof cd, nrf_usbd_setup_wlength_get()));
	
	handled = true;
	}

return handled;
}


/*	USBGetStringDescriptor

*/
static bool USBGetStringDescriptor(
	const uint8_t	index
	)
{
bool handled = false;

switch (index) {
	case 0:		{ const auto sd = gStringDescriptor0; sd.Send(); handled = true; break; }
	case 1:		{ const auto sm = gStringManufacturer; sm.Send(); handled = true; break; }
	case 2:		{ const auto sp = gStringProduct; sp.Send(); handled = true; break; }
	}

return handled;
}


static bool USBStallMe(
	const uint8_t
	)
{
nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);

return true;
}


/*	USBDeviceGetDescriptor
	Get USB device descriptor (i.e., the descriptor *belonging to* a device; not necessarily the descriptor *describing* the device)
*/
static bool USBDeviceGetDescriptor(
	DescriptorType	type,
	uint8_t		index
	)
{
bool (*f)(uint8_t index) = nullptr;

// dispatch on descriptor type
switch (type) {
	case DescriptorType::kDevice: f = USBGetDeviceDescriptor; break;
	case DescriptorType::kConfiguration: f = USBGetConfigurationDescriptor; break;
	case DescriptorType::kString: f = USBGetStringDescriptor; break;
	case DescriptorType::kDeviceQualifier: f = USBStallMe; break; // don't support
	case DescriptorType::kHIDReport:
		f = USBStallMe;
		/* This seems to be sent in error by the USBTreeView utility as a _device_ instead of a _class_ request. */
		break;
	}

return f ? (*f)(index) : false;
}


/*	USBDeviceSetConfiguration

*/
static bool USBDeviceSetConfiguration()
{
// check configuration is valid
if (const uint16_t value = nrf_usbd_setup_wvalue_get(); (value & 0xff) != 1)
	return false;

// transaction complete
nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);

// enable interrupt Endpoint 1 IN and OUT
nrf_usbd_ep_enable(NRF_USBD_EPIN(1));
nrf_usbd_ep_enable(NRF_USBD_EPOUT(1));

// prepare OUT buffer (host to device)
nrf_usbd_epout_clear(NRF_USBD_EPOUT(1));

return true;
}


static void USBEndpointClearFeature(
	const uint16_t	value
	)
{
// nothing to do
nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}


/*	USBHIDGetReportDescriptor
	[DCDHID] index should be zero
*/
static void USBHIDGetReportDescriptor(
	uint8_t
	)
{
const auto rd = gReportDescriptor;

::Send(&rd, std::min<unsigned>(sizeof rd, nrf_usbd_setup_wlength_get()));
}


/*	USBHIDGetDescriptor
	[DCDHID §7.1.1]
*/
static bool USBHIDGetDescriptor(
	DescriptorType	type,
	uint8_t		index
	)
{
void (*f)(uint8_t index) = nullptr;

// dispatch on descriptor type
switch (type) {
	case DescriptorType::kHIDReport: f = USBHIDGetReportDescriptor;	break;
	}

if (f) (*f)(index);

return f;
}


/*	USBSetAddress

*/
static bool USBSetAddress(
	const RequestType::Recipient
	)
{
// [nRFPS §6.35.9] "the software shall not process this command"
return true;
}


/*	USBGetDescriptor

*/
static bool USBGetDescriptor(
	const RequestType::Recipient recipient
	)
{
const union __attribute__((packed)) {
	uint16_t	i;
	struct {
		uint8_t		index;
		DescriptorType	type;
		};
	} value = { nrf_usbd_setup_wvalue_get() };

bool (*f)(DescriptorType, uint8_t index) = nullptr;

switch (recipient) {
	case RequestType::kDevice: f = USBDeviceGetDescriptor; break;
	case RequestType::kInterface: f = USBHIDGetDescriptor; break;
	}

return f ? (*f)(value.type, value.index) : false;
}


/*	USBSetConfiguration

*/
static bool USBSetConfiguration(
	const RequestType::Recipient recipient
	)
{
bool handled = false;

bool (*f)() = nullptr;

switch (recipient) {
	case RequestType::kDevice: f = USBDeviceSetConfiguration; break;
	}

return f ? (*f)() : false;
}


/*	USBClearFeature

*/
static bool USBClearFeature(
	const RequestType::Recipient recipient
	)
{
bool handled = false;

const uint16_t feature = nrf_usbd_setup_wvalue_get();

switch (recipient) {
	case RequestType::kEndpoint: USBEndpointClearFeature(feature); handled = true; break;
	}

return handled;
}


/*	USBStandard
	[USB §9.3]
*/
static bool USBStandard(
	const RequestType::Direction direction,
	const RequestType::Recipient recipient
	)
{
bool (*f)(const RequestType::Recipient) = nullptr;

// dispatch on request
if (direction == RequestType::Direction::kHostToDevice)
	switch (const SetupRequest request = static_cast<SetupRequest>(nrf_usbd_setup_brequest_get())) {
		case SetupRequest::kSetAddress: f = USBSetAddress; break;
		case SetupRequest::kSetConfiguration: f = USBSetConfiguration; break;
		case SetupRequest::kClearFeature: f = USBClearFeature; break;
		}

else
	switch (const SetupRequest request = static_cast<SetupRequest>(nrf_usbd_setup_brequest_get())) {
		case SetupRequest::kGetDescriptor: f = USBGetDescriptor; break;
		}

return f ? (*f)(recipient) : false;
}


/*	USBHIDSetIdle

*/
static bool USBHIDSetIdle(
	const RequestType::Recipient recipient
	)
{
bool handled = false;

union __attribute__((packed)) {
	uint16_t	i;
	struct {
		uint8_t		duration, reportID /* ***** order? */;
		};
	} value = { nrf_usbd_setup_wvalue_get() };

switch (recipient) {
	case RequestType::kClass:
		nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
		handled = true;
		break;
	}

return handled;
}


/*	USBClass

*/
static bool USBClass(
	const RequestType::Direction direction,
	const RequestType::Recipient recipient
	)
{
bool (*f)(const RequestType::Recipient) = nullptr;

// dispatch on request
if (direction == RequestType::Direction::kHostToDevice)
	switch (const ClassSetupRequest request = static_cast<ClassSetupRequest>(nrf_usbd_setup_brequest_get())) {
		case ClassSetupRequest::kSetIdle:
			f = USBHIDSetIdle;
			break;
		}

else
	;

return f ? (*f)(recipient) : false;
}


/*	USBSetup0
	Setup stage in Endpoint 0
*/
void USBSetup0()
{
const RequestType requestType { nrf_usbd_setup_bmrequesttype_get() };

// dispatch on request type
bool (*f)(RequestType::Direction, RequestType::Recipient) = nullptr;

switch (requestType.type) {
	case RequestType::kStandard:	f = USBStandard; break;
	case RequestType::kClass:	f = USBClass; break;
	}

const bool handled = f ? (*f)(requestType.direction, requestType.recipient) : false;

if (!handled) {
	nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);
	nrf_gpio_pin_set(PIN_LED_RED);
	}
}


union Report {
	char		i[5];
	struct {
		unsigned long long
				v0 : 20,
				v1 : 20;
		};
	};

#if 0
union {
	char		i[3];
	struct {
		unsigned	digit0 : 4,
				digit1 : 4,
				digit2 : 4,
				digit3 : 4,
				digit4 : 4,
				digit5 : 4;
		};
	} reportOut;
#endif


/*	USBEndpointOUT1

*/
void USBEndpointOUT1(
	Panel		&panel
	)
{
// prepare buffer to receive OUT DATA
Report reportOut;
nrf_usbd_ep_easydma_set(NRF_USBD_EPOUT(1), reinterpret_cast<uintptr_t>(&reportOut), sizeof reportOut);
nrf_usbd_task_trigger(NRF_USBD_TASK_STARTEPOUT1);
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_STARTED));
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_ENDEPOUT1));

panel.SetValue(reportOut.v0, reportOut.v1);
}


/*	USBEndpointIN1
	Send the given values as the current state
*/
void USBEndpointIN1(
	unsigned	value0,
	unsigned	value1
	)
{
Report report;
report.v0 = value0;
report.v1 = value1;

nrf_usbd_ep_easydma_set(NRF_USBD_EPIN(1), reinterpret_cast<uintptr_t>(&report), sizeof report);
nrf_usbd_task_trigger(NRF_USBD_TASK_STARTEPIN1);
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_STARTED));
while (!nrf_usbd_event_get_and_clear(NRF_USBD_EVENT_ENDEPIN1));
}
