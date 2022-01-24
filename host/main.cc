/*

	2021/02/11	Downloaded from https://developer.x-plane.com/code-sample/hello-world-sdk-3/

*/

#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMProcessing.h>

#include <optional>

#include <string.h>

#include "hid.h"
#include "xplane.h"


/*	Window
	Window in X-Plane

NOTE
	CRTP used to effect compile-time inheritance
*/
struct Window : public XPlaneWindow<Window> {
protected:
	int		fCounter;
	int		fCOM1Frequency;

public:
			Window();
	
	void		Apply(int counter, int com1frequency);
	void		Draw(XPLMWindowID);
	};


/*	Callback
	Periodic call-back from X-Plane

NOTE
	CRTP used to effect compile-time inheritance
*/
struct Callback : public XPlaneFlightLoop<Callback> {
protected:
	XPLMDataRef
			fCOM1FrequencyMainRef,
			fCOM1FrequencyStandbyRef;

public:
	static constexpr float gPollingInterval = +.1f /* duration in seconds */;
	
	
			Callback();
	
	float		operator()(float elapsedSinceLastCall, float elapsedTimeSinceLastFlightLoop, int counter);
	};



// USB HID interface
static std::optional<Panel> gPanel;

// window
static std::optional<Window> gWindow;

// callback
static std::optional<Callback> gCallback;



/*	Window
	Construct X-Plane window
*/
Window::Window() :
	fCounter(0),
	fCOM1Frequency(0)
{
// Position the window as a "free" floating window, which the user can drag around
SetPositioningMode(xplm_WindowPositionFree, -1);

// Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
SetResizingLimits(200, 200, 300, 300);
SetTitle("Sample Window");
}


/*	Apply
	New data to display
*/
void Window::Apply(
	int		counter,
	int		com1Frequency
	)
{
fCounter = counter;
fCOM1Frequency = com1Frequency;
}


/*	Draw
	Draw the window contents
*/
void Window::Draw(
	XPLMWindowID	windowID
	)
{
// Mandatory: We *must* set the OpenGL state before drawing
// (we can't make any assumptions about it)
XPLMSetGraphicsState(
	0, // no fog
	0, // 0 texture units
	0, // no lighting
	0, // no alpha testing
	1, // do alpha blend
	1, // do depth testing
	0  // no depth writing
	);

int l, t, r, b;
XPLMGetWindowGeometry(windowID, &l, &t, &r, &b);
	
float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
char title[256];
snprintf(title, sizeof title, "Tuned frequency is %d", fCOM1Frequency);
XPLMDrawString(col_white, l + 10, t - 20, title, NULL, xplmFont_Proportional);
}


/*	Callback
	Prepare periodic callback
*/
Callback::Callback() :
	XPlaneFlightLoop<Callback>(xplm_FlightLoop_Phase_AfterFlightModel),
	
	// get data references
	fCOM1FrequencyMainRef(XPLMFindDataRef("sim/cockpit/radios/com1_freq_hz")),
	fCOM1FrequencyStandbyRef(XPLMFindDataRef("sim/cockpit/radios/com1_stdby_freq_hz"))
{
}


/*	()
	Perform callback action
*/
float Callback::operator()(
	float		elapsedSinceLastCall,
	float		elapsedTimeSinceLastFlightLoop,
	int		counter
	)
{
// get COM1 frequency from X-Plane
int
	com1FrequencyMain = XPLMGetDatai(fCOM1FrequencyMainRef) * 10,
	com1FrequencyStandby = XPLMGetDatai(fCOM1FrequencyStandbyRef) * 10;

// USB interface exists?
if (gPanel)
	// synchronize value from X-Plane with panel; did panel's value change?
	if ((*gPanel).Set(com1FrequencyMain, com1FrequencyStandby)) {
		// update based on panel's value
		com1FrequencyMain = gPanel->Value0();
		com1FrequencyStandby = gPanel->Value1();
		
		// synchronize value from panel with X-Plane
		XPLMSetDatai(fCOM1FrequencyMainRef, com1FrequencyMain / 10);
		XPLMSetDatai(fCOM1FrequencyStandbyRef, com1FrequencyStandby / 10);
		}

// make sure window is displaying the current values
if (gWindow)
	gWindow->Apply(counter, com1FrequencyMain);

return gPollingInterval;
}


/*	XPluginStart
	Callback when the plug-in is first loaded
*/
PLUGIN_API int XPluginStart(
	char		outName[256],
	char		outSig[256],
	char		outDesc[256]
	)
{
try {
	static const char gPanelName[] = "PanelPlugIn";
	memcpy(outName, gPanelName, sizeof gPanelName);
	static_assert(sizeof gPanelName <= 256);

	static const char gPanelSignature[] = "org.hekster.panelplugin";
	memcpy(outSig, gPanelSignature, sizeof gPanelSignature);
	static_assert(sizeof gPanelSignature <= 256);

	static const char gPanelDescription[] = "Plug-in to support the radio panel.";
	memcpy(outDesc, gPanelDescription, sizeof gPanelDescription);
	static_assert(sizeof gPanelDescription <= 256);

	// open the connection to the USB device
	gPanel.emplace();

	// create an X-Plane window (for debugging purposes)
	// gWindow.emplace();

	// create the callback
	gCallback.emplace();
	gCallback->Schedule(Callback::gPollingInterval, true /* relative to now */);
	}

catch (...) {
	// *** how do we report errors?
	}

// return whether the USB panel connection and the X-Plane callback were successfully set up
return gPanel && gCallback;
}


/*	XPluginStop
	Called when plug-in is unloaded
*/
PLUGIN_API void	XPluginStop(void)
{
try {
	// stop the callback
	gCallback.reset();

	// close the window
	gWindow.reset();

	// close the connection to the USB device
	gPanel.reset();
	}

catch (...) {}
}


/*	XPluginEnable
	Called when plug-in is enabled
*/
PLUGIN_API int XPluginEnable()
{
// started successfully
return 1;
}


/*	XPluginDisable
	Called when plug-in is disabled
*/
PLUGIN_API void XPluginDisable()
{
}


/*	XPluginReceiveMessage
	Handle messages from X-Plane
*/
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
// *** should I be handling anything?
}
