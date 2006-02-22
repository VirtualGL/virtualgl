/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "faker-sym.h"
#include "faker-macros.h"

// Map a client-side drawable to a server-side drawable

GLXDrawable ServerDrawable(Display *dpy, GLXDrawable draw)
{
	pbwin *pb=NULL;
	if((pb=winh.findpb(dpy, draw))!=NULL) return pb->getdrawable();
	else return draw;
}

static VisualID _MatchVisual(Display *dpy, GLXFBConfig config)
{
	VisualID vid=0;
	if(!dpy || !config) return 0;
	int screen=DefaultScreen(dpy);
	if(!(vid=cfgh.getvisual(dpy, config)))
	{
		vid=__vglMatchVisual(dpy, screen, __vglConfigDepth(config),
				__vglConfigClass(config),
				0,
				__vglServerVisualAttrib(config, GLX_STEREO),
				__vglServerVisualAttrib(config, GLX_TRANSPARENT_TYPE)!=GLX_NONE);
		if(!vid) 
			vid=__vglMatchVisual(dpy, screen, 24, TrueColor, 0, 0, 0);
	}
	if(vid) cfgh.add(dpy, config, vid);
	return vid;
}

static GLXFBConfig _MatchConfig(Display *dpy, XVisualInfo *vis)
{
	GLXFBConfig c=0, *configs=NULL;  int n=0;
	if(!dpy || !vis) return 0;
	if(!(c=vish.getpbconfig(dpy, vis)))
	{
		// Punt.  We can't figure out where the visual came from
		int attribs[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
			GLX_PBUFFER_BIT, GLX_STEREO, 0, None};
		if(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid, GLX_STEREO))
			attribs[13]=1;
		#ifdef USEGLP
		if(fconfig.glp)
		{
			attribs[10]=attribs[11]=None;
			configs=glPChooseFBConfig(_localdev, attribs, &n);
			if((!configs || n<1) && attribs[13])
			{
				attribs[13]=0;
				configs=glPChooseFBConfig(_localdev, attribs, &n);
			}
			if((!configs || n<1) && attribs[0])
			{
				attribs[0]=0;
				configs=glPChooseFBConfig(_localdev, attribs, &n);
			}
			if(!configs || n<1) return 0;
		}
		else
		#endif
		{
			configs=_glXChooseFBConfig(_localdpy, vis->screen, attribs, &n);
			if((!configs || n<1) && attribs[13])
			{
				attribs[13]=0;
				configs=_glXChooseFBConfig(_localdpy, vis->screen, attribs, &n);
			}
			if((!configs || n<1) && attribs[0])
			{
				attribs[0]=0;
				configs=_glXChooseFBConfig(_localdpy, vis->screen, attribs, &n);
			}
			if(!configs || n<1) return 0;
		}
		c=configs[0];
		XFree(configs);
		if(c)
		{
			vish.add(dpy, vis, c);
			cfgh.add(dpy, c, vis->visualid);
		}
	}
	return c;
}

extern "C" {

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen, const int *attrib_list, int *nelements)
{
	GLXFBConfig *configs=NULL;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXChooseFBConfig(dpy, screen, attrib_list, nelements);
	////////////////////

	if(attrib_list)
	{
		bool overlayreq=false;
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			if(attrib_list[i]==GLX_LEVEL && attrib_list[i+1]==1)
				overlayreq=true;
		}
		if(overlayreq)
		{
			configs=_glXChooseFBConfig(dpy, screen, attrib_list, nelements);
			if(configs && nelements && *nelements)
			{
				for(int i=0; i<*nelements; i++) rcfgh.add(dpy, configs[i]);
			}
			return configs;
		}
	}

		opentrace(glXChooseFBConfig);  prargd(dpy);  prargi(screen);
		prargal13(attrib_list);  starttrace();

	int depth=24, c_class=TrueColor, level=0, stereo=0, trans=0;
	if(!attrib_list || !nelements) return NULL;
	*nelements=0;
	configs=__vglConfigsFromVisAttribs(attrib_list, screen, depth, c_class, level,
		stereo, trans, *nelements, true);
	if(configs && *nelements)
	{
		VisualID vid=__vglMatchVisual(dpy, screen, depth, c_class, level, stereo,
			trans);
		if(vid) for(int i=0; i<*nelements; i++) cfgh.add(dpy, configs[i], vid);
		else {XFree(configs);  return NULL;}
	}

		stoptrace();  if(configs) {prargc(configs[0]);}
		if(nelements) {prargi(*nelements);}	 closetrace();

	CATCH();
	return configs;
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX (Display *dpy, int screen, const int *attrib_list, int *nelements)
{
	return glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}

