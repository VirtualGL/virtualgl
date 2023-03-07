// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2018, 2021-2023 D. R. Commander
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

#include "faker-sym.h"
#include "EGLXDisplayHash.h"
#include "EGLXWindowHash.h"


extern void setWMAtom(Display *dpy, Window win, faker::VirtualWin *vw);


static INLINE EGLint EGLConfigID(faker::EGLXDisplay *eglxdpy, EGLConfig c)
{
	EGLint id;
	return (c && eglxdpy
		&& _eglGetConfigAttrib(eglxdpy->edpy, c, EGL_CONFIG_ID, &id) ? id : 0);
}

#define PRARGALEGL(a)  if(a != NULL) \
{ \
	vglout.print(#a "=["); \
	for(int __an = 0; a[__an] != EGL_NONE && __an < MAX_ATTRIBS; __an += 2) \
	{ \
		vglout.print("0x%.4X=0x%.4X ", a[__an], a[__an + 1]); \
	} \
	vglout.print("] "); \
}

#define PRARGEC(eglxdpy, a) \
	vglout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a, \
		EGLConfigID(eglxdpy, a))

#define GET_DISPLAY() \
	faker::EGLXDisplay *eglxdpy = (faker::EGLXDisplay *)display; \
	display = eglxdpy->edpy;

#define GET_DISPLAY_INIT(notInitError) \
	GET_DISPLAY() \
	if(!eglxdpy->isInit) \
	{ \
		faker::setEGLError(notInitError);  goto bailout; \
	}

#define IS_EXCLUDED_EGLX(display) \
	(faker::deadYet || faker::getFakerLevel() > 0 || !eglxdpyhash.find(display))

#define WRAP_DISPLAY() \
	if(!IS_EXCLUDED_EGLX(display)) \
	{ \
		GET_DISPLAY(); \
	}

#define WRAP_DISPLAY_INIT(notInitError) \
	if(!IS_EXCLUDED_EGLX(display)) \
	{ \
		GET_DISPLAY_INIT(notInitError); \
	}


static XVisualInfo *getVisual(Display *dpy, int screen, int depth, int c_class)
{
	XVisualInfo vtemp;  int nv = 0;

	if(!dpy) return NULL;

	vtemp.depth = depth;
	vtemp.c_class = c_class;
	vtemp.screen = screen;

	return XGetVisualInfo(dpy,
		VisualDepthMask | VisualClassMask | VisualScreenMask, &vtemp, &nv);
}


static XVisualInfo *getVisualFromConfig(faker::EGLXDisplay *eglxdpy,
	EGLConfig config)
{
	if(!eglxdpy || !config) return NULL;

	EGLint redSize, greenSize, blueSize;
	int depth = 24;

	if(_eglGetConfigAttrib(eglxdpy->edpy, config, EGL_RED_SIZE, &redSize)
		&& _eglGetConfigAttrib(eglxdpy->edpy, config, EGL_GREEN_SIZE, &greenSize)
		&& _eglGetConfigAttrib(eglxdpy->edpy, config, EGL_BLUE_SIZE, &blueSize)
		&& redSize == 10 && greenSize == 10 && blueSize == 10)
		depth = 30;

	return getVisual(eglxdpy->x11dpy, eglxdpy->screen, depth, TrueColor);
}


// Interposed EGL functions

extern "C" {

EGLBoolean eglBindTexImage(EGLDisplay display, EGLSurface surface,
	EGLint buffer)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = surface;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglBindTexImage(display, surface, buffer);

	GET_DISPLAY();
	DISABLE_FAKER();

	faker::EGLXVirtualWin *eglxvw = eglxwinhash.find(eglxdpy, surface);
	if(eglxvw) actualSurface = (EGLSurface)eglxvw->getGLXDrawable();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglBindTexImage);  PRARGX(display);  PRARGX(surface);
	if(surface != actualSurface) PRARGX(actualSurface);
	PRARGI(buffer);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	retval = _eglBindTexImage(display, actualSurface, buffer);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


