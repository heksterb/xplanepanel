/*

	MAX6954
	
	Maxim 6954 LED Display Driver interface
	
*/

#pragma once

#include <atomic>
#include <cstdint>

#include "SPIM.h"


extern "C" void GPIOTE_IRQHandler();


/*	MAX6954
	Display driver
*/
struct MAX6954 {
protected:
	friend void GPIOTE_IRQHandler();
	
	enum Register {
		kRegisterNoOperation = 0x00,
		kRegisterDecodeMode = 0x01,
		kRegisterGlobalIntensity = 0x02,
		kRegisterScanLimit = 0x03,
		kRegisterConfiguration = 0x04,
		kRegisterPortConfiguration = 0x06,
		kRegisterTest = 0x07,
		kRegisterKeyAMaskDebounce = 0x08,
		kRegisterDigitTypeKeyAPressed = 0x0c
		};
	
	
	/*	Message
		SPI message
	*/
	union __attribute__((packed)) Message {
		struct __attribute__((packed)) {
			unsigned	data : 8;
			unsigned
					registre : 7,
					read : 1;
			};
		
		uint16_t	i;
		
		operator	uint16_t() const { return i; }
		};
	static_assert(sizeof(Message) == 2);
	
	
	// key pressed
	static std::atomic<bool> gKey;
	
	
	SPIM		&fSPIM;
	
	void		HandleKeyPress();

public:
	/*	Configuration
		
	*/
	union __attribute__((packed)) Configuration {
		struct __attribute__((packed)) {
			unsigned
					shutdownOff : 1,
					unused : 1,
					blinkFast : 1,
					blinkEnable : 1,
					blinkSync : 1,
					clearDigits : 1,
					intensityLocal : 1,
					blinkPhaseP0 : 1; // read-only?
			};
		
		uint8_t		i;
		};
	static_assert(sizeof(Configuration) == 1);
	
	
	explicit	MAX6954(SPIM&);
			MAX6954(const MAX6954&) = delete;
	
	void		DecodeMode(uint8_t);
	void		GlobalIntensity(uint8_t);
	void		ScanLimit(uint8_t);
	void		Configure(const Configuration);
	Configuration	Configure();
	void		PortConfigure(uint8_t);
	void		KeyMask(unsigned char keys, uint8_t value);
	uint8_t		DebouncedKey(unsigned char keys);
	void		DigitType(uint8_t);
	uint8_t		KeyPressed(unsigned char keys);
	
	void		Digit(unsigned char digit, uint8_t value);
	uint8_t		Digit(unsigned char digit);
	
	bool		WasKeyPressed() { return gKey.exchange(false); }
	};
