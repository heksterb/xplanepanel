/*

	USB

	USB interface

*/

#pragma once

#include <array>


extern volatile bool gUSBDDetected;
extern volatile bool gUSBDEndpointOUT0SETUP;
extern volatile bool gUSBDEndpointOUT1;


extern void USBEndpointOUT1(Panel&);
extern void USBEndpointIN1(unsigned value0, unsigned value1);
extern void StartUSB();
extern void USBSetup0();



namespace USB {
	/*	RequestType
		Control transfer request type
	*/
	union __attribute__((packed)) RequestType {
		enum Recipient { kDevice, kInterface, kEndpoint, kOther };
		enum Type { kStandard, kClass, kVendor };
		enum Direction { kHostToDevice, kDeviceToHost };
	
		uint8_t		i;
		struct __attribute__((packed)) {
			Recipient	recipient : 5;
			Type		type : 2;
			Direction	direction : 1;
			};
		};
	static_assert(sizeof(RequestType) == 1, "unexpected size of USB Request Type");


	/*	SetupRequest
		Standard requests
		[USB Table 9-4]
	*/
	enum class SetupRequest : uint8_t {
		kGetStatus,
		kClearFeature,
		kReserved0,
		kSetFeature,
		kReserved1,
		kSetAddress,
		kGetDescriptor,
		kSetDescriptor,
		kGetConfiguration,
		kSetConfiguration,
		kGetInterface,
		kSetInterface,
		kSyncFrame
		};
	

	/*	ClassSetupRequest
	
	*/
	enum class ClassSetupRequest : uint8_t {
		kGetReport = 1,
		kGetIdle,
		kGetProtocol,
		kSetReport = 9,
		kSetIdle,
		kSetProtocol
		};
	
	
	/*	InterfaceClass
	
	*/
	enum class InterfaceClass : uint8_t {
		kReserved,
		kAudio,
		kCommunicationsCommunications,
		kHID,
		kPhysicial,
		kImage,
		kPrinter,
		kMassStorage,
		kHub,
		kCommunicationsData,
		kSmartCard,
		kContentSecurity,
		kVideo,
		kPersonalHealthcare,
		kDiagnostic = 0xDC,
		kWireless = 0xE0,
		kMiscellaneous = 0xEF,
		kAppplication = 0xFE,
		kVendor = 0xFF
		};
	
	
	/*	DescriptorType
	
		[USB Table 9-5]
	*/
	enum class DescriptorType : uint8_t {
		kNone,
		kDevice,
		kConfiguration,
		kString,
		kInterface,
		kEndpoint,
		kDeviceQualifier,
		kOtherSpeedConfiguration,
		kInterfacePower,
		
		/* Device Class Definition for Human Interface Devices 1.11: §7.1 */
		kHID = 0x21,
		kHIDReport,
		kHIDPhysical
		};
	
	
	/*	Descriptor
		Base class of all USB descriptors
	*/
	struct __attribute__((packed)) Descriptor {
		uint8_t		length;
		DescriptorType type;
		
		constexpr	Descriptor(
					const uint8_t length,
					DescriptorType type
					) :
					length(length),
					type(type)
					{}
		
		void		Send() const;
		};
	
	
	/*	DeviceDescriptor
	
	*/
	struct __attribute__((packed)) DeviceDescriptor : public Descriptor {
		uint16_t	usb;
		uint8_t		klass,
				subClass;
		uint8_t		protocol;
		uint8_t		maxPacketSize0;
		uint16_t	vendorID,
				productID;
		uint16_t	device;
		uint8_t		manufacturerI,
				productI,
				serialNumberI;
		uint8_t		configurationsN;
	
		constexpr	DeviceDescriptor(
					uint8_t		klass,
					uint8_t		subClass,
					uint8_t		protocol,
					uint8_t		maxPacketSize0,
					uint16_t	vendorID,
					uint16_t	productID,
					uint16_t	device,
					uint8_t		manufacturerI,
					uint8_t		productI,
					uint8_t		serialNumberI,
					uint8_t		configurationsN
					) :
					Descriptor(sizeof *this, DescriptorType::kDevice),
					usb(0x0200), // USB version 02.00
					klass(klass),
					subClass(subClass),
					protocol(protocol),
					maxPacketSize0(maxPacketSize0),
					vendorID(vendorID),
					productID(productID),
					device(device),
					manufacturerI(manufacturerI),
					productI(productI),
					serialNumberI(serialNumberI),
					configurationsN(configurationsN)
					{}
		};
	static_assert(sizeof(DeviceDescriptor) == 18, "unexpected size of USB device descriptor");