EGLBoolean eglChooseConfig(EGLDisplay display, const EGLint *attrib_list,
	EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	EGLBoolean retval = EGL_FALSE;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglChooseConfig(display, attrib_list, configs, config_size,
			num_config);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglChooseConfig);  PRARGX(display);  PRARGALEGL(attrib_list);
	PRARGX(configs);  PRARGI(config_size);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	EGLint attribs[MAX_ATTRIBS + 1];  int j = 0;
	EGLint nativeRenderable = -1, samples = -1, surfaceType = -1, visualID = -1,
		visualType = -1;

	if(attrib_list)
	{
		for(EGLint i = 0; attrib_list[i] != EGL_NONE && i < MAX_ATTRIBS - 4;
			i += 2)
		{
			if(attrib_list[i] == EGL_NATIVE_RENDERABLE
				&& attrib_list[i + 1] != EGL_DONT_CARE)
				nativeRenderable = attrib_list[i + 1];
			else if(attrib_list[i] == EGL_SAMPLES
				&& attrib_list[i + 1] != EGL_DONT_CARE)
				samples = attrib_list[i + 1];
			else if(attrib_list[i] == EGL_SURFACE_TYPE
				&& attrib_list[i + 1] != EGL_DONT_CARE)
				surfaceType = attrib_list[i + 1];
			else if(attrib_list[i] == EGL_NATIVE_VISUAL_ID
				&& attrib_list[i + 1] != EGL_DONT_CARE)
				visualID = attrib_list[i + 1];
			else if(attrib_list[i] == EGL_NATIVE_VISUAL_TYPE
				&& attrib_list[i + 1] != EGL_DONT_CARE)
				visualType = attrib_list[i + 1];
			else
			{
				attribs[j++] = attrib_list[i];
				attribs[j++] = attrib_list[i + 1];
			}
		}
	}
	attribs[j++] = EGL_SURFACE_TYPE;
	if(surfaceType >= 0)
		attribs[j++] = surfaceType &
			~(EGL_WINDOW_BIT | EGL_PIXMAP_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT);
	else
		attribs[j++] = EGL_PBUFFER_BIT;
	if(fconfig.samples >= 0) samples = fconfig.samples;
	if(samples >= 0)
	{
		attribs[j++] = EGL_SAMPLES;
		attribs[j++] = samples;
	}
	attribs[j] = EGL_NONE;

	if(surfaceType == -1) surfaceType = EGL_WINDOW_BIT;

	if(!num_config)
	{
		faker::setEGLError(EGL_BAD_PARAMETER);
		goto done;
	}

	if(surfaceType & EGL_PIXMAP_BIT && !fconfig.eglxIgnorePixmapBit)
	{
		retval = EGL_TRUE;
		*num_config = 0;
	}
	if(nativeRenderable == EGL_FALSE
		&& (surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT) || visualID >= 0
			|| visualType >= 0))
	{
		retval = EGL_TRUE;
		*num_config = 0;
	}
	if(nativeRenderable == EGL_TRUE
		&& (surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT)) == 0)
	{
		retval = EGL_TRUE;
		*num_config = 0;
	}
	if(visualID >= 0)
	{
		XVisualInfo *v =
			glxvisual::visualFromID(eglxdpy->x11dpy, eglxdpy->screen, visualID);
		if(!v || (surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT)) == 0)
		{
			retval = EGL_TRUE;
			*num_config = 0;
		}
		if(v) _XFree(v);
	}
	if(visualType >= 0
		&& ((visualType != TrueColor && visualType != GLX_TRUE_COLOR)
			|| (surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT)) == 0))
	{
		retval = EGL_TRUE;
		*num_config = 0;
	}

	if(!retval)
	{
		EGLint nc = 0;

		retval = _eglChooseConfig(display, attribs, NULL, config_size, &nc);
		if(!retval || !nc)
		{
			*num_config = nc;
			goto done;
		}

		EGLConfig *newConfigs = new EGLConfig[nc];
		EGLint ncOrig = nc;

		retval = _eglChooseConfig(display, attribs, newConfigs, ncOrig, &nc);
		if(!retval || !nc)
		{
			delete [] newConfigs;
			*num_config = nc;
			goto done;
		}

		*num_config = 0;
		j = 0;
		for(EGLint i = 0; i < nc; i++)
		{
			XVisualInfo *v = getVisualFromConfig(eglxdpy, newConfigs[i]);
			if(!v && (nativeRenderable == EGL_TRUE || visualID >= 0
					|| visualType >= 0
					|| (surfaceType & (EGL_WINDOW_BIT | EGL_PIXMAP_BIT))))
				continue;
			if(v) XFree(v);
			(*num_config)++;
			if(configs && config_size >= 1 && j < config_size)
				configs[j++] = newConfigs[i];
		}
		if(configs && config_size >= 1)
			*num_config = min(*num_config, config_size);
		delete [] newConfigs;
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(num_config)
	{
		if(configs && *num_config)
			for(EGLint i = 0; i < *num_config; i++)
				vglout.print("configs[%d]=0x%.8lx(0x%.2x) ", i,
					(unsigned long)configs[i], EGLConfigID(eglxdpy, configs[i]));
		PRARGI(*num_config);
	}
	else PRARGX(num_config);
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLint eglClientWaitSync(EGLDisplay display, EGLSync sync, EGLint flags,
	EGLTime timeout)
{
	WRAP_DISPLAY();
	return _eglClientWaitSync(display, sync, flags, timeout);
}

EGLint eglClientWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags,
	EGLTimeKHR timeout)
{
	WRAP_DISPLAY();
	return _eglClientWaitSyncKHR(display, sync, flags, timeout);
}


EGLImage eglCreateImage(EGLDisplay display, EGLContext context, EGLenum target,
	EGLClientBuffer buffer, const EGLAttrib *attrib_list)
{
	WRAP_DISPLAY();
	return _eglCreateImage(display, context, target, buffer, attrib_list);
}

EGLImageKHR eglCreateImageKHR(EGLDisplay display, EGLContext context,
	EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	WRAP_DISPLAY();
	return _eglCreateImageKHR(display, context, target, buffer, attrib_list);
}


EGLBoolean eglCopyBuffers(EGLDisplay display, EGLSurface surface,
	NativePixmapType native_pixmap)
{
	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglCopyBuffers(display, surface, native_pixmap);

	faker::setEGLError(EGL_BAD_NATIVE_PIXMAP);

	CATCH();
	return EGL_FALSE;
}


EGLContext eglCreateContext(EGLDisplay display, EGLConfig config,
	EGLContext share_context, const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreateContext(display, config, share_context, attrib_list);
	bailout:
	return EGL_NO_CONTEXT;
}


EGLSurface eglCreatePixmapSurface(EGLDisplay display, EGLConfig config,
	EGLNativePixmapType native_pixmap, const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreatePixmapSurface(display, config, native_pixmap, attrib_list);
	bailout:
	return EGL_NO_SURFACE;
}

EGLSurface eglCreatePlatformPixmapSurface(EGLDisplay display, EGLConfig config,
	void *native_pixmap, const EGLAttrib *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreatePlatformPixmapSurface(display, config, native_pixmap,
		attrib_list);
	bailout:
	return EGL_NO_SURFACE;
}

