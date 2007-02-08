#ifdef linux
VIRTUALGL
#endif
{
	global:
		glXChooseVisual;
		glXCopyContext;
		glXCreateContext;
		glXCreateGLXPixmap;
		glXDestroyContext;
		glXDestroyGLXPixmap;
		glXGetConfig;
		#ifdef USEGLP
		glXGetCurrentContext;
		#endif
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

		glXFreeContextEXT;
		glXImportContextEXT;
		glXQueryContextInfoEXT;

		glXJoinSwapGroupNV;
		glXBindSwapBarrierNV;
		glXQuerySwapGroupNV;
		glXQueryMaxSwapGroupsNV;
		glXQueryFrameCountNV;
		glXResetFrameCountNV;

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
		XMaskEvent;
		XMoveResizeWindow;
		XNextEvent;
		XOpenDisplay;
		XQueryExtension;
		XResizeWindow;
		XServerVendor;
		XWindowEvent;

	local:
		*;
};
