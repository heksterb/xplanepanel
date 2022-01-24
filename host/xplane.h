/*

	xplane

	X-Plane C++ Abstraction

*/

#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMProcessing.h>

#if IBM
	#include <windows.h>
#endif
#if LIN
	#include <GL/gl.h>
#elif __GNUC__
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif

#ifndef XPLM300
	#error This is made to be compiled against the XPLM300 SDK
#endif


/*

	XPlaneWindow

*/

template <typename Behavior>
struct XPlaneWindow {
protected:
	static XPLMCreateWindow_t CreateParams(XPlaneWindow<Behavior> *that);
	
	static void	DrawHandler(XPLMWindowID windowID, void *const refCon) {
				static_cast<Behavior*>(refCon)->Draw(windowID);
				}
	static int	MouseClickHandler(XPLMWindowID windowID, int x, int y, int isDown, void *const refCon) {
				return static_cast<Behavior*>(refCon)->MouseClick(windowID, x, y, isDown);
				}
	static int	WheelHandler(XPLMWindowID windowID, int x, int y, int wheel, int clicks, void *const refCon) {
				return static_cast<Behavior*>(refCon)->Wheel(windowID, x, y, wheel, clicks);
				}
	static void	KeyHandler(XPLMWindowID windowID, char key, XPLMKeyFlags flags, char virtualKey, void *const refCon, int losingFocus) {
				static_cast<Behavior*>(refCon)->Key(windowID, key, flags, virtualKey, losingFocus);
				}
	static XPLMCursorStatus CursorStatusHandler(XPLMWindowID windowID, int x, int y, void *const refCon) {
				return static_cast<Behavior*>(refCon)->CursorStatus(windowID, x, y);
				}
	
	const XPLMWindowID fID;

public:
			XPlaneWindow() :
				fID(XPLMCreateWindowEx(&CreateParams(this)))
				{}
			~XPlaneWindow() { XPLMDestroyWindow(fID); }

	operator	XPLMWindowID() { return fID; }
	
	// behavior; the defaults here may be statically overridden by the subclass
	XPLMCursorStatus CursorStatus(XPLMWindowID, int x, int y) { return xplm_CursorDefault; }
	void		Draw(XPLMWindowID) {}
	int		MouseClick(XPLMWindowID, int x, int y, int isDown) { return 0; }
	int		Wheel(XPLMWindowID, int x, int y, int wheel, int clicks) { return 0; }
	void		Key(XPLMWindowID, char key, XPLMKeyFlags, char virtualKey, int losingFocus) {}

	// accessors
	void		SetResizingLimits(
				unsigned	minWidth,
				unsigned	minHeight,
				unsigned	maxWidth,
				unsigned	maxHeight
				) {
				XPLMSetWindowResizingLimits(fID, minWidth, minHeight, maxWidth, maxHeight);
				}
	void		SetTitle(const char title[]) { XPLMSetWindowTitle(fID, title); }
	void		SetPositioningMode(
				XPLMWindowPositioningMode mode,
				int		monitorIndex
				) {
				XPLMSetWindowPositioningMode(fID, mode, monitorIndex);
				}
	};


template <typename Behavior>
XPLMCreateWindow_t XPlaneWindow<Behavior>::CreateParams(
	XPlaneWindow<Behavior> *that
	)
{
XPLMCreateWindow_t params;
params.structSize = sizeof params;
	params.visible = 1;
params.drawWindowFunc = DrawHandler;
	// Note on "dummy" handlers:
	// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
params.handleMouseClickFunc = MouseClickHandler;
params.handleRightClickFunc = MouseClickHandler;
params.handleMouseWheelFunc = WheelHandler;
params.handleKeyFunc = KeyHandler;
params.handleCursorFunc = CursorStatusHandler;
params.refcon = that;

	params.layer = xplm_WindowLayerFloatingWindows;
	// Opt-in to styling our window like an X-Plane 11 native window
	// If you're on XPLM300, not XPLM301, swap this enum for the literal value 1.
	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
	
// Set the window's initial bounds
int left, bottom, right, top;
XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
params.left = left + 50;
params.bottom = bottom + 150;
params.right = params.left + 200;
params.top = params.bottom + 200;

return params;
}



/*

	XPlaneFlightLoop

*/

template <typename Behavior>
struct XPlaneFlightLoop {
protected:
	static XPLMCreateFlightLoop_t CreateParams(
				XPlaneFlightLoop<Behavior> *const that,
				XPLMFlightLoopPhaseType phase
				) {
				XPLMCreateFlightLoop_t params;
				params.structSize = sizeof params;
				params.phase = phase;
				params.callbackFunc = &Callback;
				params.refcon = that;

				return params;
				}

	static float	Callback(
				float elapsedSinceLastCall,
				float elapsedTimeSinceLastFlightLoop,
				int counter,
				void *refCon
				) {
				return static_cast<Behavior*>(refCon)->operator()(elapsedSinceLastCall, elapsedTimeSinceLastFlightLoop, counter);
				}

	const XPLMFlightLoopID	fID;

public:
			XPlaneFlightLoop(
				XPLMFlightLoopPhaseType phase
				) :
				fID(XPLMCreateFlightLoop(&CreateParams(this, phase)))
				{}

			~XPlaneFlightLoop() {
				XPLMDestroyFlightLoop(fID);
				}
	
	// overridable behavior
	float		operator()(float elapsedSinceLastCall, float elapsedTimeSinceLastFlightLoop, int counter) { return 0 /* don't reschedule */; }

	// accessors
	void		Schedule(float interval, int relativeToNow) {
				XPLMScheduleFlightLoop(fID, interval, relativeToNow);
				}
	};