EGLSurface eglCreatePlatformPixmapSurfaceEXT(EGLDisplay display,
	EGLConfig config, void *native_pixmap, const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreatePlatformPixmapSurfaceEXT(display, config, native_pixmap,
		attrib_list);
	bailout:
	return EGL_NO_SURFACE;
}


EGLSync eglCreateSync(EGLDisplay display, EGLenum type,
	const EGLAttrib *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_BAD_DISPLAY);
	return _eglCreateSync(display, type, attrib_list);
	bailout:
	return EGL_NO_SYNC;
}

EGLSyncKHR eglCreateSyncKHR(EGLDisplay display, EGLenum type,
	const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_BAD_DISPLAY);
	return _eglCreateSyncKHR(display, type, attrib_list);
	bailout:
	return EGL_NO_SYNC_KHR;
}


EGLSyncKHR eglCreateSync64KHR(EGLDisplay display, EGLenum type,
	const EGLAttribKHR *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_BAD_DISPLAY);
	return _eglCreateSync64KHR(display, type, attrib_list);
	bailout:
	return EGL_NO_SYNC_KHR;
}


EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay display,
	EGLenum buftype, EGLClientBuffer buffer, EGLConfig config,
	const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreatePbufferFromClientBuffer(display, buftype, buffer, config,
		attrib_list);
	bailout:
	return EGL_NO_SURFACE;
}


EGLSurface eglCreatePbufferSurface(EGLDisplay display, EGLConfig config,
	const EGLint *attrib_list)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglCreatePbufferSurface(display, config, attrib_list);
	bailout:
	return EGL_NO_SURFACE;
}


EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config,
	NativeWindowType native_window, const EGLint *attrib_list)
{
	EGLSurface surface = 0, actualSurface = 0;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglCreateWindowSurface(display, config, native_window,
			attrib_list);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglCreateWindowSurface);  PRARGX(display);
	PRARGEC(eglxdpy, config);  PRARGX(native_window);  PRARGALEGL(attrib_list);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(attrib_list)
	{
		for(int i = 0; i < MAX_ATTRIBS && attrib_list[i] != EGL_NONE; i += 2)
		{
			if(attrib_list[i] == EGL_LARGEST_PBUFFER || attrib_list[i] == EGL_WIDTH
				|| attrib_list[i] == EGL_HEIGHT)
			{
				faker::setEGLError(EGL_BAD_ATTRIBUTE);
				goto done;
			}
		}
	}

	if(!config)
		faker::setEGLError(EGL_BAD_CONFIG);
	else if(!native_window)
		faker::setEGLError(EGL_BAD_NATIVE_WINDOW);
	else
	{
		faker::EGLXVirtualWin *eglxvw =
			eglxwinhash.find(eglxdpy->x11dpy, native_window);
		if(eglxvw)
			faker::setEGLError(EGL_BAD_ALLOC);
		else
		{
			eglxvw = new faker::EGLXVirtualWin(eglxdpy->x11dpy, native_window,
				display, config, attrib_list);
			surface = eglxvw->getDummySurface();
			actualSurface = (EGLSurface)eglxvw->getGLXDrawable();
			eglxwinhash.add(eglxdpy, surface, eglxvw);
		}
	}

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(surface);  if(actualSurface) PRARGX(actualSurface);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return surface;
}


EGLSurface eglCreatePlatformWindowSurface(EGLDisplay display, EGLConfig config,
	void *native_window, const EGLAttrib *attrib_list)
{
	EGLint attribs[MAX_ATTRIBS + 1];
	int j = 0;

	if(!native_window)
	{
		faker::setEGLError(EGL_BAD_NATIVE_WINDOW);
		return 0;
	}

	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != EGL_NONE && i < MAX_ATTRIBS; i += 2)
		{
			attribs[j++] = attrib_list[i];  attribs[j++] = attrib_list[i + 1];
		}
	}
	attribs[j] = EGL_NONE;

	NativeWindowType *native_window_ptr = (NativeWindowType *)native_window;
	return eglCreateWindowSurface(display, config, *native_window_ptr, attribs);
}

EGLSurface eglCreatePlatformWindowSurfaceEXT(EGLDisplay display,
	EGLConfig config, void *native_window, const EGLint *attrib_list)
{
	if(!native_window)
	{
		faker::setEGLError(EGL_BAD_NATIVE_WINDOW);
		return 0;
	}

	NativeWindowType *native_window_ptr = (NativeWindowType *)native_window;
	return eglCreateWindowSurface(display, config, *native_window_ptr,
		attrib_list);
}


EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglDestroyContext(display, context);
	bailout:
	return EGL_FALSE;
}


EGLBoolean eglDestroyImage(EGLDisplay display, EGLImage image)
{
	WRAP_DISPLAY();
	return _eglDestroyImage(display, image);
}

EGLBoolean eglDestroyImageKHR(EGLDisplay display, EGLImageKHR image)
{
	WRAP_DISPLAY();
	return _eglDestroyImageKHR(display, image);
}


EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = 0;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglDestroySurface(display, surface);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglDestroySurface);  PRARGX(display);  PRARGX(surface);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::EGLXVirtualWin *eglxvw = eglxwinhash.find(eglxdpy, surface);
	if(eglxvw)
	{
		actualSurface = (EGLSurface)eglxvw->getGLXDrawable();
		eglxwinhash.remove(eglxdpy, surface);
		retval = EGL_TRUE;
	}
	else retval = _eglDestroySurface(display, surface);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(actualSurface) PRARGX(actualSurface);
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglDestroySync(EGLDisplay display, EGLSync sync)
{
	WRAP_DISPLAY();
	return _eglDestroySync(display, sync);
}

