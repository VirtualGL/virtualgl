/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "faker-sym.h"

// Map a client-side drawable to a server-side drawable

GLXDrawable ServerDrawable(Display *dpy, GLXDrawable draw)
{
	pbwin *pbw=NULL;
	if(winh.findpb(dpy, draw, pbw)) return pbw->getdrawable();
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
				0);
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
	if(!(c=vish.getpbconfig(dpy, vis))&& !(c=vish.mostrecentcfg(dpy, vis)))
	{
		// Punt.  We can't figure out where the visual came from
		int attribs[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_STEREO, 0,
			GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None};
		if(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid, GLX_STEREO))
			attribs[11]=1;
		#ifdef USEGLP
		if(fconfig.glp)
		{
			attribs[12]=attribs[13]=None;
			if(!attribs[11]) attribs[10]=None;
			configs=glPChooseFBConfig(_localdev, attribs, &n);
			if((!configs || n<1) && attribs[11])
			{
				attribs[10]=attribs[11]=0;
				configs=glPChooseFBConfig(_localdev, attribs, &n);
			}
			if((!configs || n<1) && attribs[1])
			{
				attribs[1]=0;
				configs=glPChooseFBConfig(_localdev, attribs, &n);
			}
			if(!configs || n<1) return 0;
		}
		else
		#endif
		{
			configs=glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), attribs, &n);
			if((!configs || n<1) && attribs[11])
			{
				attribs[11]=0;
				configs=glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), attribs, &n);
			}
			if((!configs || n<1) && attribs[1])
			{
				attribs[1]=0;
				configs=glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), attribs, &n);
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
	if(!_isremote(dpy))
	{
			opentrace(_glXChooseFBConfig);  prargd(dpy);  prargi(screen);
			prargal13(attrib_list);  starttrace();

		configs=_glXChooseFBConfig(dpy, screen, attrib_list, nelements);

			stoptrace();  if(configs) {prargc(configs[0]);}
			if(nelements) {prargi(*nelements);}	 closetrace();

		return configs;
	}
	////////////////////

		opentrace(glXChooseFBConfig);  prargd(dpy);  prargi(screen);
		prargal13(attrib_list);  starttrace();

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
			int dummy;
			if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
				configs=NULL;
			else configs=_glXChooseFBConfig(dpy, screen, attrib_list, nelements);
			if(configs && nelements && *nelements)
			{
				for(int i=0; i<*nelements; i++) rcfgh.add(dpy, configs[i]);
			}
			stoptrace();  if(configs) {prargc(configs[0]);}
			if(nelements) {prargi(*nelements);}	 closetrace();
			return configs;
		}
	}

	int depth=24, c_class=TrueColor, level=0, stereo=0, trans=0;
	if(!attrib_list || !nelements) return NULL;
	*nelements=0;
	configs=__vglConfigsFromVisAttribs(attrib_list, depth, c_class, level,
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

#ifdef SUNOGL
GLXFBConfigSGIX *glXChooseFBConfigSGIX (Display *dpy, int screen, const int *attrib_list, int *nelements)
#else
GLXFBConfigSGIX *glXChooseFBConfigSGIX (Display *dpy, int screen, int *attrib_list, int *nelements)
#endif
{
	return glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}

#ifdef SUNOGL
void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned int mask)
#else
void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
#endif
{
	TRY();
	bool srcoverlay=false, dstoverlay=false;
	if(ctxh.isoverlay(src)) srcoverlay=true;
	if(ctxh.isoverlay(dst)) dstoverlay=true;
	if(srcoverlay && dstoverlay)
		{_glXCopyContext(dpy, src, dst, mask);  return;}
	else if(srcoverlay!=dstoverlay)
		_throw("glXCopyContext() cannot copy between overlay and non-overlay contexts");
	#ifdef USEGLP
	if(fconfig.glp) glPCopyContext(src, dst, mask);
	else
	#endif
	_glXCopyContext(_localdpy, src, dst, mask);
	CATCH();
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
	int attribs[257], j=0;
	for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
	{
		attribs[j++]=attrib_list[i];  attribs[j++]=attrib_list[i+1];
	}
	attribs[j++]=GLX_PBUFFER_WIDTH;  attribs[j++]=width;
	attribs[j++]=GLX_PBUFFER_HEIGHT;  attribs[j++]=height;
	attribs[j]=None;
	return glXCreatePbuffer(dpy, config, attribs);
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
{
	glXDestroyPbuffer(dpy, pbuf);
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
	if(ctxh.isoverlay(ctx)) {_glXFreeContextEXT(dpy, ctx);  return;}
	#ifdef USEGLP
	// Tell me about the rabbits, George ...
	if(fconfig.glp) return;
	else
	#endif
	_glXFreeContextEXT(_localdpy, ctx);
}

static const char *glxextensions=
#ifdef SUNOGL
	"GLX_ARB_get_proc_address GLX_ARB_multisample GLX_EXT_texture_from_pixmap GLX_EXT_visual_info GLX_EXT_visual_rating GLX_SGI_make_current_read GLX_SGIX_fbconfig GLX_SGIX_pbuffer GLX_SUN_get_transparent_index GLX_SUN_init_threads";
#else
	"GLX_ARB_get_proc_address GLX_ARB_multisample GLX_EXT_texture_from_pixmap GLX_EXT_visual_info GLX_EXT_visual_rating GLX_SGI_make_current_read GLX_SGIX_fbconfig GLX_SGIX_pbuffer GLX_SUN_get_transparent_index";
#endif

const char *glXGetClientString(Display *dpy, int name)
{
	// If this is called internally to OpenGL, use the real function
	if(!_isremote(dpy)) return _glXGetClientString(dpy, name);
	////////////////////
	if(name==GLX_EXTENSIONS) return glxextensions;
	else if(name==GLX_VERSION) return "1.4";
	else if(name==GLX_VENDOR) return __APPNAME;
	else return NULL;
}

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	GLXFBConfig c;  int retval=0;
	TRY();

	// Prevent recursion
	if(!_isremote(dpy)) return _glXGetConfig(dpy, vis, attrib, value);
	////////////////////

		opentrace(glXGetConfig);  prargd(dpy);  prargv(vis);  prargx(attrib);
		starttrace();

	if(!dpy || !vis || !value)
	{
		stoptrace()  prargx(value);  closetrace();
		return GLX_BAD_VALUE;
	}

	int level=__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
		GLX_LEVEL);
	int trans=(__vglClientVisualAttrib(dpy, DefaultScreen(dpy), vis->visualid,
		GLX_TRANSPARENT_TYPE)==GLX_TRANSPARENT_INDEX);
	if(level && trans && attrib!=GLX_LEVEL && attrib!=GLX_TRANSPARENT_TYPE)
	{
		int dummy;
		stoptrace();  if(value) {prargi(*value);}  closetrace();
		if(!_XQueryExtension(dpy, "GLX", &dummy, &dummy, &dummy))
			retval=GLX_NO_EXTENSION;
		else retval=_glXGetConfig(dpy, vis, attrib, value);
		return retval;
	}

	if(!(c=_MatchConfig(dpy, vis))) _throw("Could not obtain Pbuffer-capable RGB visual on the server");

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
		*value=__vglServerVisualAttrib(c, GLX_STEREO);		
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
	GLXContext ctx=_glXGetCurrentContext();
	if(fconfig.glp && !ctxh.isoverlay(ctx)) ctx=glPGetCurrentContext();
	return ctx;
}
#endif