	/*	ConfigurationDescriptor

	*/
	struct __attribute__((packed)) ConfigurationDescriptor : public Descriptor {
		uint16_t	totalLength;
		uint8_t		interfacesN;
		uint8_t		configurationValue;
		uint8_t		configurationI;
		uint8_t		reserved0 : 5,
				remoteWakeup : 1,
				selfPowered : 1,
				reserved1 : 1;
		uint8_t		maxPower;
	
	
		constexpr	ConfigurationDescriptor(
					uint16_t	totalLength,
					uint8_t		interfacesN,
					uint8_t		configurationValue,
					uint8_t		configurationI,
					bool		remoteWakeup,
					bool		selfPowered,
					uint8_t		maxPower
					) :
					Descriptor(sizeof *this, DescriptorType::kConfiguration),
					totalLength(totalLength),
					interfacesN(interfacesN),
					configurationValue(configurationValue),
					configurationI(configurationI),
					reserved0(0 /* [USBC p104] must be zero */),
					remoteWakeup(remoteWakeup),
					selfPowered(selfPowered),
					reserved1(1 /* [USBC p104] must be 1 */),
					maxPower(maxPower)
					{}
		};
	static_assert(sizeof(ConfigurationDescriptor) == 9, "unexpected size of USB configuration descriptor");


	/*	InterfaceDescriptor

	*/
	struct __attribute__((packed)) InterfaceDescriptor : public Descriptor {
		uint8_t		number;
		uint8_t		alternateSetting;
		uint8_t		endpointsN;
		InterfaceClass	klass;
		uint8_t		subclass;
		uint8_t		protocol;
		uint8_t		stringI;
	
	
		constexpr	InterfaceDescriptor(
					uint8_t		number,
					uint8_t		alternateSetting,
					uint8_t		endpointsN,
					InterfaceClass	klass,
					uint8_t		subclass,
					uint8_t		protocol,
					uint8_t		stringI
					) :
					Descriptor(sizeof *this, DescriptorType::kInterface),
					number(number),
					alternateSetting(alternateSetting),
					endpointsN(endpointsN),
					klass(klass),
					subclass(subclass),
					protocol(protocol),
					stringI(stringI)
					{}
		};
	static_assert(sizeof(InterfaceDescriptor) == 9, "unexpected size of USB interface descriptor");
	
	
	/*	EndpointDescriptor
	
	*/
	struct __attribute__((packed)) EndpointDescriptor : public Descriptor {
		enum Direction { kOUT, kIN };
		enum Transfer { kControl, kIsochronous, kBulk, kInterrupt };
		enum Synchronization { kNoSynchronization, kAsynchronous, kAdaptive, kSynchronous };
		enum Usage { kData, kFeedback, kImplicitFeedback, kReserved };
	
		unsigned	number : 4;
		unsigned	reserved0 : 3;
		Direction	direction : 1;
		Transfer	transfer : 2;
		Synchronization	synchronization : 2;
		Usage		usage : 2;
		unsigned	reserved1 : 2;
	
		uint16_t	maxPacketSize;
		uint8_t		interval;
	
		constexpr	EndpointDescriptor(
					uint8_t		number,
					Direction	direction,
					Transfer	transfer,
					Synchronization	synchronization,
					Usage		usage,
					uint16_t	maxPacketSize,
					uint8_t		interval
					) :
					Descriptor(sizeof *this, DescriptorType::kEndpoint),
					number(number),
					reserved0(0),
					direction(direction),
					transfer(transfer),
					synchronization(synchronization),
					usage(usage),
					reserved1(0),
					maxPacketSize(maxPacketSize),
					interval(interval)
					{}
		};
	static_assert(sizeof(EndpointDescriptor) == 7, "unexpected size of USB endpoint descriptor");
	
	
	/*	StringDescriptor0

	*/
	template <unsigned qNumLanguages>
	struct __attribute__((packed)) StringDescriptor0 : public Descriptor {
		/* I think these are defined by Microsoft: https://docs.microsoft.com/en-us/windows/win32/intl/language-identifier-constants-and-strings */
		std::array<uint16_t, qNumLanguages> languages;
	
		constexpr	StringDescriptor0(
					const std::array<uint16_t, qNumLanguages> &languages
					) :
					Descriptor(sizeof *this, DescriptorType::kString),
					languages(languages)
					{}
		};
	
	
	/*	StringDescriptor
	
	*/
	template <unsigned qLength>
	struct __attribute__((packed)) StringDescriptor : public Descriptor {
		std::array<char16_t, qLength + 1> string; // with superfluous terminator
	
		constexpr	StringDescriptor(
					std::array<char16_t, qLength + 1> string // string with terminator
					) :
					Descriptor(sizeof *this - sizeof(char16_t), DescriptorType::kString),
					string(string)
					{}
		};


