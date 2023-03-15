3.1
===

### Significant changes relative to 3.1 beta1:

1. Fixed an issue in the EGL back end whereby textures and other OpenGL objects
were not automatically destroyed along with the context and drawable in which
the objects were created.

2. Added an environment variable (`VGL_EXITFUNCTION`) that, when set to
`_exit` or `abort`, causes the VirtualGL Faker to call the specified function
rather than `exit()` when a non-recoverable error occurs.

3. Fixed an issue whereby the interposed `eglCreatePlatformWindowSurface()`
and `eglCreatePlatformWindowSurfaceEXT()` functions incorrectly treated the
native window argument as an X window handle rather than a pointer to an X
window handle.  This caused a segfault in VLC when using the OpenGL video
output module.

4. `vglserver_config` now works properly with SUSE Linux Enterprise/openSUSE
Leap 15.

5. If the GLX back end is in use, then the interposed `eglGetDisplay()` and
`eglGetPlatformDisplay()` functions now return `EGL_NO_DISPLAY` rather than
throwing a fatal error.  This allows applications such as Firefox to fail
gracefully or to fall back and use the GLX API if EGL/X11 is unavailable.

6. Fixed an issue whereby the VirtualGL Configuration dialog did not pop up if
the X keyboard extension was enabled on the 2D X server.

7. The VirtualGL Faker no longer probes the 2D X server for stereo visuals
unless the VGL Transport or a transport plugin will be used.  Even if the 2D X
server has stereo visuals, they will never be used with the X11 and XV
Transports.  Probing the 2D X server for stereo visuals causes problems with
certain OpenGL implementations and with applications, such as Tecplot 360, that
include static builds of Mesa.  An undocumented environment variable
(`VGL_PROBEGLX`) can be used to override the default behavior.