Display *glXGetCurrentDisplay(void)
{
	Display *dpy=NULL;  pbwin *pbw=NULL;

	if(ctxh.overlaycurrent()) return _glXGetCurrentDisplay();

	TRY();

		opentrace(glXGetCurrentDisplay);  starttrace();

	GLXDrawable curdraw=GetCurrentDrawable();
	if(winh.findpb(curdraw, pbw)) dpy=pbw->getwindpy();
	else
	{
		if(curdraw) dpy=glxdh.getcurrentdpy(curdraw);
	}

		stoptrace();  prargd(dpy);  closetrace();

	CATCH();
	return dpy;
}

GLXDrawable glXGetCurrentDrawable(void)
{
	if(ctxh.overlaycurrent()) return _glXGetCurrentDrawable();

	pbwin *pbw=NULL;  GLXDrawable draw=GetCurrentDrawable();

	TRY();

		opentrace(glXGetCurrentDrawable);  starttrace();

	if(winh.findpb(draw, pbw)) draw=pbw->getwin();

		stoptrace();  prargx(draw);  closetrace();

	CATCH();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	if(ctxh.overlaycurrent()) return _glXGetCurrentReadDrawable();

	pbwin *pbw=NULL;  GLXDrawable read=GetCurrentReadDrawable();

	TRY();

		opentrace(glXGetCurrentReadDrawable);  starttrace();

	if(winh.findpb(read, pbw)) read=pbw->getwin();

		stoptrace();  prargx(read);  closetrace();

	CATCH();
	return read;
}

