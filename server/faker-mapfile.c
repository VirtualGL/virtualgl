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
		glXGetCurrentDisplayEXT;
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

		#ifdef EGLBACKEND

		/* EGL 1.0 */
		eglChooseConfig;
		eglCopyBuffers;
		eglCreateContext;
		eglCreatePbufferSurface;
		eglCreatePixmapSurface;
		eglCreateWindowSurface;
		eglDestroyContext;
		eglDestroySurface;
		eglGetConfigAttrib;
		eglGetConfigs;
		eglGetCurrentDisplay;
		eglGetCurrentSurface;
		eglGetDisplay;
		eglGetError;
		eglGetProcAddress;
		eglInitialize;
		eglMakeCurrent;
		eglQueryContext;
		eglQueryString;
		eglQuerySurface;
		eglSwapBuffers;
		eglTerminate;

		/* EGL 1.1 */
		eglBindTexImage;
		eglReleaseTexImage;
		eglSurfaceAttrib;
		eglSwapInterval;

		/* EGL 1.2 */
		eglCreatePbufferFromClientBuffer;

		/* EGL 1.5 */
		eglClientWaitSync;
		eglCreateImage;
		eglCreatePlatformPixmapSurface;
		eglCreatePlatformWindowSurface;
		eglCreateSync;
		eglDestroyImage;
		eglDestroySync;
		eglGetPlatformDisplay;
		eglGetSyncAttrib;
		eglWaitSync;

		/* EGL_EXT_device_query */
		eglQueryDisplayAttribEXT;

		/* EGL_EXT_platform_base */
		eglCreatePlatformPixmapSurfaceEXT;
		eglCreatePlatformWindowSurfaceEXT;
		eglGetPlatformDisplayEXT;

		/* EGL_KHR_cl_event2 */
		eglCreateSync64KHR;

		/* EGL_KHR_fence_sync */
		eglClientWaitSyncKHR;
		eglCreateSyncKHR;
		eglDestroySyncKHR;
		eglGetSyncAttribKHR;

		/* EGL_KHR_image */
		eglCreateImageKHR;
		eglDestroyImageKHR;

		/* EGL_KHR_reusable_sync */
		eglSignalSyncKHR;

		/* EGL_KHR_wait_sync */
		eglWaitSyncKHR;

		#endif

		/* OpenGL */
		glBindFramebuffer;
		glBindFramebufferEXT;
		glDeleteFramebuffers;
		glDeleteFramebuffersEXT;
		glFinish;
		glFlush;
		glDrawBuffer;
		glDrawBuffers;
		glDrawBuffersARB;
		glDrawBuffersATI;
		glFramebufferDrawBufferEXT;
		glFramebufferDrawBuffersEXT;
		glFramebufferReadBufferEXT;
		glGetBooleanv;
		glGetDoublev;
		glGetFloatv;
		glGetFramebufferAttachmentParameteriv;
		glGetFramebufferParameteriv;
		glGetIntegerv;
		glGetInteger64v;
		glGetNamedFramebufferParameteriv;
		glGetString;
		glGetStringi;
		glNamedFramebufferDrawBuffer;
		glNamedFramebufferDrawBuffers;
		glNamedFramebufferReadBuffer;
		glPopAttrib;
		glReadBuffer;
		glReadPixels;
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
		XkbOpenDisplay;
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
		xcb_create_window;
		xcb_create_window_aux;
		xcb_create_window_aux_checked;
		xcb_create_window_checked;
		xcb_destroy_subwindows;
		xcb_destroy_subwindows_checked;
		xcb_destroy_window;
		xcb_destroy_window_checked;
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