EGLBoolean eglDestroySyncKHR(EGLDisplay display, EGLSyncKHR sync)
{
	WRAP_DISPLAY();
	return _eglDestroySyncKHR(display, sync);
}


EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config,
	EGLint attribute, EGLint *value)
{
	EGLBoolean retval = EGL_FALSE;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglGetConfigAttrib(display, config, attribute, value);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetConfigAttrib);  PRARGX(display);  PRARGEC(eglxdpy, config);
	PRARGX(attribute);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(config && value)
	{
		XVisualInfo *v = getVisualFromConfig(eglxdpy, config);

		if(attribute == EGL_NATIVE_RENDERABLE)
		{
			*value = v ? EGL_TRUE : EGL_FALSE;
			retval = EGL_TRUE;
		}
		else if(attribute == EGL_NATIVE_VISUAL_ID)
		{
			*value = v ? v->visualid : 0;
			retval = EGL_TRUE;
		}
		else if(attribute == EGL_NATIVE_VISUAL_TYPE)
		{
			*value = v ? TrueColor : 0;
			retval = EGL_TRUE;
		}
		if(v) _XFree(v);
	}

	if(!retval)
		retval = _eglGetConfigAttrib(display, config, attribute, value);

	if(value && attribute == EGL_SURFACE_TYPE && *value & EGL_PBUFFER_BIT)
	{
		*value |= EGL_WINDOW_BIT;
		*value &= ~EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(value) { PRARGIX(*value); }  else { PRARGX(value); }
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig *configs,
	EGLint config_size, EGLint *num_config)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglGetConfigs(display, configs, config_size, num_config);
	bailout:
	return EGL_FALSE;
}


EGLDisplay eglGetCurrentDisplay(void)
{
	EGLDisplay display = 0;

	TRY();

	if(faker::getEGLExcludeCurrent() || !faker::getEGLXContextCurrent())
		return _eglGetCurrentDisplay();

	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetCurrentDisplay);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	display = (EGLDisplay)faker::getCurrentEGLXDisplay();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(display);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return display;
}


EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
	EGLSurface surface = 0, actualSurface = 0;

	TRY();

	if(faker::getEGLExcludeCurrent() || !faker::getEGLXContextCurrent())
		return _eglGetCurrentSurface(readdraw);

	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetCurrentSurface);  PRARGX(readdraw);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	actualSurface = surface = _eglGetCurrentSurface(readdraw);
	faker::EGLXVirtualWin *eglxvw =
		eglxwinhash.findInternal(faker::getCurrentEGLXDisplay(), actualSurface);
	if(eglxvw) surface = eglxvw->getDummySurface();

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(surface);
	if(surface != actualSurface) PRARGX(actualSurface);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return surface;
}


#define CREATE_DISPLAY() \
{ \
	bool isDefault = false; \
	if(native_display != EGL_DEFAULT_DISPLAY) \
	{ \
		if(screen < 0) screen = DefaultScreen(native_display); \
		eglxdpy = eglxdpyhash.find((Display *)native_display, screen); \
	} \
	if(eglxdpy == NULL) \
	{ \
		if(native_display == EGL_DEFAULT_DISPLAY) \
		{ \
			native_display = _XOpenDisplay(NULL); \
			isDefault = true; \
		} \
		if(native_display) \
		{ \
			if(screen < 0) screen = DefaultScreen(native_display); \
			if(screen >= ScreenCount(native_display)) \
			{ \
				faker::setEGLError(EGL_BAD_ATTRIBUTE); \
				goto done; \
			} \
			eglxdpy = new faker::EGLXDisplay; \
			eglxdpy->edpy = EDPY; \
			eglxdpy->x11dpy = (Display *)native_display; \
			eglxdpy->screen = screen; \
			eglxdpy->isDefault = isDefault; \
			eglxdpy->isInit = false; \
			eglxdpyhash.add((Display *)native_display, screen, eglxdpy); \
		} \
	} \
	display = (EGLDisplay)eglxdpy; \
}


EGLDisplay eglGetDisplay(EGLNativeDisplayType native_display)
{
	faker::EGLXDisplay *eglxdpy = NULL;
	EGLDisplay display = EGL_NO_DISPLAY;
	int screen = -1;

	TRY();

	if(native_display != EGL_DEFAULT_DISPLAY
		&& faker::isDisplayExcluded((Display *)native_display))
		return _eglGetDisplay(native_display);

	if(!fconfig.egl)
	{
		static bool alreadyWarned = false;
		if(!alreadyWarned)
		{
			if(fconfig.verbose)
				vglout.print("[VGL] WARNING: EGL/X11 emulation requires the EGL back end\n");
			alreadyWarned = true;
		}
		return display;
	}

	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetDisplay);  PRARGD(native_display);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	CREATE_DISPLAY()

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(display);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return (EGLDisplay)display;
}