GLXDrawable glXGetCurrentReadDrawableSGI(void)
{
	return glXGetCurrentReadDrawable();
}

int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
	VisualID vid=0;  int retval=0;
	TRY();

	// Prevent recursion && handle overlay configs
	if((dpy && config) && (!_isremote(dpy) || rcfgh.isoverlay(dpy, config)))
		return _glXGetFBConfigAttrib(dpy, config, attribute, value);
	////////////////////

	int screen=dpy? DefaultScreen(dpy):0;

		opentrace(glXGetFBConfigAttrib);  prargd(dpy);  prargc(config);
		prargi(attribute);  starttrace();

	if(!dpy || !config || !value)
	{
		stoptrace();  prargx(value);  closetrace();
		return GLX_BAD_VALUE;
	}

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
	else if(attribute==GLX_RENDER_TYPE)
	{
		if(c_class==PseudoColor) *value=GLX_COLOR_INDEX_BIT;
		else *value=GLX_RGBA_BIT;
	}
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
	else if(attribute==GLX_DRAWABLE_TYPE)
	{
		*value=GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT;
	}
	else
	{
		if(attribute==GLX_BUFFER_SIZE && c_class==PseudoColor
			&& __vglServerVisualAttrib(config, GLX_RENDER_TYPE)==GLX_RGBA_BIT)
			attribute=GLX_RED_SIZE;
		if(attribute==GLX_CONFIG_CAVEAT)
		{
			int vistype=__vglServerVisualAttrib(config, GLX_X_VISUAL_TYPE);
			if(vistype!=GLX_TRUE_COLOR && vistype!=GLX_PSEUDO_COLOR && !fconfig.glp)
			{
				*value=GLX_NON_CONFORMANT_CONFIG;
				return retval;
			}
		}
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
	return _glXGetFBConfigs(_localdpy, DefaultScreen(_localdpy), nelements);
}

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
	if(winh.isoverlay(dpy, draw))
		return _glXGetSelectedEvent(dpy, draw, event_mask);

	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXGetSelectedEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
	glXGetSelectedEvent(dpy, drawable, mask);
}

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
	#ifdef USEGLP
	if(fconfig.glp) return 0;
	else
	#endif
	return _glXImportContextEXT(_localdpy, contextID);
}

Bool glXIsDirect(Display *dpy, GLXContext ctx)
{
	if(ctxh.isoverlay(ctx)) return _glXIsDirect(dpy, ctx);

	#ifdef USEGLP
	if(fconfig.glp) return True;
	else
	#endif
	return _glXIsDirect(_localdpy, ctx);
}