#ifdef SUNOGL
void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned int mask)
#else
void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
#endif
{
	#ifdef USEGLP
	if(fconfig.glp) glPCopyContext(src, dst, mask);
	else
	#endif
	_glXCopyContext(_localdpy, src, dst, mask);
}

GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attrib_list)
{
	GLXPbuffer pb=0;

		opentrace(glXCreatePbuffer);  prargd(dpy);  prargc(config);
		prargal13(attrib_list);  starttrace();

	#ifdef USEGLP
	if(fconfig.glp) pb=glPCreateBuffer(config, attrib_list);
	else
	#endif
	pb=_glXCreatePbuffer(_localdpy, config, attrib_list);
	TRY();
	if(dpy && pb) glxdh.add(pb, dpy);
	CATCH();

		stoptrace();  prargx(pb);  closetrace();

	return pb;
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list)
{
	GLXPbuffer pb=0;

		opentrace(glXCreateGLXPbufferSGIX);  prargd(dpy);  prargc(config);
		prargi(width);  prargi(height);  prargal13(attrib_list);
		starttrace();

	#ifdef USEGLP
	if(fconfig.glp)
	{
		int glpattribs[257], j=0;
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			glpattribs[j++]=attrib_list[i];  glpattribs[j++]=attrib_list[i+1];
		}
		glpattribs[j++]=GLP_PBUFFER_WIDTH;  glpattribs[j++]=width;
		glpattribs[j++]=GLP_PBUFFER_HEIGHT;  glpattribs[j++]=height;
		glpattribs[j]=None;
		pb=glPCreateBuffer(config, glpattribs);
	}
	#endif
	pb=_glXCreateGLXPbufferSGIX(_localdpy, config, width, height, attrib_list);
	TRY();
	if(dpy && pb) glxdh.add(pb, dpy);
	CATCH();

		stoptrace();  prargx(pb);  closetrace();

	return pb;
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
{

		opentrace(glXDestroyGLXPbufferSGIX);  prargd(dpy);  prargx(pbuf);  starttrace();

	#ifdef USEGLP
	if(fconfig.glp) glPDestroyBuffer(pbuf);
	else
	#endif
	_glXDestroyGLXPbufferSGIX(_localdpy, pbuf);
	TRY();
	if(pbuf) glxdh.remove(pbuf);
	CATCH();

		stoptrace();  closetrace();
}

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
		opentrace(glXDestroyPbuffer);  prargd(dpy);  prargx(pbuf);  starttrace();

	#ifdef USEGLP
	if(fconfig.glp) glPDestroyBuffer(pbuf);
	else
	#endif
	_glXDestroyPbuffer(_localdpy, pbuf);
	TRY();
	if(pbuf) glxdh.remove(pbuf);
	CATCH();

		stoptrace();  closetrace();
}

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
	#ifdef USEGLP
	// Tell me about the rabbits, George ...
	if(fconfig.glp) return;
	else
	#endif
	_glXFreeContextEXT(_localdpy, ctx);
}

