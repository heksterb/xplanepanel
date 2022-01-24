/*
	hid
	
	USB HID abstraction for Win32
*/

#include "device.h"
#include "hid.h"


/*	GetHIDGUID
	Get HID 'Device Interface Class' GUID
*/
static GUID GetHIDGUID()
{
// 4D1E55B2-F16F-11CF-88CB-001111000030
GUID hidGUID;
HidD_GetHidGuid(&hidGUID);

return hidGUID;
}


/*	Panel::Handle
	Close Win32 handle
*/
Panel::Handle::~Handle()
{
CloseHandle(fHandle);
}


/*	OpenDevice
	Find and open the USB HID panel device; return its device handle
*/
HANDLE Panel::OpenDevice()
{
// represent the USB HID device collection
DeviceInformationSet deviceInformationSet(GetHIDGUID());

// for all interfaces of devices of the set
std::optional<DeviceInformationSet::Detail> deviceInterfaceDetail;
for (SP_DEVICE_INTERFACE_DATA &deviceInterface: deviceInformationSet) { // Visual Studio 15.9.36 braces are needed here or won't iterate
	// see if this interface is the one belonging to our device
	try {
		// get details about this specific interface (device path and device interface data)
		deviceInterfaceDetail.emplace(deviceInformationSet, deviceInterface);
		
		// get hardware ID
		DeviceInformationSet::Property hardwareID(deviceInformationSet, deviceInterfaceDetail->Information(), SPDRP_HARDWAREID);
		/* returns: HID\VID_F055&PID_1234&REV_0001 */
		
		// hardware ID matches our device?
		static const wchar_t gDeviceIdentifier[] = L"HID\\VID_F055&PID_1234";
		if (wcsncmp(hardwareID.AsMultiString(), gDeviceIdentifier, sizeof gDeviceIdentifier / sizeof *gDeviceIdentifier - 1) == 0) break;
		
		// wasn't the device we're looking for
		deviceInterfaceDetail.reset();
		}
	
	catch (...) {
		// failure to enumerate this interface doesn't prevent us from continuing
		}
	}

// didn't find our device?
if (!deviceInterfaceDetail) throw "can't find panel device";

// open
HANDLE handle = CreateFile(
	deviceInterfaceDetail->Path(),
	GENERIC_READ | GENERIC_WRITE,
	FILE_SHARE_READ | FILE_SHARE_WRITE,
	nullptr /* security attributes */,
	OPEN_EXISTING,
	FILE_FLAG_OVERLAPPED,
	nullptr /* template file */
	);
if (handle == INVALID_HANDLE_VALUE) throw GetLastError();

return handle;
}


/*	OurFirmwareVersion
	Verify the panel is ours, and return our firmware version number
*/
unsigned short Panel::OurFirmwareVersion(
	HANDLE		device
	)
{
// get USB attributes
HIDD_ATTRIBUTES attributes;
if (!HidD_GetAttributes(device, &attributes)) throw GetLastError();
if (! (
	attributes.VendorID == 0xF055 &&
	attributes.ProductID == 0x1234
	)) throw "wasn't expected panel device";

// return firmware version
return attributes.VersionNumber;
}


/*	Panel
	Open the connection to the USB panel
*/
Panel::Panel() :
	fHandle(OpenDevice()),
	fFirmwareVersion(OurFirmwareVersion(fHandle))
{
// get HID driver 'preparsed data'
if (!HidD_GetPreparsedData(fHandle, &fPreparsed)) throw GetLastError();

// should be able to query the value array, but get a weird "not implemented" error
HIDP_CAPS capabilities;
if (HidP_GetCaps(fPreparsed, &capabilities) != HIDP_STATUS_SUCCESS) throw "can't get device capabilities";
// if (capabilities.OutputReportByteLength != sizeof(Report)) throw "unexpected panel HID report size";
}


/*	~Panel
	Close the connection to the USB panel
*/
Panel::~Panel()
{
(void) HidD_FreePreparsedData(fPreparsed);
}


/*	Set
	Apply values to display
	
	The provided values are the target we want the device to display; a USB write request will be made
	only if needed and when possible.  Returns whether the device itself reported updated values.
*/
bool Panel::Set(
	unsigned	value0,
	unsigned	value1
	)
{
bool readUpdatedValue = false;

// until we can keep an asynchronous read request pending
/* We don't actually *need* to have a read request pending; it would have been fine to just make a synchronous
   nonblocking request to check if the device sent data.  But I don't think that's an option in Win32.
   In any case, we need asynchronous writes; so for symmetry, we just read asynchronously as well. */
do {
	// no read request pending?
	if (!fRead) {
		// prepare the asynchronous read request
		fRead.emplace();
		fRead->fOverlapped.Offset = fRead->fOverlapped.OffsetHigh = 0;
		fRead->fOverlapped.hEvent = nullptr; // use file handle as synchronization object
		
		// make asynchronous read request
		if (!ReadFile(fHandle, &fRead->fReport, sizeof fRead->fReport, nullptr, &fRead->fOverlapped))
			// call should only 'fail' because it is now pending
			switch (const DWORD error = GetLastError()) {
				case ERROR_IO_PENDING: break;
				default:	throw error;
				}
		else
			// read succeeded immediately; assume that GetOverlappedResult still works as if asynchronous
			/* This happens when there were multiple read reports issued from the USB device within the
			   intervening polling interval. */
			;
		}
	
	// asynchronous read request complete?
	if (
		DWORD read;
		GetOverlappedResult(fHandle, &fRead->fOverlapped, &read, false /* wait */)
		) {
		// extract read value
		fReadValue0 = fRead->fReport.value0;
		fReadValue1 = fRead->fReport.value1;
		readUpdatedValue = true;
		
		// request no longer pending
		fRead.reset();
		}
		
	else
		// should only 'fail' because it has not yet completed
		switch (DWORD error = GetLastError()) {
			case ERROR_IO_INCOMPLETE: break;
			default:	throw error;
			}
	} while (!fRead);

// pending asynchronous write?
if (fWrite)
	// completed?
	if (
		DWORD wrote;
		GetOverlappedResult(fHandle, &fWrite->fOverlapped, &wrote, false /* wait */)
		) {
		// record written value
		fWroteValue0 = fWrite->fReport.value0;
		fWroteValue1 = fWrite->fReport.value1;
		
		// request no longer pending
		fWrite.reset();
		}
	
	else
		// should only 'fail' because it has not yet completed
		switch (DWORD error = GetLastError()) {
			case ERROR_IO_INCOMPLETE: break;
			default:	throw error;
			}

// need to update written value?
if (value0 != fWroteValue0 || value1 != fWroteValue1)
	// no write already pending?
	if (!fWrite) {
		// prepare an asynchronous write request
		fWrite.emplace();
		fWrite->fReport.reportID = 0;
		fWrite->fReport.value0 = value0;
		fWrite->fReport.value1 = value1;
		fWrite->fOverlapped.Offset = fWrite->fOverlapped.OffsetHigh = 0;
		fWrite->fOverlapped.hEvent = nullptr; // use file handle as synchronization object
		
		// make asynchronous write request
		if (!WriteFile(fHandle, &fWrite->fReport, sizeof fWrite->fReport, nullptr, &fWrite->fOverlapped))
			// call should only 'fail' because it is now pending
			switch (const DWORD error = GetLastError()) {
				case ERROR_IO_PENDING: break;
				default:	throw error;
				}
		else
			// write succeeded immediately; assume that GetOverlappedResult still works as if asynchronous
			;
		}

return readUpdatedValue;
}