EGLDisplay eglGetPlatformDisplay(EGLenum platform, void *native_display,
	const EGLAttrib *attrib_list)
{
	faker::EGLXDisplay *eglxdpy = NULL;
	EGLDisplay display = EGL_NO_DISPLAY;
	int screen = -1;

	TRY();

	if(platform != EGL_PLATFORM_X11_EXT
		|| (native_display != EGL_DEFAULT_DISPLAY
				&& faker::isDisplayExcluded((Display *)native_display)))
		return _eglGetPlatformDisplay(platform, native_display, attrib_list);

	if(!fconfig.egl)
	{
		static bool alreadyWarned = false;
		if(!alreadyWarned)
		{
			if(fconfig.verbose)
				vglout.print("[VGL] WARNING: EGL/X11 emulation requires the EGL back end\n");
			alreadyWarned = true;
		}
		return display;
	}

	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetPlatformDisplay);  PRARGX(platform);  PRARGD(native_display);
	PRARGALEGL(attrib_list);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(attrib_list)
	{
		for(int i = 0; i < MAX_ATTRIBS && attrib_list[i] != EGL_NONE; i += 2)
		{
			if(attrib_list[i] == EGL_PLATFORM_X11_SCREEN_EXT)
				screen = attrib_list[i + 1];
		}
	}

	CREATE_DISPLAY()

	done:
	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGX(display);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return (EGLDisplay)display;
}

EGLDisplay eglGetPlatformDisplayEXT(EGLenum platform, void *native_display,
	const EGLint *attrib_list)
{
	EGLAttrib attribs[MAX_ATTRIBS + 1];
	int j = 0;

	if(attrib_list)
	{
		for(int i = 0; attrib_list[i] != EGL_NONE && i < MAX_ATTRIBS; i += 2)
		{
			attribs[j++] = attrib_list[i];  attribs[j++] = attrib_list[i + 1];
		}
	}
	attribs[j] = EGL_NONE;

	return eglGetPlatformDisplay(platform, native_display, attribs);
}


EGLint eglGetError(void)
{
	EGLint error = faker::getEGLError();

	if(error != EGL_SUCCESS)
		faker::setEGLError(EGL_SUCCESS);
	else
		error = _eglGetError();

	return error;
}


// If an application uses eglGetProcAddress() to obtain the address of a
// function that we're interposing, we need to return the address of the
// interposed function.