const char *glXGetClientString(Display *dpy, int name)
{
	#ifdef USEGLP
	if(fconfig.glp)
	{
		if(name==GLX_VERSION) return "1.3";
		else return glPGetLibraryString(name);
	}
	else
	#endif
	return _glXGetClientString(_localdpy, name);
}

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	GLXFBConfig c;  int retval=0;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXGetConfig(dpy, vis, attrib, value);
	////////////////////

	if(vis)
	{
		int level=__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_LEVEL);
		int trans=(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
			GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
		if(level && trans && attrib!=GLX_LEVEL && attrib!=GLX_TRANSPARENT_TYPE)
			return _glXGetConfig(dpy, vis, attrib, value);
	}

	if(!dpy || !value) throw rrerror("glXGetConfig", "Invalid argument");

		opentrace(glXGetConfig);  prargd(dpy);  prargv(vis);  prargx(attrib);
		starttrace();

	errifnot(c=_MatchConfig(dpy, vis));

	if(attrib==GLX_USE_GL)
	{
		if(vis->c_class==TrueColor || vis->c_class==PseudoColor) *value=1;
		else *value=0;
	}
	else if(vis->c_class==PseudoColor && (attrib==GLX_RED_SIZE || attrib==GLX_GREEN_SIZE
		|| attrib==GLX_BLUE_SIZE || attrib==GLX_ALPHA_SIZE
		|| attrib==GLX_ACCUM_RED_SIZE || attrib==GLX_ACCUM_GREEN_SIZE
		|| attrib==GLX_ACCUM_BLUE_SIZE || attrib==GLX_ACCUM_ALPHA_SIZE))
		*value=0;
	else if(attrib==GLX_LEVEL || attrib==GLX_TRANSPARENT_TYPE
		|| attrib==GLX_TRANSPARENT_INDEX_VALUE
		|| attrib==GLX_TRANSPARENT_RED_VALUE
		|| attrib==GLX_TRANSPARENT_GREEN_VALUE
		|| attrib==GLX_TRANSPARENT_BLUE_VALUE
		|| attrib==GLX_TRANSPARENT_ALPHA_VALUE)
		*value=__vglClientVisualAttrib(dpy, vis->screen, vis->visualid, attrib);
	else if(attrib==GLX_RGBA)
	{
		if(vis->c_class==PseudoColor) *value=0;  else *value=1;
	}
	else if(attrib==GLX_STEREO)
	{
		*value= (__vglClientVisualAttrib(dpy, vis->screen, vis->visualid, GLX_STEREO)
			&& __vglServerVisualAttrib(c, GLX_STEREO));
	}
	else if(attrib==GLX_X_VISUAL_TYPE)
	{
		if(vis->c_class==PseudoColor) *value=GLX_PSEUDO_COLOR;
		else *value=GLX_TRUE_COLOR;
	}
	else
	{
		if(attrib==GLX_BUFFER_SIZE && vis->c_class==PseudoColor
			&& __vglServerVisualAttrib(c, GLX_RENDER_TYPE)==GLX_RGBA_BIT)
			attrib=GLX_RED_SIZE;
		#ifdef USEGLP
		if(fconfig.glp)
		{
			if(attrib==GLX_VIDEO_RESIZE_SUN) *value=0;
			else if(attrib==GLX_VIDEO_REFRESH_TIME_SUN) *value=0;
			else if(attrib==GLX_GAMMA_VALUE_SUN) *value=100;
			else retval=glPGetFBConfigAttrib(c, attrib, value);
		}
		else
		#endif
		retval=_glXGetFBConfigAttrib(_localdpy, c, attrib, value);
	}

		stoptrace();  if(value) {prargi(*value);}  closetrace();

	CATCH();
	return retval;
}

#ifdef USEGLP
GLXContext glXGetCurrentContext(void)
{
	if(fconfig.glp) return glPGetCurrentContext();
	else
	return _glXGetCurrentContext();
}
#endif

Display *glXGetCurrentDisplay(void)
{
	Display *dpy=NULL;  pbwin *pb=NULL;

	TRY();

		opentrace(glXGetCurrentDisplay);  starttrace();

	if((pb=winh.findpb(GetCurrentDrawable()))!=NULL)
		dpy=pb->getwindpy();
	else dpy=glxdh.getcurrentdpy(GetCurrentDrawable());

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}

