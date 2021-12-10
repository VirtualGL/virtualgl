// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011-2012, 2014-2015, 2019-2021 D. R. Commander
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

#ifndef __CONTEXTHASHEGL_H__
#define __CONTEXTHASHEGL_H__

#include "glxvisual.h"
#include "Hash.h"


typedef struct
{
	VGLFBConfig config;
	GLsizei nDrawBufs;
	GLenum drawBufs[16], readBuf;
	GLuint drawFBO, readFBO;
} EGLContextAttribs;


#define HASH  faker::Hash<EGLContext, void *, EGLContextAttribs *>

// This maps an EGLContext to a VGLFBConfig + context-specific attributes

namespace backend
{
	class ContextHashEGL : public HASH
	{
		public:

			static ContextHashEGL *getInstance(void)
			{
				if(instance == NULL)
				{
					util::CriticalSection::SafeLock l(instanceMutex);
					if(instance == NULL) instance = new ContextHashEGL;
				}
				return instance;
			}

			static bool isAlloc(void) { return instance != NULL; }

			void add(EGLContext ctx, VGLFBConfig config)
			{
				if(!ctx || !config) THROW("Invalid argument");
				EGLContextAttribs *attribs = NULL;
				attribs = new EGLContextAttribs;
				attribs->config = config;
				attribs->nDrawBufs = 0;
				for(int i = 0; i < 16; i++) attribs->drawBufs[i] = GL_NONE;
				attribs->readBuf = GL_NONE;
				attribs->drawFBO = attribs->readFBO = 0;
				HASH::add(ctx, NULL, attribs);
			}

			VGLFBConfig findConfig(EGLContext ctx)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) return attribs->config;
				return 0;
			}

			void setDrawBuffers(EGLContext ctx, GLsizei n, const GLenum *bufs)
			{
				if(n < 0 || n > 16 || !bufs) THROW("Invalid argument");
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs)
				{
					attribs->nDrawBufs = n;
					for(int i = 0; i < 16; i++) attribs->drawBufs[i] = GL_NONE;
					memcpy(attribs->drawBufs, bufs, sizeof(GLenum) * n);
				}
			}

			GLenum getDrawBuffer(EGLContext ctx, GLsizei index)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs && index >= 0 && index < attribs->nDrawBufs)
					return attribs->drawBufs[index];
				return GL_NONE;
			}

			GLenum *getDrawBuffers(EGLContext ctx, GLsizei &nDrawBufs)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs)
				{
					nDrawBufs = attribs->nDrawBufs;
					if(nDrawBufs)
					{
						GLenum *drawBufs = new GLenum[nDrawBufs];
						for(GLsizei i = 0; i < nDrawBufs; i++)
							drawBufs[i] = attribs->drawBufs[i];
						return drawBufs;
					}
				}
				nDrawBufs = 0;
				return NULL;
			}

			void setReadBuffer(EGLContext ctx, GLenum readBuf)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) attribs->readBuf = readBuf;
			}

			GLenum getReadBuffer(EGLContext ctx)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) return attribs->readBuf;
				return GL_NONE;
			}

			void setDrawFBO(EGLContext ctx, GLuint drawFBO)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) attribs->drawFBO = drawFBO;
			}

			GLuint getDrawFBO(EGLContext ctx)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) return attribs->drawFBO;
				return 0;
			}

			void setReadFBO(EGLContext ctx, GLuint readFBO)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) attribs->readFBO = readFBO;
			}

			GLuint getReadFBO(EGLContext ctx)
			{
				EGLContextAttribs *attribs = HASH::find(ctx, NULL);
				if(attribs) return attribs->readFBO;
				return 0;
			}

			void remove(EGLContext ctx)
			{
				if(ctx) HASH::remove(ctx, NULL);
			}

		private:

			~ContextHashEGL(void)
			{
				HASH::kill();
			}

			void detach(HashEntry *entry)
			{
				EGLContextAttribs *attribs = entry ? entry->value : NULL;
				delete attribs;
			}

			bool compare(EGLContext key1, void * key2, HashEntry *entry)
			{
				return false;
			}

			static ContextHashEGL *instance;
			static util::CriticalSection instanceMutex;
	};
}

#undef HASH


#define ctxhashegl  (*(backend::ContextHashEGL::getInstance()))

#endif  // __CONTEXTHASHEGL_H__
