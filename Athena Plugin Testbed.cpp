// Downloaded from https://developer.x-plane.com/code-sample/hello-world-sdk-3/


#include "XPLMInstance.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"
#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include <string.h>
#include <stdio.h>
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

#ifndef XPLM301
	#error This is made to be compiled against the XPLM301 SDK
#endif

// An opaque handle to the window we will create
static XPLMWindowID	g_window;

// A forward declaration to the function handling the menu
static void AthenaMenuHandler(void * mRef, void * iRef);

//Window forward declarations
void CreateDrawingTestWindow();

// Callbacks we will register when we create our window
void				DrawTestWindow(XPLMWindowID in_window_id, void * in_refcon);
int					dummy_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void * in_refcon) { return 0; }
XPLMCursorStatus	dummy_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void * in_refcon) { return xplm_CursorDefault; }
int					dummy_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void * in_refcon) { return 0; }
void				dummy_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void * in_refcon, int losing_focus) { }

// Globals for the menu system
XPLMMenuID	gkMenuId;
int			giMenuItem;

// Globals for the instanced rendering test
//const char* g_objPath = "lib/airport/vehicles/pushback/tug.obj";
const char* g_objPath = "lib/airport/aircraft/airliners/heavy_e_ual.obj";
XPLMObjectRef g_object = NULL;
XPLMInstanceRef g_instance[3] = { NULL };

// Static functions for instanced drawing test

// Function to check for already loaded object?
static void load_cb(const char* real_path, void* ref)
{//		loading callback function
	XPLMObjectRef* dest = (XPLMObjectRef*)ref;
	if (*dest == NULL)
	{
		*dest = XPLMLoadObject(real_path);
	}
}

PLUGIN_API int XPluginStart(
							char *		outName,
							char *		outSig,
							char *		outDesc)
{
	strcpy(outName, "AthenaTestPlugin");
	strcpy(outSig, "athena.tests.first");
	strcpy(outDesc, "A Hello World plug-in for the XPLM301 SDK being used as a testbed for Athena.");
	
	//First let's register ourselves in the plugin menu
	
	//Insert a menu called "Athena Menu"
	giMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "Athena Menu", NULL, 1);

	//						//which menu to inset in						//menu handler function
	gkMenuId = XPLMCreateMenu("Athena Menu", XPLMFindPluginsMenu(), giMenuItem, AthenaMenuHandler, NULL);
	XPLMAppendMenuItem(gkMenuId, "Test Window", (void *)"Test Window", 1);
	//							//name of menu item		//the string that identifies this item in the handler function

	//Instanced drawing test menu item
	XPLMAppendMenuItem(gkMenuId, "Add Instance", (void *)"Add Instance", 1);

	//Post our test window automatically to ensure the plugin is running
	//CreateTestWindow();

	/* We must return 1 to indicate successful initialization, otherwise we
	 * will not be called back again. */
	
	//Set some properties of our window so that X-Plane respects what we're doing here
	XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
	XPLMSetWindowResizingLimits(g_window, 200, 200, 500, 500);

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	// Since we created the window, we'll be good citizens and clean it up
	XPLMDestroyWindow(g_window);
	g_window = NULL;
	XPLMDestroyMenu(gkMenuId);

	for (int i = 0; i < 3; i++)
	{
		if (g_instance[i])
			XPLMDestroyInstance(g_instance[i]);
		if (g_object)
			XPLMUnloadObject(g_object);
	}
}

/*
 * XPluginDisable
 *
 * We do not need to do anything when we are disabled, but we must provide the handler.
 *
 */
PLUGIN_API void XPluginDisable(void)
{
}

/*
 * XPluginEnable.
 *
 * We don't do any enable-specific initialization, but we must return 1 to indicate
 * that we may be enabled at this time.
 *
 */
PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

/*
 * XPluginReceiveMessage
 *
 * We don't have to do anything in our receive message handler, but we must provide one.
 *
 */
PLUGIN_API void XPluginReceiveMessage(
	XPLMPluginID	inFromWho,
	int				inMessage,
	void *			inParam)
{
}

//function to kick off the creation of the test window
void CreateDrawingTestWindow()
{
	XPLMCreateWindow_t params;
	params.structSize = sizeof(params);
	params.visible = 1;
	params.drawWindowFunc = DrawTestWindow;
	// Note on "dummy" handlers:
	// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
	params.handleMouseClickFunc = dummy_mouse_handler;
	params.handleRightClickFunc = dummy_mouse_handler;
	params.handleMouseWheelFunc = dummy_wheel_handler;
	params.handleKeyFunc = dummy_key_handler;
	params.handleCursorFunc = dummy_cursor_status_handler;
	params.refcon = NULL;
	params.layer = xplm_WindowLayerFloatingWindows;
	// Opt-in to styling our window like an X-Plane 11 native window
	// If you're on XPLM300, not XPLM301, swap this enum for the literal value 1.
	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;

	// Set the window's initial bounds
	// Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
	// We'll need to query for the global desktop bounds!
	int left, bottom, right, top;
	XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
	params.left = left + 50;
	params.bottom = bottom + 150;
	params.right = params.left + 200;
	params.top = params.bottom + 200;

	g_window = XPLMCreateWindowEx(&params);

	// Position the window as a "free" floating window, which the user can drag around
	XPLMSetWindowPositioningMode(g_window, xplm_WindowPositionFree, -1);
	// Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
	XPLMSetWindowResizingLimits(g_window, 200, 200, 300, 300);
	XPLMSetWindowTitle(g_window, "Drawing Test Window");
}


