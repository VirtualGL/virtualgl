{
	global:
		glXChooseVisual;
		glXCopyContext;
		glXCreateContext;
		glXCreateGLXPixmap;
		glXDestroyContext;
		glXDestroyGLXPixmap;
		glXGetConfig;
		glXGetCurrentDrawable;
		glXIsDirect;
		glXMakeCurrent;
		glXQueryExtension;
		glXQueryVersion;
		glXSwapBuffers;
		glXUseXFont;
		glXWaitGL;

		glXGetClientString;
		glXQueryServerString;
		glXQueryExtensionsString;

		glXChooseFBConfig;
		glXCreateNewContext;
		glXCreatePbuffer;
		glXCreatePixmap;
		glXCreateWindow;
		glXDestroyPbuffer;
		glXDestroyPixmap;
		glXDestroyWindow;
		glXGetCurrentDisplay;
		glXGetCurrentReadDrawable;
		glXGetFBConfigAttrib;
		glXGetFBConfigs;
		glXGetSelectedEvent;
		glXGetVisualFromFBConfig;
		glXMakeContextCurrent;
		glXQueryContext;
		glXQueryDrawable;
		glXSelectEvent;

		glXChooseFBConfigSGIX;
		glXCreateContextWithConfigSGIX;
		glXCreateGLXPbufferSGIX;
		glXCreateGLXPixmapWithConfigSGIX;
		glXDestroyGLXPbufferSGIX;
		glXGetCurrentReadDrawableSGI;
		glXGetFBConfigAttribSGIX;
		glXGetFBConfigFromVisualSGIX;
		glXGetSelectedEventSGIX;
 		glXGetVisualFromFBConfigSGIX;
		glXMakeCurrentReadSGI;
		glXQueryGLXPbufferSGIX;
		glXSelectEventSGIX;

		glXFreeContextEXT;
		glXImportContextEXT;
		glXQueryContextInfoEXT;

		glXJoinSwapGroupNV;
		glXBindSwapBarrierNV;
		glXQuerySwapGroupNV;
		glXQueryMaxSwapGroupsNV;
		glXQueryFrameCountNV;
		glXResetFrameCountNV;

		glXGetTransparentIndexSUN;

		glXGetProcAddressARB;
		glXGetProcAddress;

		glXCreateContextAttribsARB;

		glXBindTexImageEXT;
		glXReleaseTexImageEXT;

		glXSwapIntervalEXT;
		glXSwapIntervalSGI;

		glFinish;
		glFlush;
		glViewport;
		glDrawBuffer;
		glPopAttrib;
		glReadPixels;
		glDrawPixels;
		glIndexd;
		glIndexf;
		glIndexi;
		glIndexs;
		glIndexub;
		glIndexdv;
		glIndexfv;
		glIndexiv;
		glIndexsv;
		glIndexubv;
		glClearIndex;
		glGetDoublev;
		glGetFloatv;
		glGetIntegerv;
		glMaterialfv;
		glMaterialiv;
		glPixelTransferf;
		glPixelTransferi;

		XCheckMaskEvent;
		XCheckTypedEvent;
		XCheckTypedWindowEvent;
		XCheckWindowEvent;
		XCloseDisplay;
		XConfigureWindow;
		XCopyArea;
		XCreateWindow;
		XCreateSimpleWindow;
		XDestroySubwindows;
		XDestroyWindow;
		XFree;
		XGetGeometry;
		XGetImage;
		XListExtensions;
		XMaskEvent;
		XMoveResizeWindow;
		XNextEvent;
		XOpenDisplay;
		XQueryExtension;
		XResizeWindow;
		XServerVendor;
		XWindowEvent;

		__XCopyArea;

		_vgl_dlopen;

		#ifdef FAKEXCB
		xcb_get_extension_data;
		xcb_glx_query_version;
		xcb_glx_query_version_reply;
		xcb_poll_for_event;
		xcb_poll_for_queued_event;
		xcb_wait_for_event;
		#endif

	local:
		*;
};
