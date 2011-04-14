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
		glXSwapIntervalSGI;

		glXBindTexImageEXT;
		glXReleaseTexImageEXT;

		glXGetTransparentIndexSUN;

		glXGetProcAddressARB;
		glXGetProcAddress;

		glFinish;
		glFlush;
		glViewport;
		glDrawBuffer;
		glPopAttrib;
		glReadPixels;
		glDrawPixels;
		#ifdef SUNOGL
		glBegin;
		#else
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
		#endif
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
		XConfigureWindow;
		XCopyArea;
		XCreateWindow;
		XCreateSimpleWindow;
		XDestroyWindow;
		XFree;
		XGetGeometry;
		XListExtensions;
		XMaskEvent;
		XMoveResizeWindow;
		XNextEvent;
		XOpenDisplay;
		XQueryExtension;
		XResizeWindow;
		XServerVendor;
		XWindowEvent;
		#ifdef sparc
		XSolarisGetVisualGamma;
		#endif

		__XFree;
		__XGetGeometry;
		__XNextEvent;
		__XOpenDisplay;
		__XCreateWindow;

		_vgl_dlopen;

	local:
		*;
};