void	DrawTestWindow(XPLMWindowID in_window_id, void * in_refcon)
{
	// Mandatory: We *must* set the OpenGL state before drawing
	// (we can't make any assumptions about it)
	XPLMSetGraphicsState(
						 0 /* no fog */,
						 0 /* 0 texture units */,
						 0 /* no lighting */,
						 0 /* no alpha testing */,
						 1 /* do alpha blend */,
						 1 /* do depth testing */,
						 0 /* no depth writing */
						 );
	
	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);

	int iWordWrapWidth = 180;
	
	float col_white[] = {1.0, 1.0, 1.0}; // red, green, blue
	
	// Display the mouse's position in 2d as a test for string buffers

	int iMouse_globalpos_x;
	int iMouse_globalpos_y;
	char acScratch_Buffer[150];

	XPLMGetMouseLocationGlobal(&iMouse_globalpos_x, &iMouse_globalpos_y);
	sprintf(acScratch_Buffer, "Global mouse location: %d %d\n", iMouse_globalpos_x, iMouse_globalpos_y);
	XPLMDrawString(col_white, l + 10, t - 20, acScratch_Buffer, NULL, xplmFont_Proportional);

	// Leave a note about using the instanced drawing test
	XPLMDrawString(col_white, l + 10, t - 40, "Start instanced drawing\ntest in the plugin menu.", NULL, xplmFont_Proportional);

	/*XPLMDrawString(col_white, l + 10, t - 20, "Hello world!", NULL, xplmFont_Proportional);

	XPLMDrawString(col_white, l + 5, t - 40, "This is a hell", NULL, xplmFont_Proportional);

	XPLMDrawString(col_white, l + 10, t - 60, "of a placeholder text just to test some things y'know", &iWordWrapWidth, xplmFont_Proportional);

	XPLMDrawString(col_white, l + 30, t - 80, "I'm also testing out how wordwrap handles this whole situation", &iWordWrapWidth, xplmFont_Proportional);*/
}

void AddInstancedDrawingTest()
{
	// If object reference not set, find the value needed and set it
	if (!g_object)
	{
		XPLMLookupObjects(g_objPath, 0, 0, load_cb, &g_object);
	}
	if (g_object)
	{
		const char* drefs[] = { "sim/graphics/animation/ground_traffic/tire_steer_deg", NULL };
		if (!g_instance[0])
		{
			g_instance[0] = XPLMCreateInstance(g_object, drefs);
		}
		else if (!g_instance[1])
		{
			g_instance[1] = XPLMCreateInstance(g_object, drefs);
		}
		else if (!g_instance[2])
		{
			g_instance[2] = XPLMCreateInstance(g_object, drefs);
		}
	}

	static XPLMDataRef x = XPLMFindDataRef("sim/flightmodel/position/local_x");
	static XPLMDataRef y = XPLMFindDataRef("sim/flightmodel/position/local_y");
	static XPLMDataRef z = XPLMFindDataRef("sim/flightmodel/position/local_z");
	static XPLMDataRef heading = XPLMFindDataRef("sim/flightmodel/position/psi");
	static XPLMDataRef pitch = XPLMFindDataRef("sim/flightmodel/position/theta");
	static XPLMDataRef roll = XPLMFindDataRef("sim/flightmodel/position/phi");

	static float tire = 0.0;
	tire += 10.0;
	if (tire > 45.0) tire -= 90.0;

	XPLMDrawInfo_t stDrawInfo;
	stDrawInfo.structSize = sizeof(stDrawInfo);
	stDrawInfo.x = XPLMGetDataf(x);
	stDrawInfo.y = XPLMGetDataf(y);
	stDrawInfo.z = XPLMGetDataf(z);
	stDrawInfo.pitch = XPLMGetDataf(pitch);
	stDrawInfo.heading = XPLMGetDataf(heading);
	stDrawInfo.roll = XPLMGetDataf(roll);

	if (g_instance[0] || g_instance[1] || g_instance[2])
	{
		XPLMInstanceSetPosition(g_instance[2] ? g_instance[2] : (g_instance[1] ? g_instance[1] : g_instance[0]), &stDrawInfo, &tire);
	}
}

//Function for handling the plugin menu
void AthenaMenuHandler(void * mRef, void * iRef)
{
	// Check to see which menu item was clicked and then perform the needed function
	if (!strcmp((char *)iRef, "Test Window"))
	{
		CreateDrawingTestWindow();
	}
	if (!strcmp((char *)iRef, "Add Instance"))
	{
		AddInstancedDrawingTest();
	}
}