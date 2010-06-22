/* This defines the necessary constants and prototypes for the
   GL_EXT_framebuffer_object extension, since not all platforms define
   these (even when the extension is supported) */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GL_COLOR_ATTACHMENT0_EXT
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#endif

#ifndef GL_DRAW_FRAMEBUFFER_EXT
#define GL_DRAW_FRAMEBUFFER_EXT           0x8CA9
#endif

#ifndef GL_DRAW_FRAMEBUFFER_BINDING_EXT
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT   0x8CA6
#endif

#ifndef GL_FRAMEBUFFER_EXT
#define GL_FRAMEBUFFER_EXT                0x8D40
#endif

#ifndef GL_READ_FRAMEBUFFER_EXT
#define GL_READ_FRAMEBUFFER_EXT           0x8CA8
#endif

#ifndef GL_READ_FRAMEBUFFER_BINDING_EXT
#define GL_READ_FRAMEBUFFER_BINDING_EXT   0x8CAA
#endif

#ifndef GL_RENDERBUFFER_EXT
#define GL_RENDERBUFFER_EXT               0x8D41
#endif

#ifndef GL_EXT_framebuffer_object
extern void glBindFramebufferEXT(GLenum, GLuint);
extern void glBindRenderbufferEXT(GLenum, GLuint);
extern void glDeleteFramebuffersEXT(GLsizei, const GLuint *);
extern void glDeleteRenderbuffersEXT(GLsizei, const GLuint *);
extern void glFramebufferRenderbufferEXT(GLenum, GLenum, GLenum, GLuint);
extern void glGenFramebuffersEXT(GLsizei, GLuint *);
extern void glGenRenderbuffersEXT(GLsizei, GLuint *);
extern void glRenderbufferStorageEXT(GLenum, GLenum, GLsizei, GLsizei);
#endif

#ifdef __cplusplus
}
#endif