#define CHECK_FAKED(f) \
	if(!strcmp((char *)procName, #f)) \
	{ \
		retval = (void (*)(void))f; \
		if(fconfig.trace) vglout.print("[INTERPOSED]"); \
	}

void (*eglGetProcAddress(const char *procName))(void)
{
	void (*retval)(void) = NULL;

	faker::init();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglGetProcAddress);  PRARGS((char *)procName);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(procName)
	{
		// EGL 1.0
		CHECK_FAKED(eglChooseConfig);
		CHECK_FAKED(eglCopyBuffers);
		CHECK_FAKED(eglCreateContext);
		CHECK_FAKED(eglCreatePbufferSurface);
		CHECK_FAKED(eglCreatePixmapSurface);
		CHECK_FAKED(eglCreateWindowSurface);
		CHECK_FAKED(eglDestroyContext);
		CHECK_FAKED(eglDestroySurface);
		CHECK_FAKED(eglGetConfigAttrib);
		CHECK_FAKED(eglGetConfigs);
		CHECK_FAKED(eglGetCurrentDisplay);
		CHECK_FAKED(eglGetCurrentSurface);
		CHECK_FAKED(eglGetDisplay);
		CHECK_FAKED(eglGetError);
		CHECK_FAKED(eglGetProcAddress);
		CHECK_FAKED(eglInitialize);
		CHECK_FAKED(eglMakeCurrent);
		CHECK_FAKED(eglQueryContext);
		CHECK_FAKED(eglQueryString);
		CHECK_FAKED(eglQuerySurface);
		CHECK_FAKED(eglSwapBuffers);
		CHECK_FAKED(eglTerminate);

		// EGL 1.1
		CHECK_FAKED(eglBindTexImage);
		CHECK_FAKED(eglReleaseTexImage);
		CHECK_FAKED(eglSurfaceAttrib);
		CHECK_FAKED(eglSwapInterval);

		// EGL 1.2
		CHECK_FAKED(eglCreatePbufferFromClientBuffer);

		// EGL 1.5
		CHECK_FAKED(eglClientWaitSync);
		CHECK_FAKED(eglCreateImage);
		CHECK_FAKED(eglCreatePlatformPixmapSurface);
		CHECK_FAKED(eglCreatePlatformWindowSurface);
		CHECK_FAKED(eglCreateSync);
		CHECK_FAKED(eglDestroyImage);
		CHECK_FAKED(eglDestroySync);
		CHECK_FAKED(eglGetPlatformDisplay);
		CHECK_FAKED(eglGetSyncAttrib);
		CHECK_FAKED(eglWaitSync);

		// EGL_EXT_device_query
		CHECK_FAKED(eglQueryDisplayAttribEXT);

		// EGL_EXT_platform_base
		CHECK_FAKED(eglCreatePlatformPixmapSurfaceEXT);
		CHECK_FAKED(eglCreatePlatformWindowSurfaceEXT);
		CHECK_FAKED(eglGetPlatformDisplayEXT);

		// EGL_KHR_cl_event2
		CHECK_FAKED(eglCreateSync64KHR);

		// EGL_KHR_fence_sync
		CHECK_FAKED(eglClientWaitSyncKHR);
		CHECK_FAKED(eglCreateSyncKHR);
		CHECK_FAKED(eglDestroySyncKHR);
		CHECK_FAKED(eglGetSyncAttribKHR);

		// EGL_KHR_image
		CHECK_FAKED(eglCreateImageKHR);
		CHECK_FAKED(eglDestroyImageKHR);

		// EGL_KHR_reusable_sync
		CHECK_FAKED(eglSignalSyncKHR);

		// EGL_KHR_wait_sync
		CHECK_FAKED(eglWaitSyncKHR);

		// OpenGL
		CHECK_FAKED(glBindFramebuffer)
		CHECK_FAKED(glBindFramebufferEXT)
		CHECK_FAKED(glDeleteFramebuffers)
		CHECK_FAKED(glDeleteFramebuffersEXT)
		CHECK_FAKED(glFinish)
		CHECK_FAKED(glFlush)
		CHECK_FAKED(glDrawBuffer)
		CHECK_FAKED(glDrawBuffers)
		CHECK_FAKED(glDrawBuffersARB)
		CHECK_FAKED(glDrawBuffersATI)
		CHECK_FAKED(glFramebufferDrawBufferEXT)
		CHECK_FAKED(glFramebufferDrawBuffersEXT)
		CHECK_FAKED(glFramebufferReadBufferEXT)
		CHECK_FAKED(glGetBooleanv)
		CHECK_FAKED(glGetDoublev)
		CHECK_FAKED(glGetFloatv)
		CHECK_FAKED(glGetFramebufferAttachmentParameteriv)
		CHECK_FAKED(glGetFramebufferParameteriv)
		CHECK_FAKED(glGetIntegerv)
		CHECK_FAKED(glGetInteger64v)
		CHECK_FAKED(glGetNamedFramebufferParameteriv)
		CHECK_FAKED(glGetString)
		CHECK_FAKED(glGetStringi)
		CHECK_FAKED(glNamedFramebufferDrawBuffer)
		CHECK_FAKED(glNamedFramebufferDrawBuffers)
		CHECK_FAKED(glNamedFramebufferReadBuffer)
		CHECK_FAKED(glPopAttrib)
		CHECK_FAKED(glReadBuffer)
		CHECK_FAKED(glReadPixels)
		CHECK_FAKED(glViewport)
	}
	if(!retval)
	{
		// GL_EXT_x11_sync_object does not currently work with VirtualGL.
		if(!strcmp((char *)procName, "glImportSyncEXT"))
		{
			if(fconfig.trace) vglout.print("[NOT IMPLEMENTED]");
			retval = NULL;
		}
		else
		{
			if(fconfig.trace) vglout.print("[passed through]");
			retval = _eglGetProcAddress(procName);
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	return retval;
}


EGLBoolean eglGetSyncAttrib(EGLDisplay display, EGLSync sync, EGLint attribute,
	EGLAttrib *value)
{
	WRAP_DISPLAY();
	return _eglGetSyncAttrib(display, sync, attribute, value);
}

EGLBoolean eglGetSyncAttribKHR(EGLDisplay display, EGLSyncKHR sync,
	EGLint attribute, EGLint *value)
{
	WRAP_DISPLAY();
	return _eglGetSyncAttribKHR(display, sync, attribute, value);
}


EGLBoolean eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor)
{
	EGLBoolean retval = EGL_FALSE;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglInitialize(display, major, minor);

	GET_DISPLAY();
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglInitialize);  PRARGX(display);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	eglxdpy->isInit = true;
	retval = EGL_TRUE;
	if(major) *major = 1;
	if(minor) *minor = 5;

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(major) PRARGI(*major);  if(minor) PRARGI(*minor);
	CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw, EGLSurface read,
	EGLContext context)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualDraw = draw, actualRead = read;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
	{
		faker::setEGLExcludeCurrent(true);
		faker::setOGLExcludeCurrent(true);
		faker::setEGLXContextCurrent(false);
		faker::setCurrentEGLXDisplay(NULL);
		return _eglMakeCurrent(display, draw, read, context);
	}
	faker::setEGLExcludeCurrent(false);
	faker::setOGLExcludeCurrent(false);

	GET_DISPLAY()
	if(!eglxdpy->isInit && context)
	{
		faker::setEGLError(EGL_NOT_INITIALIZED);  goto bailout; \
	}
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglMakeCurrent);  PRARGX(display);  PRARGX(draw);  PRARGX(read);
	PRARGX(context);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::EGLXVirtualWin *drawEGLXVW = eglxwinhash.find(eglxdpy, draw);
	if(drawEGLXVW)
	{
		actualDraw = (EGLSurface)drawEGLXVW->updateGLXDrawable();
		setWMAtom(drawEGLXVW->getX11Display(), drawEGLXVW->getX11Drawable(),
			drawEGLXVW);
	}
	faker::EGLXVirtualWin *readEGLXVW = eglxwinhash.find(eglxdpy, read);
	if(readEGLXVW)
	{
		actualRead = (EGLSurface)readEGLXVW->updateGLXDrawable();
		if(readEGLXVW != drawEGLXVW)
			setWMAtom(readEGLXVW->getX11Display(), readEGLXVW->getX11Drawable(),
				readEGLXVW);
	}
	retval = _eglMakeCurrent(display, actualDraw, actualRead, context);

	if((drawEGLXVW = eglxwinhash.findInternal(eglxdpy, actualDraw)) != NULL)
	{
		drawEGLXVW->clear();  drawEGLXVW->cleanup();
	}
	if((readEGLXVW = eglxwinhash.findInternal(eglxdpy, actualRead)) != NULL)
		readEGLXVW->cleanup();

	if(context && retval)
	{
		// Disable the OpenGL interposer
		faker::setEGLXContextCurrent(true);
		faker::setCurrentEGLXDisplay(eglxdpy);
	}
	else
	{
		faker::setEGLXContextCurrent(false);
		faker::setCurrentEGLXDisplay(NULL);
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(draw != actualDraw) PRARGX(actualDraw);
	if(read != actualRead) PRARGX(actualRead);
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglQueryContext(EGLDisplay display, EGLContext context,
	EGLint attribute, EGLint *value)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglQueryContext(display, context, attribute, value);
	bailout:
	return EGL_FALSE;
}


