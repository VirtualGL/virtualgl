// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011, 2013-2016, 2019-2023 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include <unistd.h>
#include "Mutex.h"
#include <X11/Xlibint.h>
#include "ContextHash.h"
#include "EGLXDisplayHash.h"
#include "EGLXWindowHash.h"
#include "ContextHashEGL.h"
#include "PbufferHashEGL.h"
#include "GLXDrawableHash.h"
#include "GlobalCriticalSection.h"
#include "PixmapHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "fakerconfig.h"
#include "threadlocal.h"
#include <dlfcn.h>
#include "faker.h"


namespace faker {

Display *dpy3D = NULL;
bool deadYet = false;
char *glExtensions = NULL;
EGLint eglMajor = 0, eglMinor = 0;
VGL_THREAD_LOCAL(TraceLevel, long, 0)
VGL_THREAD_LOCAL(FakerLevel, long, 0)
VGL_THREAD_LOCAL(GLXExcludeCurrent, bool, false)
VGL_THREAD_LOCAL(EGLExcludeCurrent, bool, false)
VGL_THREAD_LOCAL(OGLExcludeCurrent, bool, false)
VGL_THREAD_LOCAL(AutotestColor, long, -1)
VGL_THREAD_LOCAL(AutotestRColor, long, -1)
VGL_THREAD_LOCAL(AutotestFrame, long, -1)
VGL_THREAD_LOCAL(AutotestDisplay, Display *, NULL)
VGL_THREAD_LOCAL(AutotestDrawable, long, 0)
VGL_THREAD_LOCAL(EGLXContextCurrent, bool, false)
VGL_THREAD_LOCAL(EGLError, long, EGL_SUCCESS)
VGL_THREAD_LOCAL(CurrentEGLXDisplay, EGLXDisplay *, NULL)


static void cleanup(void)
{
	if(PixmapHash::isAlloc()) PMHASH.kill();
	if(VisualHash::isAlloc()) VISHASH.kill();
	if(ContextHash::isAlloc()) CTXHASH.kill();
	if(GLXDrawableHash::isAlloc()) GLXDHASH.kill();
	if(WindowHash::isAlloc()) WINHASH.kill();
	if(EGLXDisplayHash::isAlloc()) EGLXDPYHASH.kill();
	if(EGLXWindowHash::isAlloc()) EGLXWINHASH.kill();
	if(backend::ContextHashEGL::isAlloc()) CTXHASHEGL.kill();
	if(backend::PbufferHashEGL::isAlloc()) PBHASHEGL.kill();
	if(backend::RBOContext::isAlloc()) RBOCONTEXT.kill();
	free(glExtensions);
	unloadSymbols();
}


void safeExit(int retcode)
{
	bool shutdown;

	globalMutex.lock(false);
	shutdown = deadYet;
	if(!deadYet)
	{
		deadYet = true;
		cleanup();
		fconfig_deleteinstance();
	}
	globalMutex.unlock(false);
	if(!shutdown)
	{
		if(!strcasecmp(fconfig.exitfunction, "_exit"))
			_exit(retcode);
		else if(!strcasecmp(fconfig.exitfunction, "abort"))
			abort();
		else
			exit(retcode);
	}
	else pthread_exit(0);
}


class GlobalCleanup
{
	public:

		~GlobalCleanup()
		{
			faker::GlobalCriticalSection *gcs =
				faker::GlobalCriticalSection::getInstance(false);
			if(gcs) gcs->lock(false);
			fconfig_deleteinstance(gcs);
			deadYet = true;
			if(gcs) gcs->unlock(false);
		}
};
GlobalCleanup globalCleanup;


// Used when VGL_TRAPX11=1

int xhandler(Display *dpy, XErrorEvent *xe)
{
	char temps[256];

	temps[0] = 0;
	XGetErrorText(dpy, xe->error_code, temps, 255);
	vglout.PRINT("[VGL] WARNING: X11 error trapped\n[VGL]    Error:  %s\n[VGL]    XID:    0x%.8x\n",
		temps, xe->resourceid);
	return 0;
}

static const char *getX11ErrorName(CARD8 errorCode)
{
	switch (errorCode)
	{
	case Success:
		return "Success"; /* everything's okay */
	case BadRequest:
		return "BadRequest"; /* bad request code */
	case BadValue:
		return "BadValue"; /* int parameter out of range */
	case BadWindow:
		return "BadWindow"; /* parameter not a Window */
	case BadPixmap:
		return "BadPixmap"; /* parameter not a Pixmap */
	case BadAtom:
		return "BadAtom"; /* parameter not an Atom */
	case BadCursor:
		return "BadCursor"; /* parameter not a Cursor */
	case BadFont:
		return "BadFont"; /* parameter not a Font */
	case BadMatch:
		return "BadMatch"; /* parameter mismatch */
	case BadDrawable:
		return "BadDrawable"; /* parameter not a Pixmap or Window */
	case BadAccess:
		return "BadAccess"; /* depending on context:
		 - key/button already grabbed
		 - attempt to free an illegal
		   cmap entry
		- attempt to store into a read-only
		   color map entry.
		- attempt to modify the access control
		   list from other than the local host.
		*/
	case BadAlloc:
		return "BadAlloc"; /* insufficient resources */
	case BadColor:
		return "BadColor"; /* no such colormap */
	case BadGC:
		return "BadGC"; /* parameter not a GC */
	case BadIDChoice:
		return "BadIDChoice"; /* choice not in range or already used */
	case BadName:
		return "BadName"; /* font or color name doesn't exist */
	case BadLength:
		return "BadLength"; /* Request length incorrect */
	case BadImplementation:
		return "BadImplementation"; /* server is defective */

	case FirstExtensionError:
		return "FirstExtensionError";
	case LastExtensionError:
		return "LastExtensionError";
	default:
		return "(unknown)";
	}
}

static const char *getGLXErrorName(CARD8 errorCode)
{
	switch (errorCode)
	{
	case GLXBadContext:
		return "GLXBadContext";
	case GLXBadContextState:
		return "GLXBadContextState";
	case GLXBadDrawable:
		return "GLXBadDrawable";
	case GLXBadPixmap:
		return "GLXBadPixmap";
	case GLXBadContextTag:
		return "GLXBadContextTag";
	case GLXBadCurrentWindow:
		return "GLXBadCurrentWindow";
	case GLXBadRenderRequest:
		return "GLXBadRenderRequest";
	case GLXBadLargeRequest:
		return "GLXBadLargeRequest";
	case GLXUnsupportedPrivateRequest:
		return "GLXUnsupportedPrivateRequest";
	case GLXBadFBConfig:
		return "GLXBadFBConfig";
	case GLXBadPbuffer:
		return "GLXBadPbuffer";
	case GLXBadCurrentDrawable:
		return "GLXBadCurrentDrawable";
	case GLXBadWindow:
		return "GLXBadWindow";
	case GLXBadProfileARB:
		return "GLXBadProfileARB";
	default:
		return "(unknown)";
	}
}

static const char *getGLXCommandName(CARD16 minorCode)
{
	switch (minorCode)
	{
	case X_GLXRender:
		return "GLXRender";
	case X_GLXRenderLarge:
		return "GLXRenderLarge";
	case X_GLXCreateContext:
		return "GLXCreateContext";
	case X_GLXDestroyContext:
		return "GLXDestroyContext";
	case X_GLXMakeCurrent:
		return "GLXMakeCurrent";
	case X_GLXIsDirect:
		return "GLXIsDirect";
	case X_GLXQueryVersion:
		return "GLXQueryVersion";
	case X_GLXWaitGL:
		return "GLXWaitGL";
	case X_GLXWaitX:
		return "GLXWaitX";
	case X_GLXCopyContext:
		return "GLXCopyContext";
	case X_GLXSwapBuffers:
		return "GLXSwapBuffers";
	case X_GLXUseXFont:
		return "GLXUseXFont";
	case X_GLXCreateGLXPixmap:
		return "GLXCreateGLXPixmap";
	case X_GLXGetVisualConfigs:
		return "GLXGetVisualConfigs";
	case X_GLXDestroyGLXPixmap:
		return "GLXDestroyGLXPixmap";
	case X_GLXVendorPrivate:
		return "GLXVendorPrivate";
	case X_GLXVendorPrivateWithReply:
		return "GLXVendorPrivateWithReply";
	case X_GLXQueryExtensionsString:
		return "GLXQueryExtensionsString";
	case X_GLXQueryServerString:
		return "GLXQueryServerString";
	case X_GLXClientInfo:
		return "GLXClientInfo";
	case X_GLXGetFBConfigs:
		return "GLXGetFBConfigs";
	case X_GLXCreatePixmap:
		return "GLXCreatePixmap";
	case X_GLXDestroyPixmap:
		return "GLXDestroyPixmap";
	case X_GLXCreateNewContext:
		return "GLXCreateNewContext";
	case X_GLXQueryContext:
		return "GLXQueryContext";
	case X_GLXMakeContextCurrent:
		return "GLXMakeContextCurrent";
	case X_GLXCreatePbuffer:
		return "GLXCreatePbuffer";
	case X_GLXDestroyPbuffer:
		return "GLXDestroyPbuffer";
	case X_GLXGetDrawableAttributes:
		return "GLXGetDrawableAttributes";
	case X_GLXChangeDrawableAttributes:
		return "GLXChangeDrawableAttributes";
	case X_GLXCreateWindow:
		return "GLXCreateWindow";
	case X_GLXDestroyWindow:
		return "GLXDestroyWindow";
	case X_GLXSetClientInfoARB:
		return "GLXSetClientInfoARB";
	case X_GLXCreateContextAttribsARB:
		return "GLXCreateContextAttribsARB";
	case X_GLXSetClientInfo2ARB:
		return "GLXSetClientInfo2ARB";
	default:
		return "(unknown)";
	}
}

void sendGLXError(Display *dpy, CARD16 minorCode, CARD8 errorCode,
	bool x11Error)
{
	xError error;
	int majorCode, errorBase, dummy;

	if(!backend::queryExtension(dpy, &majorCode, &dummy, &errorBase))
	{
		// Cannot send error using GLX, so throw
		char errorStr[256];
		snprintf(errorStr, 256, "Error while faking GLX command %s: error code %s",
				 getGLXCommandName(minorCode),
				 (x11Error ? getX11ErrorName(errorCode) : getGLXErrorName(errorCode)));
		THROW(errorStr);
	}

	if(!fconfig.egl) dpy = DPY3D;

	LockDisplay(dpy);

	error.type = X_Error;
	error.errorCode = x11Error ? errorCode : errorBase + errorCode;
	error.sequenceNumber = dpy->request;
	error.resourceID = 0;
	error.minorCode = minorCode;
	error.majorCode = majorCode;
	_XError(dpy, &error);

	UnlockDisplay(dpy);
}


// Called from XOpenDisplay(), unless a GLX function is called first

void init(void)
{
	static int init = 0;

	if(init) return;
	GlobalCriticalSection::SafeLock l(globalMutex);
	if(init) return;
	init = 1;

	fconfig_reloadenv();
	if(strlen(fconfig.log) > 0) vglout.logTo(fconfig.log);

	if(fconfig.verbose)
		vglout.println("[VGL] %s v%s %d-bit (Build %s)", __APPNAME, __VERSION,
			(int)sizeof(size_t) * 8, __BUILD);

	if(getenv("VGL_DEBUG"))
	{
		vglout.print("[VGL] Attach debugger to process %d ...\n", getpid());
		fgetc(stdin);
	}
	if(fconfig.trapx11) XSetErrorHandler(xhandler);
}


Display *init3D(void)
{
	init();

	if(!dpy3D)
	{
		GlobalCriticalSection::SafeLock l(globalMutex);
		if(!dpy3D)
		{
			if(fconfig.egl)
			{
				int numDevices = 0, i, validDevices = 0;
				const char *exts = NULL;
				EGLDeviceEXT *devices = NULL;

				if(fconfig.verbose)
					vglout.println("[VGL] Opening EGL device %s",
						strlen(fconfig.localdpystring) > 0 ?
						fconfig.localdpystring : "(default)");

				try
				{
					if(!(exts = _eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)))
						THROW("Could not query EGL extensions");
					if(!strstr(exts, "EGL_EXT_platform_device"))
						THROW("EGL_EXT_platform_device extension not available");

					if(!_eglQueryDevicesEXT(0, NULL, &numDevices) || numDevices < 1)
						THROW("No EGL devices found");
					if((devices =
						(EGLDeviceEXT *)malloc(sizeof(EGLDeviceEXT) * numDevices)) == NULL)
						THROW("Memory allocation failure");
					if(!_eglQueryDevicesEXT(numDevices, devices, &numDevices)
						|| numDevices < 1)
						THROW("Could not query EGL devices");
					for(i = 0; i < numDevices; i++)
					{
						EGLDisplay edpy =
							_eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, devices[i],
								NULL);
						if(!edpy || !_eglInitialize(edpy, &eglMajor, &eglMinor))
							continue;
						_eglTerminate(edpy);
						if(!strcasecmp(fconfig.localdpystring, "egl"))
							break;
						if(!strncasecmp(fconfig.localdpystring, "egl", 3))
						{
							char temps[80];

							snprintf(temps, 80, "egl%d", validDevices);
							if(!strcasecmp(fconfig.localdpystring, temps))
								break;
						}
						const char *devStr =
							_eglQueryDeviceStringEXT(devices[i], EGL_DRM_DEVICE_FILE_EXT);
						if(devStr && !strcmp(devStr, fconfig.localdpystring))
							break;
						validDevices++;
					}
					if(i == numDevices) THROW("Invalid EGL device");

					if(!(dpy3D =
						(Display *)_eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT,
							devices[i], NULL)))
						THROW("Could not open EGL display");
					free(devices);  devices = NULL;
					if(!_eglInitialize((EGLDisplay)dpy3D, &eglMajor, &eglMinor))
						THROW("Could not initialize EGL");
					if(!(exts = _eglQueryString((EGLDisplay)dpy3D, EGL_EXTENSIONS)))
						THROW("Could not query EGL extensions");
					if(!strstr(exts, "EGL_KHR_no_config_context"))
						THROW("EGL_KHR_no_config_context extension not available");
					if(!strstr(exts, "EGL_KHR_surfaceless_context"))
						THROW("EGL_KHR_surfaceless_context extension not available");
				}
				catch(...)
				{
					if(devices) free(devices);
					throw;
				}
			}
			else  // fconfig.egl
			{
				if(fconfig.verbose)
					vglout.println("[VGL] Opening connection to 3D X server %s",
						strlen(fconfig.localdpystring) > 0 ?
						fconfig.localdpystring : "(default)");
				if((dpy3D = _XOpenDisplay(fconfig.localdpystring)) == NULL)
				{
					vglout.print("[VGL] ERROR: Could not open display %s.\n",
						fconfig.localdpystring);
					safeExit(1);
					return NULL;
				}
			}
		}
	}

	return dpy3D;
}