8. When using the EGL back end, interposed `XQueryExtension(..., "GLX", ...)`
and `glXQueryExtension()` function calls now return `False`, rather than
throwing a fatal error, if the 2D X server does not have a GLX extension.  (The
EGL back end uses the 2D X server's GLX extension for GLX error handling.)
This allows applications, such as Chrome/Chromium, to fail gracefully or use a
different API (such as EGL/X11) if the VirtualGL Faker is unable to emulate
GLX.

9. The VirtualGL Client now runs on Macs with Apple silicon CPUs (without
requiring Rosetta 2.)


3.0.90 (3.1 beta1)
==================

### Significant changes relative to 3.0.2:

1. The `vglconnect -x` and `vglconnect -k` options have been retired in this
version of VirtualGL and will continue to be maintained in the 3.0.x branch on
a break/fix basis only.  Those options, which had been undocumented since
VirtualGL 2.6.1, were a throwback to the early days of VirtualGL when SSH was
not universally available and SSH X11 forwarding sometimes introduced a
performance penalty.  `vglconnect -x` did not work with most modern operating
systems, since most modern operating systems disable X11 TCP connections by
default.

2. SSL encryption of the VGL Transport has been retired in this version of
VirtualGL and will continue to be maintained in the 3.0.x branch on a break/fix
basis only.  That feature, which had not been included in official VirtualGL
packages since VirtualGL 2.1.x, was not widely used.  Furthermore, SSL
encryption of the VGL Transport had no performance advantages over SSH
tunneling on modern systems, and it had some security disadvantages due to its
reliance on the RSA key exchange algorithm (which made it incompatible with TLS
v1.3.)

3. When using the EGL back end, VirtualGL now supports 3D applications
(including recent versions of Google Chrome/Chromium and Mozilla Firefox) that
use the EGL/X11 API instead of the GLX API.  As of this writing, VirtualGL does
not support EGL pixmap surfaces or front buffer/single buffer rendering with
EGL window surfaces.

4. On Un*x and Mac clients, `vglconnect` now uses the OpenSSH `ControlMaster`
option to avoid the need to authenticate with the server multiple times when
tunneling the VGL Transport through SSH.

5. `vglconnect` now accepts a new command-line argument (`-v`) that, when
combined with `-s`, causes VirtualGL to be preloaded into all processes
launched in the remote shell, thus eliminating the need to invoke `vglrun`.


3.0.2
=====

### Significant changes relative to 3.0.1:

1. Fixed an issue whereby `vglserver_config` failed to unload the `nvidia`
module when using recent versions of nVidia's proprietary drivers.  In some
cases, this led to incorrect device permissions for **/dev/nvidia*** after the
display manager was restarted.

2. Fixed compilation errors when building with libX11 1.8.x.

3. When using the EGL back end, a GPU can now be specified by an EGL device ID
of the form `egl{n}`, where `{n}` is a zero-based index, or by a DRI device
path.  A list of valid EGL device IDs and their associated DRI device paths can
be obtained by running `/opt/VirtualGL/bin/eglinfo -e`.

4. Fixed an issue in the EGL back end whereby `glBindFramebuffer()`, when used
to bind the default framebuffer, did not restore the previous draw/read buffer
state for the default framebuffer.  This issue was known to cause flickering in
Webots when rendering a camera view, and it may have affected other
applications as well.


3.0.1
=====

### Significant changes relative to 3.0:

1. GLXSpheres now includes an option (`-si`) that can be used to specify the
swap interval via the `GLX_EXT_swap_control` and `GLX_SGI_swap_control`
extensions.

2. Fixed an issue in the EGL back end whereby `glXCreateContextAttribsARB()`
always returned NULL if `GLX_RENDER_TYPE` was specified in the attribute list.
This issue was known to affect JOGL applications but may have affected other
applications as well.

3. The EGL back end now supports the `GLX_EXT_framebuffer_sRGB` extension,
which is necessary for OpenGL 3.0 conformance.  This fixes an issue whereby
frames rendered by 3D applications using the sRGB color space appeared too
dark.

4. The VirtualGL Faker now interposes XCB window creation and destruction
functions.  This fixes an issue whereby, if an X window was created using the
XCB API and subsequently attached to an OpenGL context using
`glXMake[Context]Current()`, the off-screen buffer and other faker resources
associated with that window were not freed until the 3D application exited or
closed the X display connection associated with the window.  This issue was
known to affect Qt5 applications.

5. `vglserver_config` should now work properly with SDDM if its scripts are
installed in **/etc/sddm**, which is the case when SDDM is installed through
EPEL.

6. Fixed several issues in the EGL back end that caused OpenGL errors to be
generated by the interposed `glXMake*Current()` and `glXSwapBuffers()`
functions.  These OpenGL errors sometimes caused fatal errors when closing Qt
applications, and they may have affected other types of applications and use
cases as well.


3.0
===

### Significant changes relative to 3.0 beta1:

1. Worked around an issue with Red Hat Enterprise Linux 8 and work-alike
operating systems whereby, if X11 forwarding was enabled by default in the SSH
client, `vglconnect -s` would hang while making a preliminary SSH connection to
find a free port on a server running one of those operating systems.

2. Fixed an issue in the EGL back end whereby `glReadPixels()` did not work
with multisampled drawables.

3. Fixed an issue in the EGL back end whereby the `glGet*()` functions
sometimes returned an incorrect value for `GL_DRAW_BUFFER0` through
`GL_DRAW_BUFFER15` if the correct value was `GL_NONE`.

4. The VirtualGL Faker now interposes the `glXGetCurrentDisplayEXT()` function,
which is part of the `GLX_EXT_import_context` extension.  (The
`GLX_EXT_import_context` extension can be used with the GLX back end if the
underlying OpenGL library and 3D X server support the extension and if indirect
OpenGL contexts are enabled using `VGL_ALLOWINDIRECT`.)

5. Fixed an issue in the EGL back end whereby the `glXChooseFBConfig()`
function returned no FB configs if the `GLX_DRAWABLE_TYPE` attribute was set to
`GLX_DONT_CARE`.

6. Fixed an issue in the EGL back end whereby the `glXSwapBuffers()` function
erroneously swapped the buffers of double-buffered Pixmaps.

7. Fixed an issue in the EGL back end that caused the bounding box in 3D Slicer
to be displayed on top of the rendered volume when "Display ROI" and depth
peeling were both enabled.

8. The EGL back end now supports OpenGL applications that simultaneously make
the same GLX drawable current in more than one thread.  As a consequence, the
EGL back end now requires the `EGL_KHR_no_config_context` and
`EGL_KHR_surfaceless_context` extensions.

9. Fixed an issue in the EGL back end whereby the OpenGL 4.5 and
`GL_EXT_direct_state_access` named framebuffer functions did not work properly
with the default framebuffer unless it was currently bound.

10. Fixed issues in the EGL back end whereby
`glGetFramebufferAttachmentParameteriv()`,
`glGetFramebufferParameteriv(..., GL_DOUBLEBUFFER, ...)`,
`glGetFramebufferParameteriv(..., GL_STEREO, ...)`, and
`glGetNamedFramebufferParameteriv()` did not return correct values for the
default framebuffer and whereby `glGet*(GL_DOUBLEBUFFER, ...)` and
`glGet*(GL_STEREO, ...)` did not return correct values for framebuffer objects.

11. By default, VirtualGL now generates a 2048-bit RSA key for use with SSL
encryption.  This fixes an error ("ee key too small") that occurred when
attempting to launch the VirtualGL Client on systems configured with a default
SSL/TLS security level of 2.  A new CMake variable (`VGL_SSLKEYLENGTH`) can be
used to restore the behavior of previous releases of VirtualGL (generating a
1024-bit RSA key) or to increase the key length for additional security.

12. VirtualGL's built-in SSL encryption feature now works with OpenSSL v1.1.1
and later.


2.6.90 (3.0 beta1)
==================

### Significant changes relative to 2.6.5:

1. Support for transparent overlay visuals has been retired in this version of
VirtualGL.  That feature will continue to be maintained in the 2.6.x branch on
a break/fix basis only.  Most applications that once used transparent overlay
visuals used them with color index rendering, which was removed in OpenGL 3.1
in 2009.  Thus, almost all applications that render overlays now do so using
other mechanisms.  Furthermore, the need for VirtualGL to hand off the
rendering of transparent overlay visuals to the 2D X server has always limited
the usefulness of the feature, and the discontinuation of the VirtualGL Client
for Exceed relegated the feature to Un*x clients (with workstation-class GPUs)
and the VGL Transport only.  Given that nVidia's implementation of transparent
overlay visuals requires disabling the X Composite extension, which cannot be
done in many modern Linux distributions, that further limited the feature to
the point of uselessness.

2. The VirtualGL Faker now assigns various permutations of common OpenGL
rendering attributes to the available 2D X server visuals.  This maximizes the
chances that "visual hunting" 3D applications (applications that use X11
functions to obtain a list of 2D X server visuals, then iterate through the
list with `glXGetConfig()` until they find a visual with a desired set of
OpenGL rendering attributes) will find a suitable visual.
`VGL_DEFAULTFBCONFIG` can still be used to assign a specified set of OpenGL
rendering attributes to all 2D X server visuals, although the usefulness of
that feature is now very limited.

3. The VirtualGL Faker now includes an EGL back end that optionally emulates
the GLX API using a combination of the EGL API (with the
`EGL_EXT_platform_device` extension) and OpenGL renderbuffer objects (RBOs.)
On supported platforms, the EGL back end allows the VirtualGL Faker to be used
without a 3D X server.  The EGL back end can be activated by setting the
`VGL_DISPLAY` environment variable to the path of a DRI device, such as
**/dev/dri/card0**, or by passing that device path to `vglrun` using the `-d`
argument.  Some obsolete OpenGL and GLX features are not supported by the EGL
back end:
    - `glXCopyContext()`
    - Accumulation buffers
    - Aux buffers
    - Indirect OpenGL contexts

    The EGL back end also does not currently support the following GLX
extensions:

    - `GLX_ARB_create_context_robustness`
    - `GLX_ARB_fbconfig_float`
    - `GLX_EXT_create_context_es2_profile`
    - `GLX_EXT_fbconfig_packed_float`
    - `GLX_EXT_framebuffer_sRGB`
    - `GLX_EXT_import_context`
    - `GLX_EXT_texture_from_pixmap`
    - `GLX_NV_float_buffer`

    Those extensions are supported by the GLX back end if the 3D X server
supports them.  The EGL back end also requires a 2D X server with a GLX
extension.

4. Fixed an issue whereby, when the 2D X server was using a "traditional"
window manager (such as TWM) or no window manager and the X11 Transport was not
using the `MIT-SHM` X extension (because the extension was unavailable or
explicitly disabled/bypassed, because the X connection was remote, or because
the 3D application and the 2D X server did not share an IPC namespace),
unhandled X NoExpose events would continue to queue up in Xlib until the 3D
application exited or the available memory on the VirtualGL server was
exhausted.


2.6.5
=====

### Significant changes relative to 2.6.4:

1. Fixed a race condition that sometimes caused various fatal errors in the
interposed `glXMakeContextCurrent()` function if both GLX drawable IDs passed
to that function were the same window handle and the corresponding X window was
simultaneously resized in another thread.

2. Fixed an oversight whereby the addresses of the interposed
`glDrawBuffers()`, `glGetString()`, and `glGetStringi()` functions introduced
in 2.6.3[2] and 2.6.4[1] were not returned from the interposed
`glXGetProcAddress()` and `glXGetProcAddressARB()` functions.

3. VirtualGL now works properly with 3D applications that use the
`glNamedFramebufferDrawBuffer()` and `glNamedFramebufferDrawBuffers()`
functions (OpenGL 4.5) or the `glFramebufferDrawBufferEXT()` and
`glFramebufferDrawBuffersEXT()` functions (`GL_EXT_direct_state_access`) and
render to the front buffer.

4. Fixed a BadRequest X11 error that occurred when attempting to use the X11
Transport with a remote X connection.

5. Worked around an issue with certain GLX implementations that list
10-bit-per-component FB configs ahead of 8-bit-per-component FB configs and
incorrectly set `GLX_DRAWABLE_TYPE|=GLX_PIXMAP_BIT` for those 10-bpc FB
configs, even though they have no X visuals attached.  This caused VirtualGL's
interposed `glXChooseVisual()` function to choose one of the 10-bpc FB configs
behind the scenes, which made it impossible to use the VGL Transport.

6. Fixed an issue whereby, when using the X11 Transport, a vertically flipped
image of a previously-rendered frame was sometimes displayed if the 3D
application called `glFlush()` while the front buffer was the active drawing
buffer and the render mode was `GL_FEEDBACK` or `GL_SELECT`.

7. `vglserver_config` now works properly if invoked with a relative path
(for example, `cd /opt/VirtualGL/bin; sudo ./vglserver_config`.)

8. Worked around a limitation in the AMDGPU drivers that prevented recent
versions of Google Chrome from enabling GPU acceleration when used with
VirtualGL.


2.6.4
=====

### Significant changes relative to 2.6.3:

1. VirtualGL now works properly with 3D applications that use the
`glDrawBuffers()` function and render to the front buffer.

2. VirtualGL can now be built using the GLX headers from Mesa 19.3.0 and later.

3. VirtualGL now works properly with 3D applications that use `dlopen()` to
load libGLX or libOpenGL rather than libGL.

4. Extended the image transport plugin API to accommodate GPU-based
post-processing and compression of rendered frames as well as alternative
methods of framebuffer readback.  See the descriptions of `RRTransInit()`
and `RRTransGetFrame()` in **[server/rrtransport.h](server/rrtransport.h)** for
more details.

5. The shared memory segment created by the VirtualGL Faker for use by
vglconfig (the application responsible for displaying the VirtualGL
Configuration dialog) is now automatically removed on FreeBSD systems if the 3D
application exits prematurely.

6. Fixed an issue whereby the VirtualGL Faker incorrectly attempted to read
back the current drawable rather than the specified GLX Pixmap when
synchronizing the GLX Pixmap with its corresponding 2D Pixmap in the body of
the interposed `glXDestroyGLXPixmap()` and `glXDestroyPixmap()` functions.
This caused various errors if the current drawable had already been destroyed
or if the current drawable and the specified GLX Pixmap were created with
incompatible visuals/FB configs.

7. Fixed an error ("dyld: Library not loaded: /usr/X11/lib/libGL.1.dylib") that
occurred when attempting to run the VirtualGL Client on macOS Catalina.


2.6.3
=====

### Significant changes relative to 2.6.2:

1. VirtualGL now enables the `GLX_EXT_create_context_es2_profile` extension
if the underlying OpenGL library and 3D X server support it.

2. The VirtualGL Faker now ensures that the `GL_EXT_x11_sync_object` extension
is not exposed to 3D applications, even if the underlying OpenGL library
supports it.  `GL_EXT_x11_sync_object` allows `glWaitSync()` and
`glClientWaitSync()` to block on X11 Synchronization Fence (XSyncFence)
objects, but since XSyncFence objects live on the 2D X server, this extension
does not currently work with VirtualGL.  Emulating the extension would be
difficult and would have similar drawbacks to those of the existing `VGL_SYNC`
option.  Preventing `GL_EXT_x11_sync_object` from being exposed to 3D
applications fixes numerous OpenGL errors that occurred when attempting to run
GNOME 3 with VirtualGL using nVidia's proprietary drivers.

3. The VirtualGL Faker now optionally interposes the `clCreateContext()` OpenCL
function in order to replace the value of the `CL_GLX_DISPLAY_KHR` property (if
specified) with the X11 Display handle for the 3D X server connection.  This
prevents 3D applications from crashing when attempting to use OpenCL/OpenGL
interoperability functions with VirtualGL.  The new OpenCL interposer is
enabled by passing `+ocl` to `vglrun` or by setting the `VGL_FAKEOPENCL`
environment variable to `1`.

4. VirtualGL no longer provides in-tree GLX headers.  Traditionally, these were
provided because, since VirtualGL is a GLX emulator, it has the ability to
support certain newer GLX features that aren't available in the underlying
libGL implementation.  However, the operating systems that lack these GLX
features are, for the most part, EOL.  In general, VirtualGL can now only be
built on systems that have Mesa 9 or later, or the equivalent.

5. VirtualGL no longer provides in-tree XCB headers.  Traditionally, these were
provided because, since VirtualGL is an XCB emulator, it has the ability to
support certain newer XCB features that aren't available in the underlying
libxcb implementation.  However, the operating systems that lack these XCB
features are, for the most part, EOL.

6. `vglserver_config` now works properly with FreeBSD systems running GDM v3.


2.6.2
=====

### Significant changes relative to 2.6.1:

1. Fixed a regression introduced by 2.6 beta1[4] that prevented VirtualGL from
building on 64-bit ARM (AArch64) platforms.

2. `vglserver_config` now works properly with Wayland-enabled Linux systems
running GDM, with the caveat that configuring such systems as VirtualGL servers
will disable the ability to log in locally with a Wayland session.

3. Fixed a BadMatch X11 error that occurred when an application attempted to
use `glXUseXFont()` while rendering to a Pbuffer and displaying to a 2D X
server screen other than 0.

4. Fixed several minor memory leaks in the VirtualGL Faker.

5. VirtualGL now enables the `GLX_ARB_create_context_robustness`,
`GLX_ARB_fbconfig_float`, `GLX_EXT_fbconfig_packed_float`,
`GLX_EXT_framebuffer_sRGB`, and `GLX_NV_float_buffer` extensions if the
underlying OpenGL library and 3D X server support them.

6. The VirtualGL Faker now prints all warning messages, notices, profiling
output, and tracing output to stdout if the `VGL_LOG` environment variable is
set to `stdout`.

7. Double buffering can now be disabled in the default FB config (which is used
whenever a 3D application passes a visual with an unknown set of OpenGL
rendering attributes to `glXCreateContext()`, `glXGetConfig()`, or
`glXCreateGLXPixmap()`) by adding `GLX_DOUBLEBUFFER,0` to the
`VGL_DEFAULTFBCONFIG` environment variable.

8. The `VGL_FORCEALPHA` and `VGL_SAMPLES` environment variables now affect the
default FB config.

9. Fixed numerous minor issues with the VirtualGL Faker's visual matching
algorithms.  These issues mostly affected DirectColor rendering, 2D X servers
with multiple screens, and 2D X servers with a different default depth than the
3D X server.

10. Error handling in the VirtualGL Faker is now more conformant with the GLX
spec.  More specifically, when erroneous arguments are passed to interposed GLX
functions, the faker now sends GLX errors through the X11 error handler rather
than throwing fatal exceptions.


2.6.1
=====

### Significant changes relative to 2.6:

1. Fixed a regression introduced by 2.6 beta1[8] that caused a fatal error
("ERROR: in findConfig-- Invalid argument") to be thrown if a 3D application
called a frame trigger function, such as `glXSwapBuffers()`, when no OpenGL
context was current.

2. Worked around a segfault in Cadence Virtuoso that occurred when using
VirtualGL's `dlopen()` interposer (**libdlfaker.so**).  The application
apparently interposes `getenv()` and calls `dlopen()` within the body of its
interposed `getenv()` function.  Since VirtualGL calls `getenv()` within the
body of its interposed `dlopen()` function, this led to infinite recursion and
stack overflow.

    The workaround for this segfault also fixed an error ("Failed to hook the
dlopen interface") that occurred when using recent versions of VirtualBox with
VirtualGL.

3. The VirtualGL User's Guide and other documentation has been overhauled to
reflect current reality and to use more clear and consistent terminology and
formatting.


2.6
===

### Significant changes relative to 2.6 beta1:

1. Fixed an issue whereby the modifications that `vglserver_config` made to
**lightdm.conf** sometimes caused `vglgenkey` to be executed multiple times.
This was known to cause problems with remote login methods, such as XDMCP, and
it also caused LightDM on Ubuntu 18.04 to lock up when logging in locally.


2.5.90 (2.6 beta1)
==================

### Significant changes relative to 2.5.2:

1. VirtualGL began handling `WM_DELETE_WINDOW` protocol messages in version
2.1.3 as a way of avoiding BadDrawable errors that would occur if the 3D
application was closed by the window manager (using the window's close gadget,
for instance) and the application did not handle `WM_DELETE_WINDOW` messages
itself.  VirtualGL throws the following error if such a situation occurs:

        [VGL] ERROR: in {method}--
        [VGL]    {line}: Window has been deleted by window manager

    However, this error was also thrown erroneously by VirtualGL if a 3D
application handled `WM_DELETE_WINDOW` messages but did not subsequently shut
down its X11 event queue.  This was known to affect GLFW applications but
likely affected other applications as well.  To address this problem, VirtualGL
now ignores `WM_DELETE_WINDOW` messages if it detects that the 3D application
is handling them.

2. Fixed an issue whereby the VGL logo would be drawn incorrectly or the
3D application would segfault if the OpenGL window was smaller than 74x29,
`VGL_LOGO` was set to `1`, and quad-buffered stereo was enabled.

3. Fixed an issue whereby the `glxinfo` utility included in the VirtualGL
packages would fail with a GLXBadContext error when used in an indirect OpenGL
environment.

4. VirtualGL can now be used with depth=30 (10-bit-per-component) visuals and
FB configs, if both the 2D and 3D X servers support them.  Currently this only
works with the X11 Transport, since other transports do not (yet) have the
ability to transmit and display 30-bit pixels.  Furthermore, anaglyphic stereo
does not work with this feature.

5. Worked around a segfault in Mayavi2 (and possibly other VTK applications)
when using nVidia's proprietary drivers.  VirtualGL's implementation of
`glXGetVisualFromFBConfig()` always returned a TrueColor X visual, regardless
of the GLXFBConfig passed to it.  From VirtualGL's point of view, this was
correct behavior, because VGL always converts the rendered frame to TrueColor
when reading it back.  However, some use cases in VTK assume that there is a
1:1 correspondence between X visuals and GLX FB configs, which is not the case
when using VirtualGL.  Thus, VGL's implementation of
`glXGetVisualFromFBConfig()` was causing VTK's visual matching algorithm to
always select the last FB config in the list, and when using nVidia hardware,
that FB config usually describes an esoteric floating point pixel format.  When
VTK subsequently attempted to create a context with this FB config and make
that context current, VirtualGL's internal call to `glXMakeContextCurrent()`
failed.  VGL passed this failure status back to VTK in the return value of
`glXMakeCurrent()`, but VTK ignored it, and the lack of a valid context led to
a subsequent segfault when VTK tried to call `glBlendFuncSeparate()`.
VirtualGL's implementation of `glXGetVisualFromFBConfig()` now returns NULL
unless the FB config has a corresponding visual on the 3D X server.

6. VirtualGL can now be built and run with OpenSSL 1.1.

7. The VGL Transport now works properly with IPv6 sockets.  When `-ipv6` is
passed to `vglconnect`, or the `VGLCLIENT_IPV6` environment variable is set to
`1`, the VirtualGL Client will accept both IPv4 and IPv6 connections.

8. Worked around a segfault that would occur, when using VirtualGL with the
non-GLVND-enabled proprietary nVidia OpenGL stack, if an OpenGL application
called `glXDestroyContext()` to destroy an active OpenGL context in which the
front buffer was the active drawing buffer, then the application called
`glXMake[Context]Current()` to swap in another context.  Under those
circumstances, VirtualGL will read back the front buffer within the body of the
interposed `glXMake[Context]Current()` function, in order to transport the
contents of the last frame that was rendered to the buffer.  Since VirtualGL
temporarily swaps in its own OpenGL context prior to reading back the buffer,
then swaps the application's OpenGL context back in after the readback,
VirtualGL was passing a dead context to nVidia's `glXMake[Context]Current()`
function, which triggered the segfault.  Arguably nVidia's
`glXMake[Context]Current()` implementation should have instead thrown a
GLXBadContext error, but regardless, VirtualGL now verifies that the
application's OpenGL context is alive before swapping it back in following a
readback.  This issue was known to affect ParaView but may have affected other
applications as well.


2.5.2
=====

### Significant changes relative to 2.5.1:

1. Previously, the VirtualGL Faker opened a connection to the 3D X server
whenever an application called `XOpenDisplay()`.  The faker now waits until the
3D X server is actually needed before opening the connection.  This prevents
non-OpenGL X11 applications from opening unnecessary connections to the 3D X
server, which could exhaust the X server's limit of 256 connections if many
users are sharing the system.

2. Fixed a regression caused by 2.4.1[6] whereby applications launched with
VirtualGL on nVidia GPUs would fail to obtain a visual if `VGL_SAMPLES` was
greater than 0.  Multisampling cannot be used with Pixmap rendering, and
because nVidia's drivers export different FB configs for `GLX_PBUFFER_BIT` and
`GLX_PBUFFER_BIT|GLX_PIXMAP_BIT`, it is necessary to specify `GLX_PBUFFER_BIT`
to obtain an FB config that supports multisampling.

3. Fixed a regression caused by 2.4 beta1[2] whereby 32-bit Linux builds of
VirtualGL built with recent compilers would sometimes crash when exiting
certain 3D applications (reported to be the case with Steam) or behave in other
unpredictable ways.

4. Fixed an issue whereby VirtualGL, when used with applications that load
OpenGL functions via `dlopen()`/`dlsym()`, would fail to load the "real"
GLX/OpenGL functions from libGL if **libvglfaker.so** was built with GCC 4.6 or
later.

5. Fixed various build issues with Clang.

6. The interposed `dlopen()` function in the Linux version of **libdlfaker.so**
will now nullify the `RTLD_DEEPBIND` flag, if an application passes that flag
to `dlopen()`.  This prevents an issue whereby an application could call
`dlopen(..., *|RTLD_DEEPBIND)` to load a shared library that uses OpenGL or
X11, thus causing the "real" OpenGL/GLX/X11 functions loaded by that shared
library to supercede VirtualGL's interposed functions, effectively disabling
VirtualGL.

7. Fixed an issue whereby VirtualGL would crash with a GLXBadContextState error
if the 3D application set the render mode to something other than `GL_RENDER`
prior to calling `glXSwapBuffers()`.


2.5.1
=====

### Significant changes relative to 2.5:

1. VirtualGL will no longer report the presence of the `GLX_EXT_swap_control`,
`GLX_EXT_texture_from_pixmap`, or `GLX_SGI_swap_control` extensions to
applications unless the underlying OpenGL library exports the necessary
functions to support those extensions.  This fixes a regression introduced in
VGL 2.4 that caused WINE to crash when running on a system whose OpenGL
implementation lacked the `glXSwapIntervalSGI()` and `glXSwapIntervalEXT()`
functions.  Furthermore, VirtualGL will now report the presence of the
`GLX_EXT_import_context` and `GLX_NV_swap_group` extensions to applications if
the underlying OpenGL library exports the necessary functions to support those
extensions.

2. Fixed compilation errors when building with GCC v6.

3. `vglserver_config` is now SELinux-aware and will set up the proper file
contexts to allow `vglgenkey` to run within the GDM startup scripts.  This has
been verified with Red Hat Enterprise Linux and work-alike systems (CentOS,
etc.), but unfortunately the version of GDM that ships in Fedora 22-24 does not
execute the GDM startup scripts at all.  At the moment, the only workaround for
those recent Fedora releases is to use LightDM.

4. Fixed a deadlock that occurred when exiting ANSYS HFSS 2014.  This fix was
an extension of 2.3.3[2], necessitated by the fact that MainWin calls X11
functions from the destructor of one of its shared libraries, which is executed
after the VirtualGL Faker has shut down.  Because VGL 2.5.x enables the XCB
interposer by default, we have to ensure that any X11 and XCB functions hand
off immediately to the underlying libraries after the faker has been shut down,
because even if an X11 function is not interposed by VGL, some of the XCB
functions it calls might be.  This issue was also known to affect ANSYS Maxwell
and may have affected other applications that use MainWin.


2.5
===

### Significant changes relative to 2.5 beta1:

1. OS X 10.11 "El Capitan" no longer allows packages to install files under
**/usr/bin**, and this was preventing the VirtualGL binary package for OS X
from installing on that platform.  The symlinks to `vglclient` and `vglconnect`
that the OS X package previously installed under **/usr/bin** have thus been
removed in this version of VirtualGL.  It will therefore be necessary to invoke
`vglconnect` and `vglclient` using the full pathname
(`/opt/VirtualGL/bin/vglconnect` or `/opt/VirtualGL/bin/vglclient`) or to add
**/opt/VirtualGL/bin** to the `PATH`.

2. Fixed a regression introduced by 2.5 beta1[13] that caused certain system
commands (such as uname, hostname, etc.) to crash when running those commands
using `vglrun` on current Arch Linux spins (glibc 2.22, GCC 5.2.)  This
possibly affected other non-OpenGL, non-X11 applications on other bleeding-edge
Linux distributions as well.

3. `vglserver_config` should now work properly with MDM (MATE Display Manager),
if its config files are installed in the standard location (**/etc/mdm**).

4. Fixed a regression introduced in 2.4 that caused `vglrun` to abort with
"VGL_ISACTIVE=1: is not an identifier" when running on Solaris 10 (or other
systems in which `/bin/sh` doesn't support `export VAR=value` syntax.)

5. Fixed a regression introduced by 2.5 beta1[3] whereby the VirtualGL Faker
would segfault on Solaris if the 3D application called one of the GLX/OpenGL
functions that VirtualGL interposes and the underlying OpenGL library (libGL)
did not implement that function.


2.4.90 (2.5 beta1)
==================

### Significant changes relative to 2.4.1:

1. **librrfaker.so** has been renamed to **libvglfaker.so**.  The "rr"
designation dates from before VirtualGL was called "VirtualGL" (i.e. before it
became an open source project), and it is no longer relevant.

2. The symlinks that VirtualGL previously installed for Chromium (the
long-obsolete parallel rendering package, not the web browser) are no longer
included in this release.

3. The mechanism by which VirtualGL loads "real" GLX, OpenGL, X11, and XCB
functions from the respective system libraries has been refactored.  This has
the following ramifications:

    - The "real" version of an interposed GLX/OpenGL/X11/XCB function will
never be loaded until the interposed function is called.
    - `glXProcAddress[ARB]()` is now used to load all "real" GLX/OpenGL
functions from libGL (except for the `glXProcAddress[ARB]()` function itself.)
This maintains the fixes implemented in 2.4[9] as well as the previous
work-arounds for certain buggy libGL implementations that do not export the
`glXBindTexImageEXT()` and `glXReleaseTexImageEXT()` functions in the libGL
symbol table (the latter was specifically known to be an issue with certain
versions of the ATI driver under Ubuntu.)
    - If `-nodl` is passed to `vglrun`, then libGL will not be loaded into the
application process until/unless the application actually calls a GLX/OpenGL
function.
    - Because XCB functions are now loaded only when needed, the XCB interposer
is no longer distribution-specific.  Therefore, it is now included in the
official VirtualGL binaries.  Furthermore, since the loading and interposing of
XCB symbols is now less intrusive, the XCB interposer is enabled by default (it
can be disabled with `vglrun -xcb`.)
    - Reverted 2.1.3[8], since that fallback mechanism is no longer necessary
with modern versions of the nVidia driver.  The issue in question can still be
worked around by explicitly setting `VGL_GLLIB` and `VGL_X11LIB`.

4. Support for color index rendering emulation has been retired in this version
of VirtualGL.  That feature will continue to be maintained in the 2.4.x branch
on a break/fix basis only.  Even when color index emulation was implemented in
VGL 10 years ago, the applications that needed it were already extremely rare.
Since then, color index rendering has been officially obsoleted in the OpenGL
spec (as of version 3.1 in 2009), and modern Un*x graphics drivers no longer
support it, nor do they generally support PseudoColor X visuals (nVidia still
supports these, but only with transparent overlays.)  Since there is generally
no reasonable way to run color index OpenGL applications without using legacy
hardware or software, it does not make sense to continue supporting these
applications in this version of VirtualGL, particularly given that color index
emulation adds a certain amount of overhead to some OpenGL calls.

5. Added support for DirectColor rendering (DirectColor is similar to
PseudoColor, except that the colormap indices for red, green, and blue can be
specified separately.)  GLXSpheres now includes a DirectColor mode, replacing
its previous color index mode.

6. VirtualGL can now be disabled on a display-by-display basis by specifying a
list of X displays to exclude (see the documentation for the `VGL_EXCLUDE`
environment variable.)  This is useful with multi-GPU systems on which a single
application may want to access the GPUs directly for parallel rendering
purposes but use VirtualGL to display the final result.

7. The `vglconnect` script now accepts an argument of `-e {command}`, which can
be used to specify a remote command to run.  This is useful for situations in
which it is necessary to start a remote process non-interactively with
`vglconnect` when using the `-s` or `-x` options.

8. The `glXCreateWindow()` function no longer fails when passed the handle of
an X window that was created in a different process or using XCB.  This
specifically fixes issues encountered when attempting to run VLC in VirtualGL,
but other applications may have been affected as well.

9. Fixed a typo in **/etc/udev/rules.d/99-virtualgl-dri.rules**, which is
created by `vglserver_config`.  More specifically, the typo caused incorrect
group permissions to be assigned to the framebuffer device when not using the
vglusers group.

10. Certain applications (known to be the case with recent versions of Firefox
with off-main-thread compositing enabled) would crash with the following error
when running in VirtualGL:

        [VGL] ERROR: in readPixels--
        [VGL]    XXX: VirtualDrawable instance has not been fully initialized

    This was due to the application creating a GLX Pixmap and then calling
`XCopyArea()` with the Pixmap prior to performing any OpenGL rendering with it.
In these cases, VirtualGL now treats the Pixmap as a 2D Pixmap until the
application has performed OpenGL rendering with it.

11. `glxinfo` has been extended to report whether a particular GLX FB config
supports `GLX_BIND_TO_TEXTURE_RGB_EXT` and `GLX_BIND_TO_TEXTURE_RGBA_EXT`, i.e.
whether the FB config can be used with `GLX_EXT_texture_from_pixmap`.

12. `vglserver_config` should now work properly with SDDM, if its scripts are
installed in the standard location (**/usr/share/sddm/scripts**).

13. Fixed a deadlock that occurred with applications that use MainWin, or any
other OpenGL applications that load a shared library whose global constructor
calls one of the functions VirtualGL interposes.  This was known to affect
MATLAB and ANSYS DesignModeler v16.1 and later, but it may have affected other
applications as well.

14. Fixed another deadlock that occurred when running ANSYS DesignModeler v16.1
and later, or when running VirtualGL in a Parallels Desktop guest, when
VirtualGL was built with `VGL_FAKEXCB=1` (which is the default.)

15. Fixed a thread safety issue with PBO readback.  The PBO handle and other
variables related to PBO readback were being stored in static variables that
were shared among all OpenGL windows, contexts, and threads, and this was
suspected-- but not yet confirmed-- to have caused a "Could not set PBO size"
error with MetaPost.  This issue may have affected other multithreaded
applications as well.


2.4.1
=====

### Significant changes relative to 2.4:

1. When an application doesn't explicitly specify its visual requirements by
calling `glXChooseVisual()`/`glXChooseFBConfig()`, the default GLX framebuffer
config that VirtualGL assigns to it now contains a stencil buffer.  This
eliminates the need to specify `VGL_DEFAULTFBCONFIG=GLX_STENCIL_SIZE,8` with
certain applications (previously necessary when running Abaqus v6 and MAGMA5.)

2. VirtualGL will no longer advertise that it supports the
`GLX_ARB_create_context` and `GLX_ARB_create_context_profile` extensions unless
the underlying OpenGL library exports the `glXCreateContextAttribsARB()`
function.

3. Fixed "Invalid MIT-MAGIC-COOKIE-1" errors that would prevent VirtualGL from
working when `vglconnect` was used to connect to a VirtualGL server from a
client running Cygwin/X.

4. If a 3D application is rendering to the front buffer and one of the
frame trigger functions (`glFlush()`/`glFinish()`/`glXWaitGL()`) is called,
VirtualGL will no longer read back the framebuffer unless the render mode is
`GL_RENDER`.  Reading back the front buffer when the render mode is `GL_SELECT`
or `GL_FEEDBACK` is not only unnecessary, but it was known to cause a
GLXBadContextState error with newer nVidia drivers (340.xx and later) in
certain cases.

5. Fixed a deadlock that occurred in the multithreaded rendering test of
`fakerut` when it was run with the XCB interposer enabled.  This was due to
VirtualGL attempting to handle XCB events when Xlib owned the event queue.  It
is possible that this issue affected or would have affected real-world
applications as well.

6. Fixed an issue that caused certain 3D applications (observed with
CAESES/FFW, although others were possibly affected as well) to abort with
"ERROR: in TempContext-- Could not bind OpenGL context to window (window may
have disappeared)".  When the 3D application called `glXChooseVisual()`,
VirtualGL was choosing a corresponding FB config with
`GLX_DRAWABLE_TYPE=GLX_PBUFFER_BIT` (assuming that `VGL_DRAWABLE=pbuffer`,
which is the default.)  This is incorrect, however, because regardless of the
value of `VGL_DRAWABLE`, VirtualGL still uses Pixmaps on the 3D X server to
represent GLX Pixmaps (necessary in order to make `GLX_EXT_texture_from_pixmap`
work properly.)  Thus, VGL now chooses an FB config that supports both Pbuffers
and Pixmaps.  This was generally only a problem with nVidia drivers, because
they export different FB configs for `GLX_PBUFFER_BIT` and
`GLX_PBUFFER_BIT|GLX_PIXMAP_BIT`.


2.4
===

### Significant changes relative to 2.4 beta1:

1. Fixed an issue that prevented recent versions of Google Chrome/Chromium from
running properly in VirtualGL.

2. `VGL_SYNC` now affects `glFlush()`.  Although this does not strictly conform
to the OpenGL spec (`glFlush()` is supposed to be an asynchronous command), it
was necessary in order to make certain features of Cadence Allegro work
properly.  Since virtually no applications require `VGL_SYNC`, it is believed
that this change is innocuous.

3. Fixed a regression in `vglconnect` (introduced in VirtualGL 2.3) that
prevented `vglconnect -x` from working properly if the user did not have access
to the current directory (`vglconnect` was erroneously creating a temporary
file in the current directory instead of in **/tmp**.)

4. GLXSpheres now warns if the specified polygon count would exceed the limit
of 57600 polygons per sphere imposed by GLU and prints the actual polygon count
with this limit taken into account.  Also, a new option (`-n`) has been
introduced to increase the sphere count.

5. VirtualGL will now only enable color index rendering emulation if a color
index context is current.  This specifically fixes an interaction issue with
MSC Mentat, which occasionally calls `glIndexi()` when an RGBA context is
current, but the fix may affect other applications as well.

6. VirtualGL can now interpose enough of the XCB API to make Qt 5 work
properly.  Qt 5 does not use XCB to perform 3D rendering (there is no suitable
XCB replacement for GLX yet), but it does use XCB to detect whether the GLX
extension is available and to handle the application's event queue(s).  Thus,
when attempting to run Qt 5 applications in VirtualGL, previously the OpenGL
portion of the window would fail to resize when the window was resized, or the
application would complain that OpenGL was not available and fail to start, or
the application would fall back to non-OpenGL rendering.

    Currently, enabling XCB support in VirtualGL requires building VirtualGL
from source and adding `-DVGL_FAKEXCB=1` to the CMake command line.  The XCB
interposer is also disabled by default at run time.  It must be enabled by
setting the `VGL_FAKEXCB` environment variable to `1` or passing `+xcb` to
`vglrun`.

7. Fixed a deadlock that occurred when running Compiz 0.9.11 (and possibly
other versions as well) with VirtualGL.  The issue occurred when Compiz called
`XGrabServer()`, followed by `glXCreatePixmap()` and `glXDestroyPixmap()`.  In
VirtualGL, a GLX pixmap resides on the 3D X server, but the corresponding X11
pixmap resides on the 2D X server.  Thus, VirtualGL has to synchronize pixels
between the two pixmaps in response to certain operations, such as
`XCopyArea()` and `XGetImage()`, or when the GLX pixmap is destroyed.
VirtualGL was previously opening a new connection to the 2D X server in order
to perform this synchronization, and because the 2D X server was grabbed,
Compiz locked up when VirtualGL called `XCloseDisplay()` on the new display
connection.  In fact, however, the new display connection was unnecessary,
since the GLX/X11 pixmap synchronization occurs within the 3D rendering thread.
Thus, VirtualGL now simply reuses the same display connection that was passed
to `glXCreate[GLX]Pixmap()`.

8. NetTest and TCBench for Windows are now supplied in a package called
**VirtualGL-Utils**, which can be built from the VirtualGL source.  When the
VirtualGL Client for Exceed was discontinued, these utilities ceased to have a
home, but they are still useful tools to have, irrespective of the thin client
solution that is being used.  The Windows build of TCBench was temporarily
moved into the Windows TurboVNC Viewer packages, but it proved to be a pain to
keep the source code synchronized between the two projects.

    The **VirtualGL-Utils** package additionally contains a WGL version of
GLXSpheres, which is a useful tool to have when benchmarking Windows virtual
machines that are running in a VirtualGL environment.

9. Worked around an issue in recent versions of SPECviewperf and FEMFAT
visualizer that caused them to segfault when used with VirtualGL.  Those
applications apparently use a dynamic loading mechanism for OpenGL extension
functions, and this mechanism defines symbols such as `glGenBuffers` at file
scope.  Any symbol exported by an application will override a symbol of the
same name exported by a shared library, so when VirtualGL tried to call
`glGenBuffers()`, `glBindBuffer()`, etc., it was picking up the symbols from
the application, not from libGL (and those symbols from the application were
not necessarily defined.)  VirtualGL now obtains the function pointers it needs
for PBO readback directly from libGL using `glXProcAddress()`, rather than
relying on the dynamic linker to resolve them.  Note that this issue could be
worked around in previous versions of VirtualGL by setting `VGL_READBACK=sync`.


2.3.90 (2.4 beta1)
==================

### Significant changes relative to 2.3.3:

1. The VirtualGL Client for Exceed has been retired.  It will continue to be
maintained in the 2.3.x branch on a break/fix basis only.  Cygwin/X has matured
to the point that it now provides an adequate solution for using the VirtualGL
Client on Windows, with the only major limitation being lack of quad-buffered
stereo support.  That feature alone is insufficient to justify a code base
that is basically twice as complex as it otherwise would be.  Furthermore, we
are now maintaining our own Cygwin repository for the VirtualGL Client, which
makes it relatively easy to install on that platform.

    The VirtualGL Client for Exceed reflects VirtualGL's origins as an add-on
technology for existing remote X environments.  These days, most people use
VirtualGL with some sort of X proxy instead.  There have been no significant
changes to the VirtualGL Client since version 2.2.1, as most of the efforts of
The VirtualGL Project in recent years have focused on the server-side
components and TurboVNC.  In the early days of the project, there were
performance advantages to the VGL Transport, but that is no longer the case.
In fact, TurboVNC will generally do a better and faster job of compressing the
rendered frames from 3D applications, since it uses a hybrid compression scheme
rather than pure JPEG.

    The native Windows version of TCBench, which previously shipped with the
VirtualGL Client for Exceed, has been moved into the Windows TurboVNC Viewer
package.

2. The VirtualGL source code has been extensively refactored to use more modern
variable, class, and method naming conventions, and automated test scripts for
the utility libraries and the VirtualGL Faker have been added.

3. `glXChooseFBConfig()` now properly handles the `GLX_FBCONFIG_ID` attribute.
The improper handling of this attribute was known to cause an error
("Could not find GLX 1.3 config from peer info") when running the LWJGL
(Lightweight Java Game Library) on AMD GPUs, but it may have affected other
apps as well.

4. The performance of PBO readback on ATI FirePro GPUs has been improved
dramatically (close to an order of magnitude.)

5. `vglserver_config` will now set DRI device permissions properly on systems
that lack an **xorg.conf** file but have an **xorg.conf.d** directory.

6. `vglserver_config` should now work with recent Debian releases.

7. Fixed an issue whereby VirtualGL would not always resize the Pbuffer
corresponding to an Xt or Motif OpenGL widget whenever the widget was resized.

8. The Mac packaging system now uses pkgbuild and productbuild rather than
PackageMaker (which is obsolete and no longer supported.)  This means that
OS X 10.6 "Snow Leopard" or later must be used when packaging VirtualGL,
although the packages produced can be installed on OS X 10.5 "Leopard" or
later.  OS X 10.4 "Tiger" is no longer supported.

9. The **Uninstall VirtualGL** app should once again work on OS X 10.5.

10. Fixed an infinite drawing loop that occurred when running Altair HyperBeam
with VirtualGL.  Since 2.1.3, VirtualGL has been setting the `WM_DELETE_WINDOW`
property on any OpenGL window so that it (VGL) can be notified if the window
manager deletes the window (thus preventing VGL from trying to draw to the
window after it disappears.)  This was originally done within the body of
`XCreate[Simple]Window()`, but Java did not like us overriding the property for
2D windows (refer to 2.3.1[9].)  Thus, the setting of `WM_DELETE_WINDOW` was
moved into the body of `glXMake[Context]Current()` so that it would affect only
OpenGL windows.  However, VGL was incorrectly replacing the list of WM
protocols rather than simply adding `WM_DELETE_WINDOW` to the existing list.
VGL was also not checking whether `WM_DELETE_WINDOW` already existed in the
list before adding it.  For reasons that are not well understood, this caused
HyperBeam to get into an infinite loop, because calling `XSetWMProtocols()`
within the body of `glXMakeCurrent()` seemed to cause the application to call
`glXMakeCurrent()` again.  This issue may have affected other applications as
well.

11. Fixed an issue whereby the RPMs generated by VirtualGL's packaging system
(including the official RPMs for VGL 2.3.2 and 2.3.3) could not be installed on
later Fedora releases.

12. Fixed an issue whereby `glXSwapBuffers()` would not work properly unless
the drawable passed to that function was current.  This specifically fixes a
rendering issue with voreen, but it may have affected other apps as well.

13. Fixed an issue that prevented `vglgenkey` from working properly on Red Hat
Enterprise Linux 7.

14. Fixed an issue that prevented `vglserver_config` from working properly on
Ubuntu 14.04.


2.3.3
=====

### Significant changes relative to 2.3.2:

1. VirtualGL will no longer throw an exception if a 3D application calls
certain X11 and GLX functions with a NULL argument.  It will instead allow the
underlying X11 or GLX library to handle the error.  This specifically works
around an issue with Fiji.

2. Worked around an issue whereby, when ANSYS Workbench 14.5 was run with
VirtualGL, subprocesses (such as the geometry editor) launched from within the
Workbench environment would not exit properly (and thus would become zombies.)
This issue also affected ANSYS HFSS, which would either lock up when exiting or
print an error message: "terminate called after throwing an instance of
'rrerror'".

3. Worked around an issue whereby, when using MAGMA5 with VirtualGL, the second
and subsequent perspectives opened within the application would not always
display correctly.

4. Added support for the `GLX_EXT_texture_from_pixmap` extension.

5. Added support for the `GLX_EXT_swap_control` and `GLX_SGI_swap_control`
extensions and a new configuration variable (`VGL_REFRESHRATE`) that can be
used to control them.  See the User's Guide for more information.

6. Added support for depth=32 visuals and FB configs.

7. Added a new "window manager" mode that disables certain features in
VirtualGL that interfere with 3D window managers such as Compiz.  This,
combined with [6] and [4] above, should allow Compiz to run properly with this
version of VirtualGL, provided that the 2D X Server has support for the X
Composite extension.  See the User's Guide for more information.

8. Fixed a BadDrawable X11 error that occurred when running the Steam client in
VirtualGL.

9. Improved the accuracy of TCBench and CPUstat.

10. Streamlined VirtualGL's behavior when it is installed from source:

    - `vglrun` now works regardless of where the faker libraries have been
installed.  The build system hard-codes the value of the `VGL_LIBDIR` CMake
variable into a script that `vglrun` invokes so that it can add this directory
to `LD_LIBRARY_PATH`.  If the faker libraries are installed into a system
library directory, then packagers can choose to omit the new script, and
`vglrun` will continue to work as it always has.
    - Whenever a 64-bit build is installed, glxspheres is now renamed
glxspheres64, per the convention of the official packages.  This makes it
possible to install a 32-bit and a 64-bit version of VirtualGL into the same
directory.
    - If the install prefix is set to the default (**/opt/VirtualGL**), then
the build system defaults to installing faker libraries from a 32-bit build
into **/opt/VirtualGL/lib32** and faker libraries from a 64-bit build into
**/opt/VirtualGL/lib64**.
    - Similarly, if the install prefix is set to the default
(**/opt/VirtualGL**), then the build system defaults to installing the 32-bit
libGL symlink for Chromium into **/opt/VirtualGL/fakelib32** and the 64-bit
libGL symlink for Chromium into **/opt/VirtualGL/fakelib64**.

11. PBO readback mode is now enabled by default.  Further research has shown
that professional-grade GPUs always benefit from PBOs being enabled (quite
dramatically, in the case of AMD FirePro GPUs.)  With consumer-grade AMD GPUs,
PBOs generally do no harm, and with consumer-grade nVidia (GeForce) GPUs, the
results are mixed.  The GeForce drivers will fall back to blocking readbacks if
the pixel format requested in `glReadPixels()` doesn't match the pixel format
of the Pbuffer, so PBOs will generally be slower in those cases.  Thus,
VirtualGL now falls back to synchronous readback mode if it detects that PBOs
are not behaving asynchronously.

    Furthermore, `VGL_FORCEALPHA` is no longer enabled by default when PBOs are
enabled.  This option was introduced because of the GeForce behavior mentioned
above, but the option has no effect whatsoever with the professional-grade GPUs
that are recommended for use with VirtualGL.  Instead, VGL will now detect
situations in which `VGL_FORCEALPHA` might be beneficial and suggest enabling
or disabling it (if `VGL_VERBOSE=1`.)

12. This version of VirtualGL provides a binary package and full support for
Cygwin64.


2.3.2
=====

### Significant changes relative to 2.3.1:

1. Added new stereo options, including green/magenta and blue/yellow anaglyphic
as well as three half-resolution passive stereo options that can be used to
drive 3D TVs.

2. The 32-bit supplementary package for amd64 Debian systems should now work
properly on MultiArch-compatible systems (such as Ubuntu 11 and later.)

3. `vglserver_config` should now work properly with LightDM.

4. VirtualGL was not advertising that it supported the
`GLX_ARB_create_context_profile` extension, even though it does.  This has been
fixed.

5. VirtualGL now uses a separate OpenGL context to perform pixel readback.
This fixes several issues, including an error
(`GL_ARB_pixel_buffer_object extension not available`) when trying to enable
PBO readback with applications that use a 3.x or later OpenGL core profile, and
incorrect rendering in JPCSP and other applications that modify certain pixel
store parameters (such as `GL_PACK_SWAP_BYTES` or `GL_PACK_ROW_LENGTH`) that
VirtualGL wasn't properly handling.

6. `VGL_FORCEALPHA=1` now works properly if the 3D application specifies visual
attributes of `GLX_RED_SIZE`=`GLX_GREEN_SIZE`=`GLX_BLUE_SIZE`=`1`.

7. `glXUseXFont()` has been extended to work with Pbuffers.  Due to an
oversight, VirtualGL would previously abort with an error message if the 3D
application attempted to render text to a Pbuffer that it created.

8. Fixed an issue whereby, when displaying to a 2D X server that lacked the
`MIT-SHM` extension, the X11 Transport would sometimes fail to resize its
internal Pixmap (used for double buffering) whenever the X window was resized.
This specifically caused OpendTect to display only a portion of its 3D view
whenever it resized its 3D window after a "Restore" operation, but the issue
may have affected other applications as well.

9. Previously, 3D applications running in VirtualGL could not successfully use
`XGetImage()` to obtain the rendered frame from a GLX pixmap.  This has been
fixed.

10. `vglrun` now automatically sets an environment variable that disables the
execution of the `VBoxTestOGL` program in VirtualBox 4.2 and later.  Since
`LD_PRELOAD` is not propagated down to VBoxTestOGL whenever VirtualBox launches
it (because VirtualBox is a setuid-root executable), VBoxTestOGL always fails
in a VirtualGL environment, which makes VirtualBox believe that the system has
no 3D support.  With version 4.1.10, VirtualBox began running VBoxTestOGL every
time a VM was launched, which effectively prevented VBox from being used with
VirtualGL unless the user hacked their system by symlinking `/bin/true` to
`/usr/lib/virtualbox/VBoxTestOGL`.


2.3.1
=====

### Significant changes relative to 2.3:

1. Worked around a segfault that occurred when running CoreBreach.

2. VirtualGL now properly handles implicit deletion of windows/subwindows via
`XCloseDisplay()`, implicit deletion of subwindows via `XDestroyWindow()`, and
explicit deletion of subwindows via `XDestroySubwindows()`.  This specifically
addresses BadDrawable errors that occurred when running certain applications
in WINE 1.3.34 and later.

3. Fixed a crash in `glXCreateGLXPbufferSGIX()` that occurred when a NULL
attribute list pointer was passed to it.

4. VirtualGL should now build and run properly on FreeBSD.

5. VirtualGL now works properly with applications that dynamically load libX11.
This specifically fixes several issues that occurred when running SDL-based
applications against a version of libSDL that was configured with
`--enable-x11-shared`.

6. Changed the Debian package names to lowercase (**virtualgl** and
**virtualgl32**) to avoid an issue whereby the package was always being
installed, even if the installed version was up to date.

7. `vglserver_config` now works properly with KDM on RHEL/CentOS 5 systems.

8. Added a new option (`VGL_GLFLUSHTRIGGER`) that, when set to `0`, will cause
VirtualGL to ignore `glFlush()` commands from the 3D application.  This is
intended for rare applications that do front buffer rendering and use
`glFlush()` as an "intermediate" synchronization command but then subsequently
call `glFinish()` to indicate the end of the frame.

9. Fixed an issue whereby drag & drop operations in certain Java applications
would cause VNC servers (any VNC server, not just TurboVNC) to hang whenever
the Java application was run using VirtualGL.


2.3
===

### Significant changes relative to 2.3 beta1:

1. Fixed a regression whereby GLXSpheres would ignore the first argument after
`-fs`.

2. `glXChooseFBConfig()` and `glXChooseFBConfigSGIX()` were erroneously
returning an error when a NULL attribute list pointer was passed to them.  They
now behave correctly.

3. Fixed a regression whereby VirtualGL would deadlock when using the X11
Transport with a remote X connection.

4. Fixed a `GL_INVALID_OPERATION` error that would occur after a call to
`glXSwapBuffers()`, when a context with the OpenGL Core Profile was being used.

5. Fixed an issue whereby VirtualGL, when compiled with GCC 4.6, would abort
with "terminate called after throwing an instance of 'rrerror'" whenever a 3D
application running in VirtualGL exited.

6. Added a new configuration option (`VGL_DEFAULTFBCONFIG`) that can be used
to manually specify the rendering attributes of the default FB config that
VirtualGL uses whenever a 3D application does not specify a desired set of
visual attributes (which it would normally do by calling `glXChooseVisual()`.)
Refer to the User's Guide for more information.

7. Worked around an issue whereby using very large fonts with `glXUseXFont()`
would cause Pixmap allocation failures with certain X servers.


2.2.90 (2.3 beta1)
==================

### Significant changes relative to 2.2.1:

1. Re-fixed issue that caused MainWin-based applications to hang.  This was
initially fixed in VGL 2.1 final, but it was re-broken by the rewrite of the
global VirtualGL Faker configuration routines in VGL 2.2.

2. Overhauled the way in which VirtualGL handles Pixmap rendering, mainly to
fix interaction issues with Mathematica.

3. Added an option (`VGL_ALLOWINDIRECT`) that, when enabled, will cause
VirtualGL to honor an application's request to create an indirect OpenGL
context.  Normally VirtualGL forces all contexts to be direct for performance
reasons, but this causes problems with certain applications (notably
Mathematica 7.)

4. Added two new command-line options to GLXSpheres that allow the window size
to be changed and the total number of frames to be specified (the application
will abort after the total number of frames has been rendered.)

5. VirtualGL will no longer die if `glXGetConfig()` or `glXGetFBConfigAttrib()`
is passed a NULL argument.

6. Fixed a BadMatch X11 error that occurred when an application attempted to
apply a new OpenGL rendering context to a drawable and the FB config of the new
OpenGL context differed from that of a context that was previously applied to
the same drawable.  This specifically was known to affect D3D applications
running in WINE.

7. CMake-based build and packaging system

8. TCBench now takes 100 samples/second by default instead of 50.

9. Added support for the `GLX_ARB_create_context` extension.


2.2.1
=====

### Significant changes relative to 2.2:

1. A 64-bit version of the VirtualGL Client for Exceed is now fully supported.

2. Fixed a severe readback performance problem that occurred whenever an
application set the render mode to `GL_SELECT` and called `glFlush()` while
doing front buffer rendering.

3. `vglserver_config` will now work properly whenever `vglgenkey` is installed
in a directory other than **/usr/bin** or **/opt/VirtualGL/bin**, as long as
`vglgenkey` is installed in the same directory as `vglserver_config`.

4. `vglconnect` now allows the user to specify the directory in which VirtualGL
binaries are installed on the server, rather than always assuming that they are
installed in **/opt/VirtualGL/bin**.

5. Fixed issues with `vglconnect.bat` that occurred when it was installed under
**c:\Program Files (x86)** on 64-bit Windows systems and invoked with the `-x`
or `-s` options.

6. Clarified the documentation of the `VGL_DISPLAY` option and documented how
to use VirtualGL with multiple graphics cards.

7. VirtualGL will no longer die if `glXDestroyContext()` is passed a NULL
argument.

8. Fixed a BadWindow error that would occur whenever a 3D application attempted
to call `glXSwapIntervalSGI()` (specifically, this was observed when running
Direct3D applications using WINE 1.3.11 or later.)  The nVidia GLX
implementation requires that a window be current (not a Pbuffer) when calling
`glXSwapIntervalSGI()`, so VirtualGL now interposes that function and makes it
a no-op.

9. Fixed an issue whereby, if a 3D application set
`GL_(RED|GREEN|BLUE|ALPHA)_SCALE` or `GL_(RED|GREEN|BLUE|ALPHA)_BIAS` to
non-default values, the colors would appear wrong when running the application
in VirtualGL, and the readback performance would be very slow.

10. Fixed an issue whereby 3D applications that requested an overlay visual
would fail with BadRequest or other X11 errors if the 2D X server lacked GLX
support (as is the case with TurboVNC.)  This was caused by the fact that
VirtualGL passes through `glXChooseVisual()` and related calls to the 2D X
server if it detects that an overlay visual is being requested.  Now, VirtualGL
will first check that the 2D X server supports GLX before passing through those
calls.

11. `vglclient -kill` and `vglclient -killall` now work (again) with the
VirtualGL Client for Exceed.


2.2
===

### Significant changes relative to 2.2 beta1:

1. Added an environment variable (`VGL_SPOILLAST`) which, when set to `0`, will
change the frame spoiling algorithm used for frames triggered by `glFlush()`
calls.  This is necessary to make Ansoft HFSS render properly.

2. Added a compatibility mode to allow NetTest to communicate with older
versions of itself (from VGL 2.1.x and prior.)

3. Fixed a race condition in the VirtualGL Client that would frequently cause
an "incorrect checksum for freed object" error when `vglclient` was shut down
using CTRL-C.  This problem was reported only on OS X but could have existed on
other platforms as well.


2.1.90 (2.2 beta1)
==================

### Significant changes relative to 2.1.4:

1. Added an option (`VGL_LOGO`) that, when enabled, will cause VirtualGL to
display a logo in the bottom right-hand corner of the 3D window.  This is meant
as a debugging tool to allow users to determine whether or not VirtualGL is
active.

2. Support for encrypting the VGL Transport with OpenSSL has been removed from
the official VirtualGL packages.  It was only a marginally useful feature,
because VirtualGL also has the ability to tunnel the VGL Transport through SSH.
It was necessary to maintain our own static OpenSSL libraries on Linux in
order to provide a version of VirtualGL that was compatible across all Linux
platforms, and this required us to keep abreast of the latest OpenSSL security
fixes, etc.  OpenSSL support can easily be re-added by building VirtualGL from
source (see **[BUILDING.md](BUILDING.md)**.)

3. Added a framework for creating generic image transport plugins.  See
**[server/rrtransport.h](server/rrtransport.h)** for a description of the API.

4. Removed support for the proprietary Sun Ray plugin, since that plugin is no
longer available from Sun.

5. For Linux, Mac/Intel, Solaris/x86, and Windows systems, the default build of
VirtualGL no longer uses TurboJPEG/IPP (which was based on the proprietary
Intel Performance Primitives) or Sun mediaLib.  Instead, VirtualGL now uses
libjpeg-turbo, a fully open source SIMD-accelerated JPEG codec developed in
conjunction with the TigerVNC Project (and based on libjpeg/SIMD.)

    As a result of this, it is no longer necessary to install a separate
TurboJPEG package on Linux systems.

6. Added a universal 32/64-bit VirtualGL Client binary for OS X.  The 32-bit
fork works on 10.4 "Tiger" or later, and the 64-bit fork works on 10.5
"Leopard" or later.

7. Added support for encoding rendered frames as I420 YUV and displaying them
through X Video.  The frames can either be displayed directly to the 2D X
server or sent through the VGL Transport for display using the VirtualGL
Client.  See the User's Guide for more details.

8. Renamed **/etc/modprobe.d/virtualgl** to **/etc/modprobe.d/virtualgl.conf**
to comply with the Ubuntu standard.

9. Added an environment variable (`VGL_SAMPLES`) and command-line switch
(`vglrun -ms`) to force VirtualGL to select a multisampled visual or override
the level of multisampling selected by the 3D application.

10. The `uninstall` script in the Mac binary package should now work on OS X
10.6.

11. VirtualGL can now use pixel buffer objects (PBOs) to accelerate the
readback of rendered frames.  This particularly helps when multiple users are
sharing the GPU.  See the "Advanced Configuration" section of the User's Guide
for more information.

12. On Linux systems, this version of VirtualGL works around the interaction
issues between **libdlfaker.so** and VirtualBox, thus eliminating the need to
specify `CR_SYSTEM_GL_PATH` or to run the VirtualBox application using
`vglrun -nodl`.

13. This version of VirtualGL provides a binary package, documentation, and
full support for Cygwin/X.

14. Fixed an error ("free(): invalid pointer") that occurred whenever an
application called `XCloseDisplay()` and the `VGL_XVENDOR` environment variable
was set.

15. VirtualGL should now work properly when used with applications that render
to framebuffer objects (FBOs.)

16. `vglconnect.bat` should now work properly on 64-bit Windows systems.
Previously, it would fail if it was installed under the **Program Files (x86)**
directory.

17. Added a `-gid` option to `vglserver_config` to allow the group ID of the
vglusers group to be specified, if that group must be created.

18. TCBench should now work properly on OS X.


2.1.4
=====

### Significant changes relative to 2.1.3:

1. Fixed a regression in `vglserver_config` that caused a "binary operator
expected" error when restricting framebuffer device access to the vglusers
group.

2. Fixed an issue in `vglserver_config` whereby the device permissions were not
being set correctly on SuSE Linux Enterprise Desktop 11.

3. VGL should now properly ignore `GLX_BUFFER_SIZE` if an application attempts
to specify it when requesting a true color visual.  Specifically, this allows
the Second Life SnowGlobe client to run properly in VGL and WINE.

4. `vglserver_config` should now work even if **/sbin** and **/usr/sbin** are
not in the `PATH`.

5. The Solaris 10/x86 version of VirtualGL should now work properly with the
nVidia 18x.xx series drivers.

6. Fixed a memory leak that occurred when running VirtualGL in quad-buffered
stereo mode.

7. The DRI device permissions in RHEL 5 were being overridden whenever a user
logged in, because RHEL 5 uses a file in **/etc/security/console.perms.d** to
specify the default DRI permissions rather than using
**/etc/security/console.perms**.  `vglserver_config` has been modified to
handle this.

8. Added an option to `vglconnect` on Linux/Unix to allow it to use `gsissh`
from the Globus Toolkit instead of the regular `ssh` program.


2.1.3
=====

### Significant changes relative to 2.1.2:

1. VirtualGL 2.1.2 printed numerous "Cannot obtain a Pbuffer-enabled 24-bit FB
config ..." error messages when starting Google Earth.  This has been fixed,
and the message has been changed to a warning and clarified.  These error
messages were printed whenever Google Earth called VirtualGL's interposed
version of `glXChooseVisual()` and that function subsequently failed to obtain
an appropriate visual for performing 3D rendering.  However, this is not
necessarily an error, because applications will sometimes call
`glXChooseVisual()` multiple times until they find a visual with desired
attributes.

2. Changed the matching criteria in VirtualGL's interposed version of
`dlopen()`.  In previous versions of VirtualGL, any calls to
`dlopen("*libGL*", ...)` would be replaced with a call to
`dlopen("librrfaker.so", ...)`.  This caused problems with VisIt, which has a
library named **libGLabelPlot.so** that was being interposed by mistake.  The
matching criteria has been changed such that `dlopen()` only overrides calls to
`dlopen("libGL\.*", ...)` or `dlopen("*/libGL\.*", ...)`.

3. `vglserver_config` should now work properly with DRI-compatible graphics
drivers (including ATI.)

4. VirtualGL's interposed version of `dlopen()` will now modify calls to
`dlopen("libdl*", ...)` as well as `dlopen("libGL*", ...)`.  This is to work
around an interaction issue with v180.xx of the nVidia accelerated 3D drivers
and WINE.

5. Fixed an interaction issue with QT4 in which VirtualGL would not properly
handle window resize events under certain circumstances.

6. Moved `dlopen()` back into a separate faker library (**libdlfaker.so**.)
**libdlfaker.so** is loaded by default, which should maintain the behavior of
VGL 2.1.2.  However, it can be disabled by passing an argument of `-nodl` to
`vglrun`.  The latter is necessary to make VirtualBox 2.2.x work with
VirtualGL.

7. `vglserver_config` should now work properly on Ubuntu 9.04 when using gdm or
kdm.  It should also (mostly) work on Fedora 11 (disabling XTEST does not work
on Fedora 11 when using gdm.)

8. Added fallback logic to VirtualGL's symbol loader, which will now try to
directly load the GLX/OpenGL symbols from **libGL.so.1** and the X11 symbols
from **libX11.so.6** if loading those symbols using `dlsym(RTLD_NEXT, ...)`
fails.  This is to work around an issue with version 18x.xx of the nVidia Linux
Display Driver.

9. If an application window was destroyed by the window manager, and the
application did not explicitly monitor and handle the `WM_DELETE` protocol
message, then previous versions of VirtualGL would, when using the X11 Image
Transport, generally abort with an X11 BadDrawable error.  This occurred
because the window was basically being ripped out from underneath VirtualGL's
blitter thread without warning.  This version of VirtualGL has been modified
such that it monitors `WM_DELETE` messages, so VirtualGL can now bow out
gracefully if the 3D application window is closed by the window manager but the
application does not handle `WM_DELETE`.

10. Worked around an interaction issue with IDL whereby the application was
calling `XGetGeometry()` with the same pointer for every argument, and this was
causing VirtualGL to lose the width and height data returned from the "real"
`XGetGeometry()` function.  Subsequently, the Pbuffer corresponding to the main
IDL window would become improperly sized, and the rendering area would not
appear to update.

11. Added an option (`VGL_TRAPX11`) that will cause VirtualGL to gracefully
trap X11 errors, print a warning message when these occur, and allow the
3D application to continue running.


2.1.2
=====

### Significant changes relative to 2.1.1:

1. Fixed a buffer overrun in TurboJPEG/mediaLib that may have caused problems
on Solaris/x86 VirtualGL servers.

2. Integrated **libdlfaker.so** into **librrfaker.so** to eliminate the need
for invoking `vglrun -dl`.

3. Developed a proper uninstaller app for the Mac OS X VirtualGL package.

4. Modified `MAXINST` variable in the **SUNWvgl** Solaris package, to prevent
multiple instances of this package from being installed simultaneously.


2.1.1
=====

This release was historically part of the Sun Shared Visualization v1.1.1
product.

### Significant changes relative to 2.1:

1. Fixed issues that occurred when displaying to the second or subsequent
screens on a multi-screen X server.

2. Updated to the wxWindows Library License v3.1.

3. Added an uncompressed YUV encoding option to the Sun Ray plugin.  This
provides significantly better performance than DPCM on Sun Ray 1 clients, and
it provides significantly better image quality in all cases.  YUV encoding will
generally use about 50% more network bandwidth than DPCM, all else being equal.

4. Further optimized the Huffman encoder in the mediaLib implementation of
TurboJPEG.  This should decrease the CPU usage when running VirtualGL on
Solaris VirtualGL servers, particularly Solaris/x86 servers running 32-bit
applications.

5. `vglconnect` now works properly with Cygwin.

6. Fixed a regression that caused VirtualGL to remove any part of the
`VGL_XVENDOR` string following the first whitespace.

7. `vglserver_config` now works properly with OpenSolaris systems.

8. `glXUseXFont()` now works if a Pixmap is the current drawable.

9. `vglserver_config` now works properly with Debian Linux systems.

10. Fixed a typo in `vglconnect` that caused it to leave temporary files lying
about.

11. Removed the libXm (Motif) dependency in the Solaris/x86 version of
VirtualGL.  This mainly affected OpenSolaris systems, on which Motif is not
available.  The libXm dependency was introduced in VirtualGL 2.0.x, because
that version of VirtualGL used libXt (X Intrinsics) to generate its popup
configuration dialog.  Java requires that libXm be loaded ahead of libXt, so it
was necessary to explicitly link the VirtualGL Faker with libXm to guarantee
that the libraries were loaded in the correct order.  Since VirtualGL 2.1.x and
later no longer use libXt, the binding to libXm could be safely removed on x86
systems.  Note, however, that the libXm binding still has to be included on
SPARC systems, because libGL on SPARC depends on libXmu, which depends on
libXt.

12. Fixed an issue with GeomView whereby attempting to resize the oldest window
in a multi-window view would cause the VirtualGL Client for Exceed to crash.

13. When used to configure a Solaris server for GLX mode with open access (all
users being able to access the 3D X server, not just members of the vglusers
group), `vglserver_config` was incorrectly placing `xhost` commands at the top
of `/etc/dt/config/Xsetup` instead of at the bottom.  This could have led to
problems, since `xhost` is not guaranteed to be in the `PATH` until the bottom
of that script.

14. VirtualGL should now build and run cleanly on Ubuntu systems (and possibly
other Debian derivatives, although only Ubuntu has been tested.)

15. Certain applications call `glFlush()` thousands of times in rapid
succession when rendering to the front buffer, even if no part of the 3D scene
has changed.  Without going into the gorey details, this caused the VirtualGL
pipeline to become overloaded in certain cases, particularly on systems with
fast pixel readback.  On such systems, every one of the `glFlush()` commands
resulted in VirtualGL transporting a rendered frame, even if the frame was
identical to the previous frame.  This resulted in application delays of up to
several minutes.  This version of VGL includes a mechanism that ensures that no
more than 100 `glFlush()` commands per second will actually result in a
rendered frame being transported, and thus strings of rapid-fire `glFlush()`
commands can no longer overload the pipeline.

16. `vglserver_config` should now work with openSUSE systems.

17. It was discovered that `xhost +LOCAL:` is a better method of enabling 3D X
server access to all users of the VirtualGL server.  This method works even if
TCP connections are disabled in the X server (which is the case on recent
Solaris and Linux distributions.)  `vglserver_config` has thus been modified to
use this method rather than `xhost +localhost`.  Also, since it is no longer
necessary to set `DisallowTCP=false` in the GDM configuration file,
`vglserver_config` now comments out this line if it exists.

18. Fixed a deadlock that occurred when using the VirtualGL Sun Ray plugin and
ParaView to render multi-context datasets.

19. Fixed an issue with Mac X11 2.1.x on OS X 10.5 "Leopard" whereby
`vglconnect` would abort with `Could not open log file.`  X11 2.1.x uses a
`DISPLAY` environment of the form `/tmp/launch-*/:0`, so it was necessary to
remove everything up to the last slash before using this variable to build a
unique log file path for the VirtualGL Client.

20. Included **libgefaker.so** in the Solaris VirtualGL packages (oops.)

21. Added an interposed version of `XSolarisGetVisualGamma()` on SPARC servers,
so applications that require gamma-corrected visuals can be fooled into
thinking that a gamma-corrected visual is available, when in fact VirtualGL's
software gamma correction mechanism is being used instead.

22. Fixed a bug in the color conversion routines of TurboJPEG/mediaLib that
caused the Solaris VirtualGL Client to display incorrect pixels along the
right-most edge of the window when 2X or 4X subsampling was used.


2.1
===

This release was historically part of the Sun Shared Visualization v1.1
product.

### Significant changes relative to 2.1 beta1:

1. Windows applications now link statically with OpenSSL, to avoid a dependency
on **msvcr71.dll** that was introduced in the Win32 OpenSSL 0.9.8e DLLs.
**libeay32.dll** and **ssleay32.dll** have been removed from the Windows
package, since they are no longer needed.

2. Implemented a new interposer library (**libgefaker.so**) which, when active,
will interpose on `getenv()` and return NULL whenever an application queries
the value of the `LD_PRELOAD` environment variable (and, on Solaris, the
`LD_PRELOAD_32` and `LD_PRELOAD_64` environment variables.)  This fools an
application into thinking that no preloading is occurring.  This feature is
currently undocumented and is subject to change, but it can be activated in
this release by passing an argument of `-ge` to `vglrun`.

3. Extended `VGL_SYNC` functionality to include `glXSwapBuffers()` as well.
This works around a couple of interaction issues between VirtualGL and
AutoForm.  The User's Guide has been updated to explain this new functionality,
to include an application recipe for AutoForm, and to include a warning about
the performance consequences of using `VGL_SYNC` on a remote X connection.

4. Included a short paragraph in the Chromium section of the User's Guide
that explains how to use VirtualGL on the render nodes to redirect 3D rendering
from a window to a Pbuffer.

5. Documented the patch revision necessary to make Exceed 2008 work properly
with VirtualGL.  Removed dire warnings.

6. Added full-screen rendering mode to GLXSpheres.

7. Fixed a regression (uninitialized variable) introduced by 2.1 beta1[7].

8. Fixed a logic error that caused the VirtualGL Client to fail when using
OpenGL drawing and talking to a legacy (VGL 2.0 or earlier) VirtualGL faker.

9. Fixed a regression that would cause VGL to deadlock when the user closed the
3D application window while using the X11 Image Transport.

10. Fixed an issue whereby `vglserver_config` would fail to detect the presence
of GLP because nm was not in the default `PATH`.

11. When running VirtualGL on Enterprise Linux 5 using the 100 series nVidia
drivers, a normal exit from a 3D application will result in
`glXChannelRectSyncSGIX()` being called after the VirtualGL hashes have already
been destructed.  `glXChannelRectSyncSGIX()` calls `XFree()` (an interposed
function), and under certain circumstances, the interposed version of `XFree()`
was trying (and failing) to access one of the previously destructed VirtualGL
hashes.  This has been fixed.

12. `vglclient -kill` now works properly on Solaris systems.

13. Fixed a timing snafu in-- and raised the run priority of-- the Windows
version of TCBench.  This improves the accuracy of TCBench on Windows in
high-client-CPU-usage scenarios.

14. Fixed an issue in the interactive mode of GLXSpheres whereby it would use
100% of the CPU when sitting idle.  Added an option to GLXSpheres to adjust the
polygon count of the scene.

15. Added application recipes for ANSYS and Cedega to the User's Guide.  Added
notes describing the ANSYS and Pro/E duplicate `glFlush()` issue and
workarounds.

16. Changed the default value for `VGL_NPROCS` to `1` (the performance study
indicates that there is no longer any measurable advantage to multithreaded
compression on modern hardware with VGL 2.1.)

17. Added a `-bench` option to `nettest` to allow it to measure actual usage on
a given Solaris or Linux network device.  This was necessitated by accuracy
issues with other open source network monitoring solutions.

18. Changed the way global hash tables are allocated in VGL, in order to fix an
interaction issue with applications that are built with MainWin.

19. `vglclient -kill` now works properly on Mac systems.

20. The VirtualGL Configuration dialog now pops up properly when Caps Lock is
active.

21. If a previously destroyed GLX context is passed as an argument to
`glXMake[Context]Current()`, the function now returns False instead of throwing
a fatal exception.  This was necessary to make a couple of different commercial
applications work properly.

22. There was some inconsistency regarding the interface for enabling lossless
compression in the Sun Ray Image Transport.  The User's Guide listed the
interface as `VGL_COMPRESS=srrgb`, whereas the output of `vglrun` listed the
interface as `-c srlossless`.  VirtualGL 2.1rc only responded to the latter.
This version of VirtualGL now responds to both interfaces, but
`VGL_COMPRESS=srrgb` is the correct and documented interface.  Any errant
references to "srlossless" have been changed.


2.1 beta1
=========

### Significant changes relative to 2.0.1:

1. The VirtualGL Configuration dialog is now implemented using FLTK instead of
X Intrinsics.  The dialog is also now handled by a separate process
(`vglconfig`), to avoid application interaction issues.  VirtualGL and
`vglconfig` communicate changes to the configuration through shared memory.

    Note that this renders both the `VGL_GUI_XTTHREADINIT` configuration option
and the corresponding application recipe for VisConcept unnecessary, and thus
both have been removed.

2. Added two new scripts (`vglconnect` and `vgllogin`) to automate the process
of connecting to a VirtualGL server and using the VGL Image Transport (formerly
"Direct Mode.")  `vglconnect` wraps both `vglclient` and SSH.  Through the use
of command-line arguments, `vglconnect` can be configured to forward either the
X11 traffic or the VGL Image Transport (or none or both) over the SSH
connection.  `vglconnect` invokes `vgllogin` on the server, which configures
the server environment with the proper `VGL_CLIENT` and `VGL_PORT` values so
that, once connected, no further action is required other than to launch a 3D
application with `vglrun`.  See the User's Guide for more information.

3. The VirtualGL Client includes many changes to support `vglconnect`.  Rather
than listen just on ports 4242 and 4243, the default behavior of `vglclient` is
now to find a free listening port in the range of 4200-4299 (it tries 4242 and
4243 first, to maintain backward compatibility.)  The VirtualGL Client records
its listening port in an X property that is later read by the VirtualGL Faker.
This feature allows more than one instance of `vglclient` to run on the same
machine.  The VirtualGL Client can also detach completely from the console and
run as a background daemon, exiting only when the X server resets or when
`vglclient` is explicitly killed.

    Previous versions of VirtualGL required one instance of the VirtualGL
Client to talk to all client-side X displays, but this created problems in
multi-user environments.  Thus, VirtualGL 2.1 runs a separate instance of
`vglclient` for each unique X display.  This eliminates the need (as well as
the ability) to run `vglclient` as a root daemon or as a Windows service, and
thus those features have been removed.

    Since `vglclient` is intended to be launched from `vglconnect`, Start Menu
links to the VirtualGL Client are no longer included in the Windows package.

    See the User's Guide for more information about the changes to the
VirtualGL Client.

4. The Windows VirtualGL package now includes an optimized version of PuTTY
0.60, which is used by the Windows version of `vglconnect`.  When tunneling the
VGL Image Transport, this version of PuTTY provides significantly better
performance than the stock version of PuTTY 0.60.

5. Added a new script (`vglserver_config`) to automate the process of
configuring the 3D X server to allow connections from VirtualGL.  This script
can also be used to configure GLP, for machines that support it.  See the
User's Guide for more information.

6. The VirtualGL Unix packages now include a benchmark called "GLXSpheres",
which is an open source look-alike of the old nVidia SphereMark demo.  This
program is meant to provide an alternative to GLXGears, since the images
generated by the latter program contain too much solid color to be a good test
of VirtualGL's image pipeline.  GLXSpheres also includes modes that can be used
to test VirtualGL's support of advanced OpenGL features, such as stereo,
overlays, and color index rendering.

7. VirtualGL now works properly with multi-process OpenGL applications that use
one process to handle X events and another process to handle 3D rendering.  In
particular, this eliminates window resize issues with Abaqus/CAE and with the
Chromium readback SPU.

8. Added an additional subsampling option to enable grayscale JPEG encoding.
This provides additional bandwidth savings over and above chrominance
subsampling, since grayscale throws away all chrominance components.  It is
potentially useful when working with applications that already render grayscale
images (medical imaging, etc.)

9. VirtualGL can now encode rendered frames as uncompressed RGB and send those
uncompressed frames through the VGL Image Transport.  This has two benefits:

     - It eliminates the need to use the X11 Image Transport (formerly "Raw
Mode") over a network, and
     - It provides the ability to send lossless stereo image pairs to a
stereo-enabled client.

    A gigabit or faster network is recommended when using RGB encoding.

10. Anaglyphic stereo support.  When VirtualGL detects that an application has
rendered something in stereo, its default behavior is to try using quad-
buffered stereo.  However, if the client or the image transport do not support
quad-buffered stereo, then VirtualGL will fall back to using anaglyphic
(red/cyan) stereo.  This provides a quick & dirty way to visualize stereo data
on clients that do not support "real" stereo rendering.

    VirtualGL 2.1 can also be configured to transport only the left eye or
right eye images from a stereo application.

11. Changed the way VirtualGL spoils frames.  Previous versions would throw out
any new frames if the image transport was already busy transporting a previous
frame.  In this release, VirtualGL instead throws out any untransported frames
in the queue and promotes every new frame to the head of the queue.  This
ensures that the last frame in a rendering sequence will always be transported.

12. Better integration with the Sun Ray plugin.  In particular, many of the Sun
Ray plugin's configuration options can now be configured through the VirtualGL
Configuration dialog.

13. The Mac VirtualGL Client is now fully documented.

14. Included mediaLib Huffman encoding optimizations contributed by Sun.  This
boosts the performance of VirtualGL on Solaris systems by as much as 30%.
This, in combination with mediaLib 2.5, should allow the Solaris/x86 version of
VirtualGL to perform as well as the Linux version, all else being equal.

15. Lighting did not work properly for color index applications in prior
versions of VGL.  This has been fixed.

16. Fixed an interaction issue with the 100 series nVidia drivers, whereby
applications that requested a single-buffered RGB visual would sometimes fail
to obtain it through VirtualGL.  The newer nVidia drivers don't always return
an RGB 8/8/8 framebuffer config as the first in the list, so it was necessary
for VirtualGL to specify `GLX_RED_SIZE`, `GLX_GREEN_SIZE`, and `GLX_BLUE_SIZE`
when obtaining a framebuffer config on the 3D X server.

17. Interframe comparison now works properly with stereo image pairs, and
interframe comparison can now be disabled by using the `VGL_INTERFRAME` option.


2.0.1
=====

This release was historically part of the Sun Shared Visualization v1.0.1
product.

### Significant changes relative to 2.0:

1. The Linux and Solaris versions of VirtualGL are now statically linked with
OpenSSL.  This fixes an issue whereby a 3D application that uses OpenSSL could
override VirtualGL's SSL bindings, thus causing VirtualGL to crash if SSL was
enabled (`VGL_SSL=1` or `vglrun +s`) and if the application's version of
OpenSSL was incompatible with VirtualGL's.

    This, combined with [5] below, has the added benefit of allowing a single
VirtualGL RPM to be used across multiple Linux platforms.  It is no longer
necessary to use a separate RPM for different versions of Enterprise Linux,
SuSE, etc.

2. **librrfaker.so** and **libturbojpeg.so** are now being linked with map
files (AKA "anonymous version scripts" on Linux.)  This is mainly a
preventative measure, because it hides any non-global symbols in the shared
objects, thus preventing those symbols from accidentally interposing on a
symbol in an application or in another shared object.  However, this was also
necessary to prevent [1] from causing the opposite problem from the one it was
intended to fix (without [2], VirtualGL could interpose on an application's SSL
bindings rather than vice versa.)

    Linux users will need to upgrade to TurboJPEG 1.04.2 (or later) to get this
fix.  For other platforms, the fix is included in the VGL 2.0.1 packages.

3. **librr.so**, **rr.h**, and the **rrglxgears.c** sample application have
been removed from the distribution packages.  These demonstrated a strawman API
for creating a VGL movie player.  The API was somewhat ill-conceived and
broken, and it needs to be revisited.

4. VirtualGL's custom version of `glxinfo` is now included in the distribution
packages, under **/opt/SUNWvgl/bin** or **/opt/VirtualGL/bin**.  This version
of `glxinfo` supports GLP on SPARC servers and also has the ability to query
GLX FB Configs as well as X visuals.

5. VGL now uses direct linking to link against libCrun on Solaris and static
linking to link against libstdc++ on Linux.  This is to prevent a problem
similar to [1], in which an application that overrides the default C++ `new`
and `delete` operators could force VGL to use its custom `new` and `delete`
operators rather than the default operators provided in libCrun/libstdc++.
Specifically, this addresses an issue whereby Pro/E would spuriously crash on
multi-processor Solaris/SPARC machines.

    libstdc++ is statically linked on Linux because Linux has no equivalent of
Direct linking, but static linking against libstdc++ has the added benefit of
allowing one VirtualGL RPM to be used across multiple Linux platforms.

6. Eliminated the use of the X `DOUBLE-BUFFER` extension in Raw Mode and
replaced it instead with X Pixmap drawing.  Previously, VGL would try to use
the `MIT-SHM` extension to draw rendered frames in Raw Mode, then it would fall
back to using the X `DOUBLE-BUFFER` extension if `MIT-SHM` was not available or
could not be used (such as on a remote X connection), then it would fall back
to single-buffered drawing if `DOUBLE-BUFFER` could not be used.  However, the
`DOUBLE-BUFFER` extension crashes on some Sun Ray configurations (specifically
Xinerama configurations), so VGL 2.0 disabled the use of `DOUBLE-BUFFER` on all
Sun Ray configurations to work around this issue.  [6] replaces that hack with
a more solid fix that ensures that Raw Mode is always double-buffered, even if
the X `DOUBLE-BUFFER` extension is not available or is not working.  Pixmap
drawing has the same performance as `DOUBLE-BUFFER`.

    This generally only affects cases in which Raw Mode is used to transmit
rendered frames over a network.  When Raw Mode is used to transmit rendered
frames to an X server on the same machine, it is almost always able to use the
`MIT-SHM` X extension.

7. Numerous doc changes, including:

     - Restructuring the User's Guide to create a more clear delineation
between what needs to be done on the server and what needs to be done on the
client.
     - (Re-)added instructions for how to use VGL with a direct X11 connection,
since it has become apparent that that configuration is necessary in some
cases.
     - Changed procedure for doing Direct Mode SSH tunneling.  The previous
procedure would not have worked if multiple users were trying to tunnel Direct
Mode from the same server.  The new procedure requires running a program (see
[8] below) that prints out an available TCP port, and using that port on the
server end of the SSH tunnel.
     - Added application recipe for ANSA 12.1.0.
     - Added procedures for using VGL with TurboVNC and for using TurboVNC in
general.
     - Added information for using VGL with GLP (which, unbelievably, was never
really included in the User's Guide.  Major oversight on our part.)

8. Modified the `nettest` program such that it finds a free TCP port number and
prints out the port number to the console when you pass an argument of
`-findport` to the program.

9. Fixed `glXGetProcAddressARB()` and `glXGetProcAddress()` on SPARC platforms.
Due to an erroneous `#ifdef` statement, these interposed functions were not
getting compiled into VirtualGL when VirtualGL was built with Sun OpenGL on
SPARC platforms.

    This, combined with [16], allows Java 2D apps running in VirtualGL on SPARC
platforms to successfully use the OpenGL pipeline to perform Java 2D rendering.

10. Sun OpenGL 120812-15 (and later) now includes a `SwapBuffers()` command for
GLP, which VirtualGL will now use if available.  Some applications that use
front buffer drawing (Pro/E and UGS/NX v4, specifically) did not work properly
with VirtualGL 2.0 in GLP mode, because the back buffer in the double-buffered
Pbuffer was not being swapped.

11. Some applications call `XListExtensions()` rather than `XQueryExtension()`
to probe for the existence of the GLX extension.  Such applications, when
displaying to VNC or another X server that lacks the GLX extension, would fail.
 VirtualGL now interposes on `XListExtensions()` and makes sure that the GLX
extension is always reported as present.  This fixes a specific issue with
UGS/NX v4 whereby NX, if run with VirtualGL and TurboVNC, would refuse to use
OpenGL to perform its 3D rendering.

12. VGL will now print a warning if the GLX context it obtains on the server's
display is indirect.  This will occur on Solaris if the framebuffer device
permissions do not allow read/write access for the current user.

13. Normally, if X11 drawing is the default in the VirtualGL Client (which is
the case on non-SPARC systems, on SPARC systems with 2D framebuffers, or on any
system if `vglclient` is invoked with the `-x` argument), then the VirtualGL
Client will only use OpenGL for drawing stereo frames.  This fixes a bug
whereby the VirtualGL Client would not switch back to X11 drawing (if X11
drawing was the default) after the application ceased rendering in stereo.

14. You can now specify a listen port number of 0 to make the VirtualGL Client
pick an available port.  This is of only marginal use at the moment, since
there is no way to make the server automatically connect to that port, but we
got this for free as a result of [8].

15. Fixed an issue that was causing the multithreaded tests in `rrfakerut` to
crash some of the nVidia 7xxx series drivers.  `rrfakerut` should now run
cleanly on the 7xxx series, but the multithreaded tests still cause the 8xxx
series drivers to crash & burn, and they cause the 9xxx series drivers to
generate incorrect pixels.  Further investigation is needed.

16. Fixed two issues that were preventing Java 2D on Solaris/SPARC from
properly detecting that OpenGL is available:

    - `glXChooseFBConfig()` now ignores the `GLX_VISUAL_ID` attribute.  That
attribute doesn't really have any meaning in VGL, and passing it through to the
3D X server was causing Java 2D apps to fail.
    - `glXGetFBConfigAttrib(..., GLX_DRAWABLE_TYPE, ...)` now always returns
`GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT`, so that Java 2D apps will
properly detect that window rendering is available in VirtualGL.

17. `vglrun` will now print usage information if you fail to provide an
application command to run.

18. Upgraded the OpenSSL DLL included with the Windows VirtualGL Client to
OpenSSL 0.9.8d (previously 0.9.8c.)

19. VirtualGL now buffers the output from the profiler class so that the
profiling output from multiple compression threads doesn't intermingle when
redirected to a log file.

20. If the `DISPLAY` environment variable is unset on the VirtualGL server,
`vglrun` will now set it automatically to `{ssh_client}:0.0` (where
`{ssh_client}` = the IP address of the SSH client.)  If `DISPLAY` is set to
`localhost:{n}.0` or `{server_hostname}:{n}.0`, VGL assumes that SSH X11
forwarding is in use and sets `VGL_CLIENT={ssh_client}:0.0` instead (without
modifying `DISPLAY`.)  `vglrun` prints a warning that it is doing this.  You
must still explicitly set `VGL_CLIENT` or `DISPLAY` if you are doing something
exotic, such as tunneling VGL's client/server connection through SSH or
displaying to an X server with a display number other than 0.  But this fix
should eliminate the need to set `VGL_CLIENT` and `DISPLAY` in most cases.
Note that the Sun Ray plugin also reads the `VGL_CLIENT` environment variable,
so if you are connecting to the VirtualGL server from a Sun Ray server using
SSH X11 forwarding, this eliminates the need to explicitly set `VGL_CLIENT` in
that case as well.

21. Implemented the "spoil last frame" algorithm in the Sun Ray plugin whenever
VGL reads back/compresses/sends the framebuffer in response to a `glFlush()`.
This fixes issues with slow model regeneration and zoom operations in Pro/E
when using the Sun Ray plugin.

    Normally, the Sun Ray plugin uses the "spoil first frame" algorithm, which
causes the frames in the queue to be discarded whenever a new frame is
received.  However, this algorithm requires that the framebuffer be read back
every time a frame is rendered, even if that frame is ultimately going to be
spoiled.  This causes problems with applications (Pro/E, specifically) that
call `glFlush()` frequently when doing front buffer drawing.  Each of these
`glFlush()` calls triggers a framebuffer readback in VirtualGL, which can cause
significant interaction delays in the application.  The "spoil last frame"
algorithm discards the newest frame if the queue is currently busy, so when
this algorithm is used, most of the duplicate frames triggered by the repeated
`glFlush()` events are discarded with no framebuffer readback (and thus very
little overhead.)

    This fix does not affect Direct Mode and Raw Mode, since those modes
already use the "spoil last frame" algorithm in all cases.

22. Added an option (`VGL_INTERFRAME`), which, when set to `0`, will disable
interframe comparison of rendered frames in Direct Mode.  This was necessary to
work around an interaction issue between VGL and Pro/E Wildfire v3 that led to
slow performance when zooming in or out on the Pro/E model.

23. Added an option (`VGL_LOG`), which can be used to redirect the console
output from the VirtualGL Faker to a file instead of stderr.

24. The mediaLib implementation of TurboJPEG and the mediaLib-accelerated gamma
correction code have been modified slightly, to avoid calls to mediaLib
functions that wrap `memalign()` and `free()`.  This works around an
interaction issue with Pro/E v3 (and potentially with other applications that
use their own custom memory allocators.)


2.0
===

This release was historically part of the Sun Shared Visualization v1.0
product.

### Significant changes relative to 2.0 beta3:

1. Included **libturbojpeg.dylib** in Mac package (so that installing the
TurboJPEG package is no longer necessary unless you want to rebuild VGL from
source.)

2. Included TCBench and NetTest in the Mac package.

3. Added an internal gamma correction system.  This was a last-minute feature
addition to address the fact that there was no way to gamma-correct
applications that were remotely displayed to non-SPARC clients.  SPARC users
are accustomed to OpenGL applications being gamma-corrected by default, so
VirtualGL now mimics this behavior whenever it is running on a SPARC server,
even if the client is not a SPARC machine.

    The behavior of the `VGL_GAMMA` environment variable has changed as
follows:

     - `VGL_GAMMA=1` (or `vglrun +g`):

        Enable gamma correction using the best available method.  If displaying
to a SPARC X server that has gamma-corrected visuals, try to use those
gamma-corrected visuals.  Otherwise, enable VGL's internal gamma correction
system with a gamma correction factor of 2.22.

        This is the default when running applications on SPARC VirtualGL
servers.

     - `VGL_GAMMA=0` (or `vglrun -g`):

        Do not use gamma-corrected visuals (even if available on the X server),
and do not use VGL's internal gamma correction system.

        This is the default when running applications on non-SPARC VirtualGL
servers.

     - `VGL_GAMMA={f}` (or `vglrun -gamma {f}`):

        Do not use gamma-corrected visuals (even if available on the client),
but enable VGL's internal gamma correction system with a gamma correction
factor of {f}.

    Enabling the internal gamma correction system increases CPU usage by a few
percent, but this has not been shown to affect the overall performance of VGL
by a measurable amount.

    See the `VGL_GAMMA` entry in Chapter 18 of the User's Guide for more info.

4. Increased the size of the TurboJPEG compression holding buffer to account
for rare cases in which compressing very-high-frequency image tiles
(specifically parts of the 3D Studio MAX Viewperf test) with high quality
levels (specifically Q99 or above) would produce JPEG images that are larger
than the uncompressed input.

    Linux users will need to upgrade to TurboJPEG 1.04 (or later) to get this
fix.  For other platforms, the fix is included in the VGL 2.0 packages.

5. Minor documentation changes, including a change to the recommended OpenGL
patches for SPARC VGL servers and an application recipe for SDRC I-DEAS Master
Series.

6. Check for exceptions in the `new` operator to prevent VGL from dying
ungracefully in out-of-memory situations.

7. Fixed a bug in the multithreaded compression code whereby it would use too
much memory to hold the image tiles for the second and subsequent compression
threads.  This led to memory exhaustion if the tile size was set to a low value
(such as 16x16 or 32x32.)


2.0 beta3
=========

### Significant changes relative to 2.0 beta2:

1. Solaris packages now include the OpenSSL shared libraries (libssl and
libcrypto), thus eliminating the need to install Blastwave OpenSSL on Solaris 9
and prior.

2. Packages now include a convenience script (`vglgenkey`) that will generate
an xauth key for the 3D X server.  This provides a more secure way of granting
access to the 3D X server than using `xhost +localhost`.

3. Built and packaged the VirtualGL Client for Mac OS/X (Intel only.)  This
package is currently undocumented and includes only the VirtualGL Client (no
server components), but its usage should be self-explanatory for those familiar
with VGL on other platforms (it should work very similarly to the Linux
version.)

4. Normally, when running in Raw Mode, VirtualGL will try to use the `MIT-SHM`
X extension to draw rendered frames.  If `MIT-SHM` is not available or doesn't
work (which would be the case if the X connection is remote), then VGL will try
to use the X `DOUBLE-BUFFER` extension.  Failing that, it will fall back to
single-buffered drawing.  In a Sun Ray environment, the X `DOUBLE-BUFFER`
extension is unstable when Xinerama is used.  `DOUBLE-BUFFER` doesn't really
double-buffer in a Sun Ray environment anyhow, so this release of VirtualGL
disables the use of `DOUBLE-BUFFER` if it detects that it is running in a Sun
Ray environment.

5. Reformatted User's Guide using [Deplate](http://deplate.sourceforge.net).
Numerous other doc changes as well.

6. Extended VGL to support the full GLX 1.4 spec (this involved simply creating
a function stub for `glXGetProcAddress()`, but it was necessary to make J2D
work under Java 6.)

7. Changed the key sequence for the popup configuration dialog (again) to
CTRL-SHIFT-F9 to avoid a conflict with Solaris Common Desktop Environment.

8. VGL no longer tries to switch to Direct Mode automatically whenever a stereo
frame is rendered.  Instead, if running in Raw Mode, it warns that stereo
doesn't work in Raw Mode and proceeds to send only the left eye buffer.


2.0 beta2
=========

### Significant changes relative to 2.0 beta1:

1. Added a `-dl` option to `vglrun`, which inserts a new **libdlfaker.so**
interposer ahead of VirtualGL in the link order.  **libdlfaker.so** intercepts
`dlopen()` calls from an application and, if the application is trying to use
`dlopen()` to open libGL, **librrfaker.so** (the VirtualGL Faker) is opened
instead.  This allows VirtualGL to be used seamlessly with applications that do
not dynamically link against libGL and do not provide any sort of override
mechanism.  See the User's Guide for a list of 3D applications that are known
to require the `-dl` option.

2. Bug fixed:  Lazy loading of OpenSSL on Solaris now works properly, so
OpenSSL doesn't have to be installed unless VirtualGL is being used in SSL
mode.

3. Added "application recipes" section to User's Guide.

4. Tweaked FBX library to improve performance of VGL when used with NX and
FreeNX.

5. Bug fixed:  TurboJPEG for Solaris did not properly handle 4:2:0
decompression if no pixel format conversion was required.

6. Bug fixed:  Minor (and somewhat esoteric) visual matching bug in GLP mode

7. Bug fixed:  In GLX mode, VGL did not properly handle FB configs that had no
associated X Visual type.

8. Stereo now works on Solaris/SPARC servers using kfb framebuffers in both GLP
and GLX modes.  Sun OpenGL patch 120812-12 (or later) and KFB driver patch
120928-10 (or later) are required.

9. Bug fixed:  In GLX mode, some of the "real" GLX functions that VirtualGL
calls on the server will in turn call `glXGetClientString()`,
`glXQueryExtensions()`, etc. to list extensions available on the server's X
display (the 3D X server.)  VirtualGL interposes these functions and returns
its own extension strings rather than the "real" strings from the 3D X server,
and this caused problems under certain circumstances.  These interposed
functions will now detect whether they are being called from within another GLX
function and will return the "real" extension strings for the 3D X server if
so.

10. Generally friendlier error messages

11. Bug fixed:  Destroying an overlay window would cause VGL to segfault.

12. Changed configuration dialog popup key sequence to CTRL-SHIFT-F12 to avoid
conflict with KDE.

13. Added `VGL_GUI_XTHREADINIT` environment variable to optionally disable
VGL's use of `XtToolkitThreadInitialize()`.  Rarely, a multithreaded Motif
application relies on its own locking mechanisms and will deadlock if Motif's
built-in application and process locks are enabled.  Thus far, the only known
application that this affects is VisConcept.  Set `VGL_GUI_XTHREADINIT` to `0`
to prevent VisConcept from deadlocking when the VGL config dialog is activated.

14. VirtualGL will no longer allow a non-TrueColor stereo visual to be
selected, because such visuals won't work with the VirtualGL Client.  Color
index (PseudoColor) rendering requires Raw Mode, but stereo requires Direct
Mode, so VirtualGL cannot transmit PseudoColor rendered stereo frames.

15. VirtualGL will now automatically select Raw Mode if it detects that it is
running on a "local" display (i.e. ":{n}.0", "unix:{n}.0", etc., but not
"localhost:{n}.0".)  This allows users of X proxies such as Sun Ray, TurboVNC,
NX, etc. to run VirtualGL without having to pass an argument of `-c 0` to
`vglrun`.  "localhost:{n}.0" is not considered a "local" display, and neither
is "{server_hostname}:{n}.0", because display connections tunneled through SSH
will have a display name that takes on one of those forms.

16. The VirtualGL Client for Solaris/SPARC will now automatically detect if it
is running on a framebuffer that uses software OpenGL and will default to using
X11 drawing in that case.  Otherwise, if hardware-accelerated OpenGL is
available, then the VirtualGL Client for Solaris/SPARC will try to use OpenGL
as the default drawing method.  This improves performance on the XVR-100 and
similar framebuffers as well as working around a bug in the OGL implementation
on XVR-100 (see CR 6408535.)  Non-SPARC implementations of the VirtualGL Client
still use X11 drawing by default, except when drawing stereo frames.

17. Worked around some issues in Sun OpenGL that manifested themselves in
instability of the SPARC VirtualGL Client when drawing large numbers of
simultaneous frames from different servers.

18. Expanded N1 GridEngine hooks in `vglrun` so that the `VGL_CLIENT`,
`VGL_COMPRESS`, `VGL_PORT`, `VGL_SSL`, `VGL_GLLIB`, and `VGL_X11LIB`
environment variables will be automatically passed to `vglrun` by N1GE.  All
other variables must be explicitly passed by using the `-v` option to qsub.

19. Bug fixed:  Lazy loading of OpenGL now works properly on Solaris clients,
so the VirtualGL Client can be run on machines that don't have OpenGL
installed.

20. For some reason, popping up the VGL configuration dialog when running Java
apps caused VGL's Xt resource string to be ignored, which would cause the
widgets to be displayed incorrectly.  Thus, VirtualGL now explicitly specifies
the resources when creating each widget and doesn't rely on the use of a
resource string.