EGLBoolean eglQueryDisplayAttribEXT(EGLDisplay display, EGLint attribute,
	EGLAttrib *value)
{
	WRAP_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	return _eglQueryDisplayAttribEXT(display, attribute, value);
	bailout:
	return EGL_FALSE;
}


static char eglExtensions[2048] = { 0 };

#define ADD_EXTENSION(ext) \
	if(strstr(retval, #ext) && !strstr(eglExtensions, #ext)) \
		strncat(eglExtensions, #ext " ", 2047 - strlen(eglExtensions));

const char *eglQueryString(EGLDisplay display, EGLint name)
{
	const char *retval = NULL;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglQueryString(display, name);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglQueryString);  PRARGX(display);  PRARGX(name);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(name == EGL_VENDOR)
		retval = "VirtualGL";
	else if(name == EGL_VERSION)
		retval = "1.5";
	else
		retval = _eglQueryString(display, name);

	if(name == EGL_EXTENSIONS && retval)
	{
		faker::GlobalCriticalSection::SafeLock l(globalMutex);

		// These extensions should work if the underlying EGL implementation
		// supports them.
		ADD_EXTENSION(EGL_ARM_image_format);
		ADD_EXTENSION(EGL_ARM_implicit_external_sync);
		ADD_EXTENSION(EGL_EXT_bind_to_front);
		ADD_EXTENSION(EGL_EXT_buffer_age);
		ADD_EXTENSION(EGL_EXT_client_extensions);
		ADD_EXTENSION(EGL_EXT_create_context_robustness);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_bt2020_linear);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_bt2020_pq);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_display_p3);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_display_p3_linear);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_display_p3_passthrough);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_scrgb);
		ADD_EXTENSION(EGL_EXT_gl_colorspace_scrgb_linear);
		ADD_EXTENSION(EGL_EXT_image_dma_buf_import);
		ADD_EXTENSION(EGL_EXT_image_gl_colorspace);
		ADD_EXTENSION(EGL_EXT_image_implicit_sync_control);
		ADD_EXTENSION(EGL_EXT_pixel_format_float);
		ADD_EXTENSION(EGL_EXT_platform_base);
		ADD_EXTENSION(EGL_EXT_platform_x11);
		ADD_EXTENSION(EGL_EXT_protected_surface);
		ADD_EXTENSION(EGL_EXT_surface_CTA861_3_metadata);
		ADD_EXTENSION(EGL_EXT_surface_SMPTE2086_metadata);
		ADD_EXTENSION(EGL_HI_colorformats);
		ADD_EXTENSION(EGL_IMG_context_priority);
		ADD_EXTENSION(EGL_KHR_cl_event);
		ADD_EXTENSION(EGL_KHR_cl_event2);
		ADD_EXTENSION(EGL_KHR_client_get_all_proc_addresses);
		ADD_EXTENSION(EGL_KHR_config_attribs);
		ADD_EXTENSION(EGL_KHR_context_flush_control);
		ADD_EXTENSION(EGL_KHR_create_context);
		ADD_EXTENSION(EGL_KHR_create_context_no_error);
		ADD_EXTENSION(EGL_KHR_fence_sync);
		ADD_EXTENSION(EGL_KHR_get_all_proc_addresses);
		ADD_EXTENSION(EGL_KHR_gl_colorspace);
		ADD_EXTENSION(EGL_KHR_gl_renderbuffer_image);
		ADD_EXTENSION(EGL_KHR_gl_texture_2D_image);
		ADD_EXTENSION(EGL_KHR_gl_texture_3D_image);
		ADD_EXTENSION(EGL_KHR_gl_texture_cubemap_image);
		ADD_EXTENSION(EGL_KHR_image);
		ADD_EXTENSION(EGL_KHR_image_base);
		ADD_EXTENSION(EGL_KHR_no_config_context);
		ADD_EXTENSION(EGL_KHR_platform_x11);
		ADD_EXTENSION(EGL_KHR_reusable_sync);
		ADD_EXTENSION(EGL_KHR_surfaceless_context);
		ADD_EXTENSION(EGL_KHR_vg_parent_image);
		ADD_EXTENSION(EGL_KHR_wait_sync);
		ADD_EXTENSION(EGL_NV_context_priority_realtime);
		ADD_EXTENSION(EGL_NV_coverage_sample);
		ADD_EXTENSION(EGL_NV_coverage_sample_resolve);
		ADD_EXTENSION(EGL_NV_cuda_event);
		ADD_EXTENSION(EGL_NV_depth_nonlinear);
		ADD_EXTENSION(EGL_NV_post_convert_rounding);
		ADD_EXTENSION(EGL_NV_robustness_video_memory_purge);
		ADD_EXTENSION(EGL_NV_system_time);
		ADD_EXTENSION(EGL_TIZEN_image_native_buffer);
		ADD_EXTENSION(EGL_TIZEN_image_native_surface);

		if(eglExtensions[strlen(eglExtensions) - 1] == ' ')
			eglExtensions[strlen(eglExtensions) - 1] = 0;

		retval = eglExtensions;
	}

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(retval) PRARGS(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglQuerySurface(EGLDisplay display, EGLSurface surface,
	EGLint attribute, EGLint *value)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = surface;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglQuerySurface(display, surface, attribute, value);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	faker::EGLXVirtualWin *eglxvw = eglxwinhash.find(eglxdpy, surface);
	if(eglxvw) actualSurface = (EGLSurface)eglxvw->getGLXDrawable();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglQuerySurface);  PRARGX(display);  PRARGX(surface);
	if(actualSurface != surface) PRARGX(actualSurface);
	PRARGX(attribute);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	if(eglxvw && attribute == EGL_LARGEST_PBUFFER)
		retval = EGL_TRUE;
	else if(eglxvw && attribute == EGL_SWAP_BEHAVIOR && value)
	{
		*value = EGL_BUFFER_DESTROYED;
		retval = EGL_TRUE;
	}
	else
		retval = _eglQuerySurface(display, actualSurface, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();
	if(value && retval) { PRARGIX(*value); } else { PRARGX(value); }
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglReleaseTexImage(EGLDisplay display, EGLSurface surface,
	EGLint buffer)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = surface;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglReleaseTexImage(display, surface, buffer);

	GET_DISPLAY();
	DISABLE_FAKER();

	faker::EGLXVirtualWin *eglxvw = eglxwinhash.find(eglxdpy, surface);
	if(eglxvw) actualSurface = (EGLSurface)eglxvw->getGLXDrawable();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglReleaseTexImage);  PRARGX(display);  PRARGX(surface);
	if(surface != actualSurface) PRARGX(actualSurface);
	PRARGI(buffer);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	retval = _eglReleaseTexImage(display, actualSurface, buffer);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


