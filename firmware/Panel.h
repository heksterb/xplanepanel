/*

	Panel
	
	Flight simulator radio panel
	
*/

#pragma once

#include "MAX6954.h"
#include "QDEC.h"
#include "SPIM.h"


/*	Panel
	Application

*/
struct Panel {
protected:	
	signed		fAccumulate;
	unsigned
			fValue = 121500,
			fValueStandby = 122900;
	
	SPIM		fSPIM;
	MAX6954		fMAX;
	QDEC		fQDEC;
	
	
	void		UpdateOneDisplay(uint8_t base, unsigned value);
	void		UpdateDisplay();
	void		ProcessQDEC();
	void		ProcessMAXKeyPress();

public:
			Panel();
	
	void		Loop();
	void		SetValue(unsigned, unsigned);
	};