	/*	HIDClassDescriptor
	
	*/
	template <unsigned kNumDescriptors>
	struct __attribute__((packed)) HIDClassDescriptor : public Descriptor {
		struct __attribute__((packed)) Desc {
			DescriptorType	type;
			uint16_t	length;
			};
	
		uint16_t	hidVersion;
		uint8_t		countryCode;
		uint8_t		descriptorsN;
		std::array<Desc, kNumDescriptors> descriptors;
	
		constexpr	HIDClassDescriptor(
					uint16_t	hidVersion,
					uint8_t		countryCode,
					const std::array<Desc, kNumDescriptors> &descriptors
					) :
					Descriptor(sizeof *this, DescriptorType::kHID),
					hidVersion(hidVersion),
					countryCode(countryCode),
					descriptorsN(kNumDescriptors),
					descriptors(descriptors)
					{}
		};
	// *** don't know how to generalize this assertion
	static_assert(sizeof(HIDClassDescriptor<1>) == 6 + 3 * 1, "unexpected size of USB HID Class<1> descriptor");
	static_assert(sizeof(HIDClassDescriptor<2>) == 6 + 3 * 2, "unexpected size of USB HID Class<2> descriptor");
	
	
	/*	HIDReportDescriptorItem
		Base class of all USB HID Report Descriptor Items
	*/
	struct __attribute__((packed)) HIDReportDescriptorItemPrefix {
		enum Type { kMain, kGlobal, kLocal };
	
		// [USBHID §6.2.2.4] main items
		enum TagMain {
			kInput = 8,
			kOutput,
			kCollection, // note order reversed from [USBHID] listing
			kFeature,
			kCollectionEnd
			};
	
		// [USBHID §6.2.2.7] global items
		enum TagGlobal {
			kUsageGlobal,
			kLogicalMinimum,
			kLogicalMaximum,
			kPhysicalMinimum,
			kPhysicalMaximum,
			kUnitExponent,
			kUnit,
			kReportSize,
			kReportID,
			kReportCount,
			kPush,
			kPop
			};
	
		// [USBHID §6.2.2.8] local items
		enum TagLocal {
			kUsageLocal,
			kUsageMinimum,
			kUsageMaximum,
			kDesignatorIndex,
			kDesignatorMinimum,
			kDesignatorMaximum,
			kStringIndex,
			kStringMinimum,
			kStringMaximum,
			kDelimiter
			};

		unsigned	size2 : 2;
		Type		type : 2;
		unsigned	tag : 4;
	
		static constexpr inline unsigned logb(
					unsigned	i
					) {
					return
						i == 1 ? 1 :
						i == 2 ? 2 :
						i == 4 ? 3 : 0;
					}



		constexpr	HIDReportDescriptorItemPrefix(
					TagMain		tag,
					unsigned	size2
					) :
					tag(tag),
					type(kMain),
					size2(logb(size2))
					{}
	
		constexpr	HIDReportDescriptorItemPrefix(
					TagGlobal	tag,
					unsigned	size2
					) :
					tag(tag),
					type(kGlobal),
					size2(logb(size2))
					{}
	
		constexpr	HIDReportDescriptorItemPrefix(
					TagLocal	tag,
					unsigned	size2
					) :
					tag(tag),
					type(kLocal),
					size2(logb(size2))
					{}
		};
	static_assert(sizeof(HIDReportDescriptorItemPrefix) == 1, "unexpected size of USB HID report descriptor item prefix");
	
	
	template <typename Tag, typename Value>
	struct __attribute__((packed)) HIDReportDescriptorItem {
		HIDReportDescriptorItemPrefix prefix;
		Value		value;
	
		constexpr	HIDReportDescriptorItem(Tag tag, Value value) :
					prefix(tag, sizeof value),
					value(value)
					{}
		};
	
	template <typename Tag>
	struct __attribute__((packed)) HIDReportDescriptorItem<Tag, void> {
		HIDReportDescriptorItemPrefix prefix;
	
		constexpr	HIDReportDescriptorItem(Tag tag) :
					prefix(tag, 0 /* no data */)
					{}
		};

	static_assert(sizeof(HIDReportDescriptorItem<HIDReportDescriptorItemPrefix::TagGlobal, uint8_t>) == 1 + 1, "unexpected size of USB HID report descriptor item (1 byte)");
	static_assert(sizeof(HIDReportDescriptorItem<HIDReportDescriptorItemPrefix::TagGlobal, uint16_t>) == 1 + 2, "unexpected size of USB HID report descriptor item (2 byte)");
	static_assert(sizeof(HIDReportDescriptorItem<HIDReportDescriptorItemPrefix::TagGlobal, uint32_t>) == 1 + 4, "unexpected size of USB HID report descriptor item (4 byte)");
	}