int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	int retval=0;
	if(ctxh.isoverlay(ctx)) return _glXQueryContext(dpy, ctx, attribute, value);

		opentrace(glXQueryContext);  prargd(dpy);  prargx(ctx);  prargi(attribute);
		starttrace();

	if(attribute==GLX_RENDER_TYPE)
	{
		int fbcid=-1;
		#ifdef USEGLP
		if(fconfig.glp) retval=glPQueryContext(ctx, GLX_FBCONFIG_ID, &fbcid);
		else
		#endif
		retval=_glXQueryContext(_localdpy, ctx, GLX_FBCONFIG_ID, &fbcid);
		if(fbcid>0)
		{
			VisualID vid=cfgh.getvisual(dpy, fbcid);
			if(vid && __vglVisualClass(dpy, DefaultScreen(dpy), vid)==PseudoColor
				&& value) *value=GLX_COLOR_INDEX_TYPE;
			else if(value) *value=GLX_RGBA_TYPE;
		}
	}
	else
	{
		#ifdef USEGLP
		if(fconfig.glp) retval=glPQueryContext(ctx, attribute, value);
		else
		#endif
		retval=_glXQueryContext(_localdpy, ctx, attribute, value);
	}

		stoptrace();  if(value) prargi(*value);  closetrace();

	return retval;
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	if(ctxh.isoverlay(ctx))
		return _glXQueryContextInfoEXT(dpy, ctx, attribute, value);

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

	if(winh.isoverlay(dpy, draw))
	{
		_glXQueryDrawable(dpy, draw, attribute, value);
		stoptrace(); if(value) {prargi(*value);}  closetrace();
		return;
	}

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
	// If this is called internally to OpenGL, use the real function
	if(!_isremote(dpy)) return _glXQueryExtensionsString(dpy, screen);
	////////////////////
	return glxextensions;
}

const char *glXQueryServerString(Display *dpy, int screen, int name)
{
	// If this is called internally to OpenGL, use the real function
	if(!_isremote(dpy)) return _glXQueryServerString(dpy, screen, name);
	////////////////////
	if(name==GLX_EXTENSIONS) return glxextensions;
	else if(name==GLX_VERSION) return "1.4";
	else if(name==GLX_VENDOR) return __APPNAME;
	else return NULL;
}

#ifdef SUNOGL
void glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value)
#else
int glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value)
#endif
{
	glXQueryDrawable(dpy, pbuf, attribute, value);
	#ifndef SUNOGL
	return 0;
	#endif
}

Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
	#ifdef USEGLP
	if(fconfig.glp) {*major=1;  *minor=4;  return True;}
	else
	#endif
	return _glXQueryVersion(_localdpy, major, minor);
}

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	if(winh.isoverlay(dpy, draw)) return _glXSelectEvent(dpy, draw, event_mask);

	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXSelectEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	glXSelectEvent(dpy, drawable, mask);
}

#ifdef SUNOGL
GLboolean glXGetTransparentIndexSUN(Display *dpy, Window overlay,
	Window underlay, unsigned int *transparentIndex)
#else
int glXGetTransparentIndexSUN(Display *dpy, Window overlay,
	Window underlay, long *transparentIndex)
#endif
{
	XWindowAttributes xwa;
	if(!transparentIndex) return False;

		opentrace(glXGetTransparentIndexSUN);  prargd(dpy);  prargx(overlay);
		prargx(underlay);  starttrace();

	if(fconfig.transpixel>=0)
		*transparentIndex=fconfig.transpixel;
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

Bool glXBindSwapBarrierNV(Display *dpy, GLuint group, GLuint barrier)
{
	return _glXBindSwapBarrierNV(_localdpy, group, barrier);
}

Bool glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier)
{
	return _glXQuerySwapGroupNV(_localdpy, ServerDrawable(dpy, drawable), group, barrier);
}

Bool glXQueryMaxSwapGroupsNV(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers)
{
	return _glXQueryMaxSwapGroupsNV(_localdpy, DefaultScreen(_localdpy), maxGroups, maxBarriers);
}

Bool glXQueryFrameCountNV(Display *dpy, int screen, GLuint *count)
{
	return _glXQueryFrameCountNV(_localdpy, DefaultScreen(_localdpy), count);
}

Bool glXResetFrameCountNV(Display *dpy, int screen)
{
	return _glXResetFrameCountNV(_localdpy, DefaultScreen(_localdpy));
}

