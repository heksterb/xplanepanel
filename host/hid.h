/*
	hid
	
	USB HID abstraction for Win32
*/

#pragma once

#include <OBJBASE.H>
#include <WINBASE.H>
#include <WINNT.H>
#include <HIDSDI.h>

#include <memory>
#include <optional>


/*	Panel
	Connection to the panel through USB HID
*/
struct Panel {
protected:
	/*	Report
		USB HID Report
	*/
	#pragma pack(push, 1)
	struct Report {
		char		reportID;
		unsigned long long
				value0 : 20,
				value1 : 20;
		};
	#pragma pack(pop)
	
	
	/*	IO
		USB HID I/O request
	*/
	struct IO {
		OVERLAPPED	fOverlapped;
		Report		fReport;
		};
	
	
	/*	Handle
		Win32 handle
	*/
	struct Handle {
	protected:
		HANDLE		fHandle;
	
	public:
				Handle(HANDLE handle) : fHandle(handle) {}
				~Handle();
		
		operator	HANDLE() const { return fHandle; }
		};
	
	
	static HANDLE	OpenDevice();
	static unsigned short OurFirmwareVersion(HANDLE);
	
	std::optional<IO>
			fRead,
			fWrite;
	
	unsigned	fReadValue0,
			fReadValue1,
			fWroteValue0,
			fWroteValue1;
	
	const Handle	fHandle;
	PHIDP_PREPARSED_DATA fPreparsed;
	const unsigned short fFirmwareVersion;

public:
			Panel();
			~Panel();
	
	operator	HANDLE() { return fHandle; }
	
	unsigned short	FirmwareVersion() const { return fFirmwareVersion; }
	
	bool		Set(unsigned valueMain, unsigned valueStandby);
	unsigned	Value0() const { return fReadValue0; }
	unsigned	Value1() const { return fReadValue1; }
	};
