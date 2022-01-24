/*

	SPIM
	
	SPI Master interface
	
*/

#pragma once

#include <cstdint>


extern "C" void SPIM3_IRQHandler();


/*	SPIM
	SPI master
*/
struct SPIM {
protected:
	static volatile bool gEnd;
	
	friend void SPIM3_IRQHandler();
	
public:
			SPIM(
				uint32_t pinCS,
				uint32_t pinClock,
				uint32_t pinMOSI,
				uint32_t pinMISO
				);
	
	uint16_t	operator()(uint16_t) const;
	};