GLXDrawable glXGetCurrentDrawable(void)
{
	pbwin *pb=NULL;  GLXDrawable draw=GetCurrentDrawable();
	TRY();

		opentrace(glXGetCurrentDrawable);  starttrace();

	if((pb=winh.findpb(draw))!=NULL)
		draw=pb->getwin();

		stoptrace();  prargx(draw);  closetrace();

	CATCH();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	pbwin *pb=NULL;  GLXDrawable read=GetCurrentReadDrawable();
	TRY();

		opentrace(glXGetCurrentReadDrawable);  starttrace();

	if((pb=winh.findpb(read))!=NULL)
		read=pb->getwin();

		stoptrace();  prargx(read);  closetrace();

	CATCH();
	return read;
}

int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
	VisualID vid=0;  int retval=0;
	TRY();

	// Prevent recursion && handle overlay configs
	if(!_isremote(dpy) || rcfgh.isoverlay(dpy, config))
		return _glXGetFBConfigAttrib(dpy, config, attribute, value);
	////////////////////

	if(!dpy || !value) throw rrerror("glXGetFBConfigAttrib", "Invalid argument");
	int screen=DefaultScreen(dpy);

		opentrace(glXGetFBConfigAttrib);  prargd(dpy);  prargc(config);
		prargi(attribute);  starttrace();

	if(!(vid=_MatchVisual(dpy, config)))
		throw rrerror("glXGetFBConfigAttrib", "Invalid FB config");

	int c_class=__vglVisualClass(dpy, screen, vid);
	if(c_class==PseudoColor && (attribute==GLX_RED_SIZE || attribute==GLX_GREEN_SIZE
		|| attribute==GLX_BLUE_SIZE || attribute==GLX_ALPHA_SIZE
		|| attribute==GLX_ACCUM_RED_SIZE || attribute==GLX_ACCUM_GREEN_SIZE
		|| attribute==GLX_ACCUM_BLUE_SIZE || attribute==GLX_ACCUM_ALPHA_SIZE))
		*value=0;
	else if(attribute==GLX_LEVEL || attribute==GLX_TRANSPARENT_TYPE
		|| attribute==GLX_TRANSPARENT_INDEX_VALUE
		|| attribute==GLX_TRANSPARENT_RED_VALUE
		|| attribute==GLX_TRANSPARENT_GREEN_VALUE
		|| attribute==GLX_TRANSPARENT_BLUE_VALUE
		|| attribute==GLX_TRANSPARENT_ALPHA_VALUE)
		*value=__vglClientVisualAttrib(dpy, screen, vid, attribute);
	else if(attribute==GLX_STEREO)
	{
		*value= (__vglClientVisualAttrib(dpy, screen, vid, GLX_STEREO)
			&& __vglServerVisualAttrib(config, GLX_STEREO));
	}
	else if(attribute==GLX_X_VISUAL_TYPE)
	{
		if(c_class==PseudoColor) *value=GLX_PSEUDO_COLOR;
		else *value=GLX_TRUE_COLOR;
	}
	else if(attribute==GLX_VISUAL_ID)
		*value=vid;
	else
	{
		if(attribute==GLX_BUFFER_SIZE && c_class==PseudoColor
			&& __vglServerVisualAttrib(config, GLX_RENDER_TYPE)==GLX_RGBA_BIT)
			attribute=GLX_RED_SIZE;
		#ifdef USEGLP
		if(fconfig.glp)
		{
			if(attribute==GLX_VIDEO_RESIZE_SUN) *value=0;
			else if(attribute==GLX_VIDEO_REFRESH_TIME_SUN) *value=0;
			else if(attribute==GLX_GAMMA_VALUE_SUN) *value=100;
			else retval=glPGetFBConfigAttrib(config, attribute, value);
		}
		else
		#endif
		retval=_glXGetFBConfigAttrib(_localdpy, config, attribute, value);
	}

		stoptrace();  if(value) {prargi(*value);}  closetrace();

	CATCH();
	return retval;
}

int glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value_return)
{
	return glXGetFBConfigAttrib(dpy, config, attribute, value_return);
}

GLXFBConfigSGIX glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
	return _MatchConfig(dpy, vis);
}

GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetFBConfigs(_localdev, nelements);
	else
	#endif
	return _glXGetFBConfigs(_localdpy, screen, nelements);
}

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXGetSelectedEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXGetSelectedEventSGIX(_localdpy, ServerDrawable(dpy, drawable), mask);
}

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
	#ifdef USEGLP
	if(fconfig.glp) return 0;
	else
	#endif
	return _glXImportContextEXT(_localdpy, contextID);
}