int glXSwapIntervalSGI(int interval)
{
	if(fconfig.trace)
		rrout.print("[VGL] glXSwapIntervalSGI() [NOT SUPPORTED]\n");
	return 0;
}

void glXBindTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer,
	const int *attrib_list)
{
	_glXBindTexImageEXT(_localdpy, ServerDrawable(dpy, drawable), buffer,
		attrib_list);
}

void glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer)
{
	_glXReleaseTexImageEXT(_localdpy, ServerDrawable(dpy, drawable), buffer);
}

#define checkfaked(f) if(!strcmp((char *)procName, #f)) {  \
	retval=(void (*)(void))f;  if(fconfig.trace) rrout.print("[INTERPOSED]");}
#ifdef SUNOGL
#define checkfakedidx(f) if(!strcmp((char *)procName, #f)) retval=(void (*)(void))r_##f;
#else
#define checkfakedidx(f) checkfaked(f)
#endif

void (*glXGetProcAddressARB(const GLubyte *procName))(void)
{
	void (*retval)(void)=NULL;

	__vgl_fakerinit();

		opentrace(glXGetProcAddressARB);  prargs((char *)procName);  starttrace();

	if(procName)
	{
		checkfaked(glXGetProcAddressARB)
		checkfaked(glXGetProcAddress)

		checkfaked(glXChooseVisual)
		checkfaked(glXCopyContext)
		checkfaked(glXCreateContext)
		checkfaked(glXCreateGLXPixmap)
		checkfaked(glXDestroyContext)
		checkfaked(glXDestroyGLXPixmap)
		checkfaked(glXGetConfig)
		checkfaked(glXGetCurrentContext)
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
		checkfaked(glXGetCurrentReadDrawableSGI)
		checkfaked(glXGetFBConfigAttrib)
		checkfaked(glXGetFBConfigs)
		checkfaked(glXGetSelectedEvent)
		checkfaked(glXGetVisualFromFBConfig)
		checkfaked(glXMakeContextCurrent);
		checkfaked(glXMakeCurrentReadSGI)
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

		checkfaked(glXChooseFBConfigSGIX)
		checkfaked(glXCreateContextWithConfigSGIX)
		checkfaked(glXCreateGLXPixmapWithConfigSGIX)
		checkfaked(glXCreateGLXPbufferSGIX)
		checkfaked(glXDestroyGLXPbufferSGIX)
		checkfaked(glXGetFBConfigAttribSGIX)
		checkfaked(glXGetFBConfigFromVisualSGIX)
		checkfaked(glXGetVisualFromFBConfigSGIX)
		checkfaked(glXQueryGLXPbufferSGIX)
		checkfaked(glXSelectEventSGIX)
		checkfaked(glXGetSelectedEventSGIX)
		checkfaked(glXSwapIntervalSGI)

		checkfaked(glXGetTransparentIndexSUN)

		checkfaked(glXBindTexImageEXT)
		checkfaked(glXReleaseTexImageEXT)

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
		checkfakedidx(glIndexd)
		checkfakedidx(glIndexf)
		checkfakedidx(glIndexi)
		checkfakedidx(glIndexs)
		checkfakedidx(glIndexub)
		checkfakedidx(glIndexdv)
		checkfakedidx(glIndexfv)
		checkfakedidx(glIndexiv)
		checkfakedidx(glIndexsv)
		checkfakedidx(glIndexubv)
		checkfaked(glClearIndex)
		checkfaked(glGetDoublev)
		checkfaked(glGetFloatv)
		checkfaked(glGetIntegerv)
		checkfaked(glPixelTransferf)
		checkfaked(glPixelTransferi)
	}
	if(!retval)
	{
		if(fconfig.trace) rrout.print("[passed through]");
		if(__glXGetProcAddressARB) retval=_glXGetProcAddressARB(procName);
		else if(__glXGetProcAddress) retval=_glXGetProcAddress(procName);
	}

		stoptrace();  closetrace();

	return retval;
}

void (*glXGetProcAddress(const GLubyte *procName))(void)
{
	return glXGetProcAddressARB(procName);
}

}
