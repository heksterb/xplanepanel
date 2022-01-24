/*
	device
	
	USB Setup abstraction for Win32
	
	[WIN32HID]	https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/hid-clients
*/

#include "device.h"


/*	DeviceInformationSet
	Represent the set of devices in the collection with the given GUID
*/
DeviceInformationSet::DeviceInformationSet(
	GUID		guid
	) :
	fHIDGUID(guid),
	
	// get handle to 'device information set'
	fHandle(
		SetupDiGetClassDevs(&fHIDGUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)
		)
{
if (fHandle == INVALID_HANDLE_VALUE) throw GetLastError();
}


/*	~DeviceInformationSet
	Stop representing the collection
*/
DeviceInformationSet::~DeviceInformationSet() noexcept(false)
{
if (!SetupDiDestroyDeviceInfoList(fHandle)) throw GetLastError();
}


/*	Iterator::!=
	Return whether the iterator is not at the end
	
	The 'at end' iterator is represented by the integer type.
*/
bool DeviceInformationSet::Iterator::operator!=(
	int
	)
{
bool haveValue;

// able to get HID interfaces of that device?
if (SetupDiEnumDeviceInterfaces(fSet, nullptr /* all devices */, &fSet.fHIDGUID, fIndex, &fInterface))
	haveValue = true;

else
	switch (const DWORD error = GetLastError()) {
		// no more interfaces to enumerate?
		case ERROR_NO_MORE_ITEMS:
			haveValue = false;
			break;
		
		default:
			throw error;
		}

// not at end if we have a value
return haveValue;
}


/*	DeviceInformationSet::Detail
	Represent the detailed information of the given device
*/
DeviceInformationSet::Detail::Detail(
	HDEVINFO	set,
	SP_DEVICE_INTERFACE_DATA &deviceInterface
	)
{
// get device interface detail (contains essentially the 'device path')
fDetail.s.cbSize = sizeof fDetail.s;
fDeviceInfoData.cbSize = sizeof fDeviceInfoData;
if (!SetupDiGetDeviceInterfaceDetail(set, &deviceInterface, &fDetail.s, sizeof fDetail, nullptr, &fDeviceInfoData)) throw GetLastError();
/* returns something like: \\?\hid#vid_f055&pid_1234#6&52060e2&3&0000#{4d1e55b2-f16f-11cf-88cb-001111000030} */
}


/*	DeviceInformationSet::Property
	Return a property of the given device
*/
DeviceInformationSet::Property::Property(
	HDEVINFO	set,
	SP_DEVINFO_DATA	&deviceInterface,
	const DWORD	property
	)
{
if (!SetupDiGetDeviceRegistryProperty(set, &deviceInterface, property, &fPropertyRegistryType, fBuffer, sizeof fBuffer, nullptr))
	throw GetLastError();
}


/*	DeviceInformationSet::Property::AsMultiString
	Return the property as an array of Unicode (UTF-16) strings
*/
const wchar_t *DeviceInformationSet::Property::AsMultiString() const
{
// ensure the property type is as expected
if (fPropertyRegistryType != REG_MULTI_SZ) throw "unexpected property type";

return reinterpret_cast<const wchar_t*>(fBuffer);
}