EGLBoolean eglSignalSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLenum mode)
{
	WRAP_DISPLAY();
	return _eglSignalSyncKHR(display, sync, mode);
}


EGLBoolean eglSurfaceAttrib(EGLDisplay display, EGLSurface surface,
	EGLint attribute, EGLint value)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = surface;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglSurfaceAttrib(display, surface, attribute, value);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	faker::EGLXVirtualWin *eglxvw = eglxwinhash.find(eglxdpy, surface);
	if(eglxvw) actualSurface = (EGLSurface)eglxvw->getGLXDrawable();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglSurfaceAttrib);  PRARGX(display);  PRARGX(surface);
	if(actualSurface != surface) PRARGX(actualSurface);
	PRARGX(attribute);  PRARGIX(value);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	retval = _eglSurfaceAttrib(display, actualSurface, attribute, value);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface)
{
	EGLBoolean retval = EGL_FALSE;
	EGLSurface actualSurface = 0;
	faker::EGLXVirtualWin *eglxvw = NULL;
	static util::Timer timer;  util::Timer sleepTimer;
	static double err = 0.;  static bool first = true;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglSwapBuffers(display, surface);

	GET_DISPLAY_INIT(EGL_NOT_INITIALIZED);
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglSwapBuffers);  PRARGX(display);  PRARGX(surface);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	fconfig.flushdelay = 0.;
	if((eglxvw = eglxwinhash.find(eglxdpy, surface)) != NULL)
	{
		actualSurface = (EGLSurface)eglxvw->getGLXDrawable();
		eglxvw->readback(GL_BACK, false, fconfig.sync);
		int interval = eglxvw->getSwapInterval();
		if(interval > 0)
		{
			double elapsed = timer.elapsed();
			if(first) first = false;
			else
			{
				double fps = fconfig.refreshrate / (double)interval;
				if(fps > 0.0 && elapsed < 1. / fps)
				{
					sleepTimer.start();
					long usec = (long)((1. / fps - elapsed - err) * 1000000.);
					if(usec > 0) usleep(usec);
					double sleepTime = sleepTimer.elapsed();
					err = sleepTime - (1. / fps - elapsed - err);  if(err < 0.) err = 0.;
				}
			}
			timer.start();
		}
		retval = EGL_TRUE;
	}
	else retval = _eglSwapBuffers(display, surface);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  if(actualSurface) PRARGX(actualSurface);
	PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	bailout:
	return retval;
}


EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval)
{
	EGLBoolean retval = EGL_FALSE;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
		return _eglSwapInterval(display, interval);

	GET_DISPLAY();
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglSwapInterval);  PRARGX(display);  PRARGI(interval);
	STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	faker::EGLXVirtualWin *eglxvw;
	EGLSurface draw = _eglGetCurrentSurface(EGL_DRAW);
	if(interval >= 0
		&& (eglxvw = eglxwinhash.findInternal(eglxdpy, draw)) != NULL)
	{
		eglxvw->setSwapInterval(interval);
		retval = EGL_TRUE;
	}
	else
		retval = _eglSwapInterval(display, interval);

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


EGLBoolean eglTerminate(EGLDisplay display)
{
	EGLBoolean retval = EGL_FALSE;

	TRY();

	if(IS_EXCLUDED_EGLX(display))
	{
		if(display == EDPY) return EGL_TRUE;
		return _eglTerminate(display);
	}

	GET_DISPLAY();
	DISABLE_FAKER();

	/////////////////////////////////////////////////////////////////////////////
	OPENTRACE(eglTerminate);  PRARGX(display);  STARTTRACE();
	/////////////////////////////////////////////////////////////////////////////

	eglxdpy->isInit = false;
	retval = EGL_TRUE;

	/////////////////////////////////////////////////////////////////////////////
	STOPTRACE();  PRARGI(retval);  CLOSETRACE();
	/////////////////////////////////////////////////////////////////////////////

	CATCH();
	ENABLE_FAKER();
	return retval;
}


EGLBoolean eglWaitSync(EGLDisplay display, EGLSync sync, EGLint flags)
{
	WRAP_DISPLAY();
	return _eglWaitSync(display, sync, flags);
}

EGLint eglWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags)
{
	WRAP_DISPLAY();
	return _eglWaitSyncKHR(display, sync, flags);
}


}  // extern "C"
