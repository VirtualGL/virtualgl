// Copyright (C)2021 D. R. Commander
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

// This is a container class that allows us to temporarily save and restore the
// FBO and RBO bindings, draw buffer(s), and read buffer.

#ifndef __BUFFERSTATE_H__
#define __BUFFERSTATE_H__

#include <GL/gl.h>


#define BS_DRAWFBO  1
#define BS_READFBO  2
#define BS_RBO  4
#define BS_DRAWBUFS  8
#define BS_READBUF  16


namespace backend
{
	class BufferState
	{
		public:

			BufferState(int saveMask) : oldDrawFBO(-1), oldReadFBO(-1), oldRBO(-1),
				oldReadBuf(-1), nOldDrawBufs(0)
			{
				for(int i = 0; i < 16; i++) oldDrawBufs[i] = GL_NONE;

				if(saveMask & BS_DRAWFBO)
					_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawFBO);
				if(saveMask & BS_READFBO)
					_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadFBO);
				if(saveMask & BS_RBO)
					_glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRBO);
				if(saveMask & BS_DRAWBUFS)
				{
					GLint maxDrawBufs = 16;
					_glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBufs);
					if(maxDrawBufs > 16) maxDrawBufs = 16;
					for(int i = 0; i < maxDrawBufs; i++)
					{
						GLint drawBuf = GL_NONE;
						_glGetIntegerv(GL_DRAW_BUFFER0 + i, &drawBuf);
						if(drawBuf != GL_NONE)
							oldDrawBufs[nOldDrawBufs++] = drawBuf;
					}
				}
				if(saveMask & BS_READBUF)
					_glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
			}

			GLint getOldReadFBO() { return oldReadFBO; }

			~BufferState(void)
			{
				if(oldDrawFBO >= 0)
					_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawFBO);
				if(oldReadFBO >= 0)
					_glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadFBO);
				if(oldRBO >= 0) _glBindRenderbuffer(GL_RENDERBUFFER, oldRBO);
				if(nOldDrawBufs > 0) _glDrawBuffers(nOldDrawBufs, oldDrawBufs);
				if(oldReadBuf >= 0) _glReadBuffer(oldReadBuf);
			}

		private:

			GLint oldDrawFBO, oldReadFBO, oldRBO, oldReadBuf;
			GLsizei nOldDrawBufs;
			GLenum oldDrawBufs[16];
	};
}

#endif  // __BUFFERSTATE_H__