Bool glXIsDirect (Display *dpy, GLXContext ctx)
{
	#ifdef USEGLP
	if(fconfig.glp) return True;
	else
	#endif
	return _glXIsDirect(_localdpy, ctx);
}

int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryContext(ctx, attribute, value);
	else
	#endif
	return _glXQueryContext(_localdpy, ctx, attribute, value);
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryContext(ctx, attribute, value);
	else
	#endif
	return _glXQueryContextInfoEXT(_localdpy, ctx, attribute, value);
}

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
	TRY();

		opentrace(glXQueryDrawable);  prargd(dpy);  prargx(draw);  prargi(attribute);
		starttrace();

	#ifdef USEGLP
	if(fconfig.glp) glPQueryBuffer(ServerDrawable(dpy, draw), attribute, value);
	else
	#endif
	_glXQueryDrawable(_localdpy, ServerDrawable(dpy, draw), attribute, value);

		stoptrace();  prargx(ServerDrawable(dpy, draw));  if(value) {prargi(*value);}
		closetrace();

	CATCH();
}

Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
	#ifdef USEGLP
	if(fconfig.glp)
		{if(error_base) *error_base=0;  if(event_base) *event_base=0;  return True;}
	else
	#endif
	return _glXQueryExtension(_localdpy, error_base, event_base);
}

const char *glXQueryExtensionsString(Display *dpy, int screen)
{
	#ifdef USEGLP
	if(fconfig.glp) return "GLX_SGIX_fbconfig GLX_SGIX_pbuffer GLX_EXT_visual_info";
	else
	#endif
	return _glXQueryExtensionsString(_localdpy, screen);
}

const char *glXQueryServerString(Display *dpy, int screen, int name)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetDeviceString(_localdev, name);
	else
	#endif
	return _glXQueryServerString(_localdpy, screen, name);
}

void glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) glPQueryBuffer(pbuf, attribute, value);
	else
	#endif
	_glXQueryGLXPbufferSGIX(_localdpy, pbuf, attribute, value);
}

Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
	#ifdef USEGLP
	if(fconfig.glp) {*major=1;  *minor=3;  return True;}
	else
	#endif
	return _glXQueryVersion(_localdpy, major, minor);
}

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXSelectEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXSelectEventSGIX(_localdpy, ServerDrawable(dpy, drawable), mask);
}

#ifdef sun
int glXDisableXineramaSUN(Display *dpy)
{
	#ifdef USEGLP
	if(fconfig.glp) return 0;
	else
	#endif
	return _glXDisableXineramaSUN(_localdpy);
}
#endif

GLboolean glXGetTransparentIndexSUN(Display *dpy, Window overlay,
	Window underlay, unsigned int *transparentIndex)
{
	XWindowAttributes xwa;
	if(!transparentIndex) return False;

		opentrace(glXGetTransparentIndexSUN);  prargd(dpy);  prargx(overlay);
		prargx(underlay);  starttrace();

	if(fconfig.transpixel>=0)
		*transparentIndex=(unsigned int)fconfig.transpixel;
	else
	{
		if(!dpy || !overlay) return False;
		XGetWindowAttributes(dpy, overlay, &xwa);
		*transparentIndex=__vglClientVisualAttrib(dpy, DefaultScreen(dpy),
			xwa.visual->visualid, GLX_TRANSPARENT_INDEX_VALUE);
	}

		stoptrace();  prargi(*transparentIndex);  closetrace();

	return True;
}

Bool glXJoinSwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint group)
{
	return _glXJoinSwapGroupNV(_localdpy, ServerDrawable(dpy, drawable), group);
}

shimfuncdpy3(Bool, glXBindSwapBarrierNV, Display*, dpy, GLuint, group, GLuint, barrier, return );

Bool glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier)
{
	return _glXQuerySwapGroupNV(_localdpy, ServerDrawable(dpy, drawable), group, barrier);
}

