{
	global:
		/* GLX 1.0 */
		glXChooseVisual;
		glXCopyContext;
		glXCreateContext;
		glXCreateGLXPixmap;
		glXDestroyContext;
		glXDestroyGLXPixmap;
		glXGetConfig;
		glXGetCurrentContext;
		glXGetCurrentDrawable;
		glXIsDirect;
		glXMakeCurrent;
		glXQueryExtension;
		glXQueryVersion;
		glXSwapBuffers;
		glXUseXFont;
		glXWaitGL;

		/* GLX 1.1 */
		glXGetClientString;
		glXQueryServerString;
		glXQueryExtensionsString;

		/* GLX 1.2 */
		glXGetCurrentDisplay;

		/* GLX 1.3 */
		glXChooseFBConfig;
		glXCreateNewContext;
		glXCreatePbuffer;
		glXCreatePixmap;
		glXCreateWindow;
		glXDestroyPbuffer;
		glXDestroyPixmap;
		glXDestroyWindow;
		glXGetCurrentReadDrawable;
		glXGetFBConfigAttrib;
		glXGetFBConfigs;
		glXGetSelectedEvent;
		glXGetVisualFromFBConfig;
		glXMakeContextCurrent;
		glXQueryContext;
		glXQueryDrawable;
		glXSelectEvent;

		/* GLX 1.4 */
		glXGetProcAddress;

		/* GLX_ARB_create_context */
		glXCreateContextAttribsARB;

		/* GLX_ARB_get_proc_address */
		glXGetProcAddressARB;

		/* GLX_EXT_import_context */
		glXFreeContextEXT;
		glXImportContextEXT;
		glXQueryContextInfoEXT;

		/* GLX_EXT_swap_control */
		glXSwapIntervalEXT;

		/* GLX_EXT_texture_from_pixmap */
		glXBindTexImageEXT;
		glXReleaseTexImageEXT;

		/* GLX_SGI_make_current_read */
		glXGetCurrentReadDrawableSGI;
		glXMakeCurrentReadSGI;

		/* GLX_SGI_swap_control */
		glXSwapIntervalSGI;

		/* GLX_SGIX_fbconfig */
		glXChooseFBConfigSGIX;
		glXCreateContextWithConfigSGIX;
		glXCreateGLXPixmapWithConfigSGIX;
		glXGetFBConfigAttribSGIX;
		glXGetFBConfigFromVisualSGIX;
		glXGetVisualFromFBConfigSGIX;

		/* GLX_SGIX_pbuffer */
		glXCreateGLXPbufferSGIX;
		glXDestroyGLXPbufferSGIX;
		glXGetSelectedEventSGIX;
		glXQueryGLXPbufferSGIX;
		glXSelectEventSGIX;

		/* OpenGL */
		glBindFramebuffer;
		glBindFramebufferEXT;
		glDeleteFramebuffers;
		glDeleteFramebuffersEXT;
		glFinish;
		glFlush;
		glDrawBuffer;
		glDrawBuffers;
		#ifdef GL_VERSION_4_5
		glFramebufferDrawBufferEXT;
		glFramebufferDrawBuffersEXT;
		glFramebufferReadBufferEXT;
		#endif
		glGetBooleanv;
		glGetDoublev;
		glGetFloatv;
		glGetIntegerv;
		glGetInteger64v;
		glGetString;
		glGetStringi;
		#ifdef GL_VERSION_4_5
		glNamedFramebufferDrawBuffer;
		glNamedFramebufferDrawBuffers;
		glNamedFramebufferReadBuffer;
		#endif
		glPopAttrib;
		glReadBuffer;
		glViewport;

		/* OpenCL */
		#ifdef FAKEOPENCL
		clCreateContext;
		#endif

		/* X11 */
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
		#ifdef FAKEXCB
		XSetEventQueueOwner;
		#endif
		XWindowEvent;

		_vgl_dlopen;
		_vgl_getAutotestColor;
		_vgl_getAutotestFrame;
		_vgl_disableFaker;
		_vgl_enableFaker;

		/* XCB */
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
