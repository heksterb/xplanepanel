/*
	device
	
	USB Setup abstraction for Win32
	
	2021/05/23	Originated
*/

#pragma once

#include <OBJBASE.H>
#include <WINBASE.H>
#include <WINNT.H>
#include <SETUPAPI.H>


/*	DeviceInformationSet
	Represent a 'device information set' for the USB HID devices
	
	https://docs.microsoft.com/en-us/windows-hardware/drivers/install/device-information-sets
*/
struct DeviceInformationSet {
public:
	/*	Iterator
		Iterate over interfaces of devices in the set
	*/
	struct Iterator {
	protected:
		DeviceInformationSet &fSet;
		unsigned	fIndex;
		SP_DEVICE_INTERFACE_DATA fInterface;
	
	public:
				Iterator(DeviceInformationSet &set) :
					fSet(set),
					fIndex(0)
					{
					fInterface.cbSize = sizeof fInterface;
					}
		
		SP_DEVICE_INTERFACE_DATA &operator*() { return fInterface; }
		void		operator++() { fIndex++; }
		bool		operator!=(int);
		};
	
	
	/*	Detail
		Detailed device information
	*/
	struct Detail {
	protected:
		static constexpr unsigned kMaxPathLength = 256;
		union {
			SP_DEVICE_INTERFACE_DETAIL_DATA s;
			char		buffer[
						offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA, DevicePath) +
							(kMaxPathLength + 1) * sizeof(wchar_t)
						];
			} fDetail;
		SP_DEVINFO_DATA fDeviceInfoData;

	public:
				Detail(HDEVINFO, SP_DEVICE_INTERFACE_DATA&);
		
		SP_DEVINFO_DATA	&Information() { return fDeviceInfoData; }
		const wchar_t	*Path() const { return fDetail.s.DevicePath; }
		};
	
	
	/*	Property
		Device information property
	*/
	struct Property {
	protected:
		static constexpr unsigned kBufferSize = 1024;
		
		BYTE		fBuffer[kBufferSize];
		DWORD		fPropertyRegistryType;
		DWORD		fPropertyRegistryValueSize;
	
	public:
				Property(HDEVINFO, SP_DEVINFO_DATA&, DWORD property);
		
		const wchar_t	*AsMultiString() const;
		};

protected:
	const GUID	fHIDGUID;
	HDEVINFO	fHandle;
	
public:
			DeviceInformationSet(GUID);
			DeviceInformationSet(const DeviceInformationSet&) = delete;
			~DeviceInformationSet() noexcept(false);
	
	operator	HDEVINFO() const { return fHandle; }
	
	// iterate over devices in the set
	Iterator 	begin() { return Iterator(*this); }
	int		end() { return 0; }
	};