shimfuncdpy4(Bool, glXQueryMaxSwapGroupsNV, Display*, dpy, int, screen, GLuint*, maxGroups, GLuint*, maxBarriers, return );
shimfuncdpy3(Bool, glXQueryFrameCountNV, Display*, dpy, int, screen, GLuint*, count, return );
shimfuncdpy2(Bool, glXResetFrameCountNV, Display*, dpy, int, screen, return );

#ifdef GLX_ARB_get_proc_address

#define checkfaked(f) if(!strcmp((char *)procName, #f)) retval=(void (*)(void))f;

extern void __vgl_fakerinit(void);

void (*glXGetProcAddressARB(const GLubyte *procName))(void)
{
	void (*retval)(void)=NULL;

	__vgl_fakerinit();

		opentrace(glXGetProcAddressARB);  prargs((char *)procName);  starttrace();

	if(procName)
	{
		checkfaked(glXChooseVisual)
		checkfaked(glXCopyContext)
		checkfaked(glXCreateContext)
		checkfaked(glXCreateGLXPixmap)
		checkfaked(glXDestroyContext)
		checkfaked(glXDestroyGLXPixmap)
		checkfaked(glXGetConfig)
		#ifdef USEGLP
		checkfaked(glXGetCurrentContext)
		#endif
		checkfaked(glXGetCurrentDrawable)
		checkfaked(glXIsDirect)
		checkfaked(glXMakeCurrent);
		checkfaked(glXQueryExtension)
		checkfaked(glXQueryVersion)
		checkfaked(glXSwapBuffers)
		checkfaked(glXUseXFont)
		checkfaked(glXWaitGL)

		checkfaked(glXGetClientString)
		checkfaked(glXQueryServerString)
		checkfaked(glXQueryExtensionsString)

		checkfaked(glXChooseFBConfig)
		checkfaked(glXCreateNewContext)
		checkfaked(glXCreatePbuffer)
		checkfaked(glXCreatePixmap)
		checkfaked(glXCreateWindow)
		checkfaked(glXDestroyPbuffer)
		checkfaked(glXDestroyPixmap)
		checkfaked(glXDestroyWindow)
		checkfaked(glXGetCurrentDisplay)
		checkfaked(glXGetCurrentReadDrawable)
		checkfaked(glXGetFBConfigAttrib)
		checkfaked(glXGetFBConfigs)
		checkfaked(glXGetSelectedEvent)
		checkfaked(glXGetVisualFromFBConfig)
		checkfaked(glXMakeContextCurrent);
		checkfaked(glXQueryContext)
		checkfaked(glXQueryDrawable)
		checkfaked(glXSelectEvent)

		checkfaked(glXFreeContextEXT)
		checkfaked(glXImportContextEXT)
		checkfaked(glXQueryContextInfoEXT)

		checkfaked(glXJoinSwapGroupNV)
		checkfaked(glXBindSwapBarrierNV)
		checkfaked(glXQuerySwapGroupNV)
		checkfaked(glXQueryMaxSwapGroupsNV)
		checkfaked(glXQueryFrameCountNV)
		checkfaked(glXResetFrameCountNV)

		checkfaked(glXGetFBConfigAttribSGIX)
		checkfaked(glXChooseFBConfigSGIX)

		checkfaked(glXCreateGLXPbufferSGIX)
		checkfaked(glXDestroyGLXPbufferSGIX)
		checkfaked(glXQueryGLXPbufferSGIX)
		checkfaked(glXSelectEventSGIX)
		checkfaked(glXGetSelectedEventSGIX)

		#ifdef sun
		checkfaked(glXDisableXineramaSUN)
		#endif

		checkfaked(glFinish)
		checkfaked(glFlush)
		checkfaked(glViewport)
		checkfaked(glDrawBuffer)
		checkfaked(glPopAttrib)
		checkfaked(glReadPixels)
		checkfaked(glDrawPixels)
		#ifdef SUNOGL
		checkfaked(glBegin)
		#endif
	}
	if(!retval) retval=_glXGetProcAddressARB(procName);

		stoptrace();  closetrace();

	return retval;
}

#endif

}
