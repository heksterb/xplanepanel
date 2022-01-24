/*

	QDEC
	
	Quadrature Decoder interface
	
*/

#pragma once

#include <atomic>
#include <cstdint>


extern "C" void QDEC_IRQHandler();


/*	QDEC
	Quadrature decoder
*/
struct QDEC {
protected:
	/* verified this is lock-free on my architecture */
	static std::atomic<bool> gReportReady;
	
	friend void	QDEC_IRQHandler();
	
public:
			QDEC(uint32_t pinA, uint32_t pinB);
	
	explicit operator bool() const { return gReportReady.exchange(false); }
	operator	int32_t() const;
	};