bool isDisplayStringExcluded(char *name)
{
	fconfig_reloadenv();

	char *dpyList = strdup(fconfig.excludeddpys);
	char *excluded = strtok(dpyList, ", \t");
	while(excluded)
	{
		if(!strcasecmp(name, excluded))
		{
			free(dpyList);  return true;
		}
		excluded = strtok(NULL, ", \t");
	}
	free(dpyList);
	return false;
}


extern "C" {

int deleteCS(XExtData *extData)
{
	if(extData) delete (util::CriticalSection *)extData->private_data;
	return 0;
}

}

}  // namespace


extern "C" {

// This is the "real" version of dlopen(), which is called by the interposed
// version of dlopen() in libdlfaker.  Can't recall why this is here and not
// in dlfaker, but it seems like there was a good reason.

void *_vgl_dlopen(const char *file, int mode)
{
	if(!__dlopen)
	{
		faker::GlobalCriticalSection::SafeLock l(globalMutex);
		if(!__dlopen)
		{
			dlerror();  // Clear error state
			__dlopen = (_dlopenType)dlsym(RTLD_NEXT, "dlopen");
			char *err = dlerror();
			if(!__dlopen)
			{
				vglout.print("[VGL] ERROR: Could not load function \"dlopen\"\n");
				if(err) vglout.print("[VGL]    %s\n", err);
				faker::safeExit(1);
			}
		}
	}
	return __dlopen(file, mode);
}


int _vgl_getAutotestColor(Display *dpy, Drawable d, int right)
{
	if(faker::getAutotestDisplay() == dpy
		&& faker::getAutotestDrawable() == (long)d)
		return right ? faker::getAutotestRColor() : faker::getAutotestColor();

	return -1;
}


int _vgl_getAutotestFrame(Display *dpy, Drawable d)
{
	if(faker::getAutotestDisplay() == dpy
		&& faker::getAutotestDrawable() == (long)d)
		return faker::getAutotestFrame();

	return -1;
}

// These functions allow image transport plugins or applications to temporarily
// disable/re-enable the faker on a per-thread basis.
void _vgl_disableFaker(void)
{
	faker::setFakerLevel(faker::getFakerLevel() + 1);
	faker::setGLXExcludeCurrent(true);
	faker::setEGLExcludeCurrent(true);
	faker::setOGLExcludeCurrent(true);
}

void _vgl_enableFaker(void)
{
	faker::setFakerLevel(faker::getFakerLevel() - 1);
	faker::setGLXExcludeCurrent(false);
	faker::setEGLExcludeCurrent(false);
	faker::setOGLExcludeCurrent(false);
}

}
