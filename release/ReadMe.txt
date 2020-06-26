VirtualGL is a toolkit that allows most Linux/Unix OpenGL applications to be remotely displayed with 3D hardware acceleration to thin clients, regardless of whether the clients have 3D rendering capabilities, and regardless of the size of the 3D data being rendered or the speed of the network.

Using the vglrun script on a Linux or Unix server, the VirtualGL Faker library is preloaded into an OpenGL application at run time.  The faker then intercepts and modifies ("interposes") certain GLX, OpenGL, X11, and XCB function calls in order to seamlessly redirect 3D rendering from the application's windows into corresponding GPU-attached off-screen buffers, read back the rendered frames, and transport the frames to the "2D X Server" (the X server to which the application's GUI is being displayed), where the frames are composited back into the application's windows.

VirtualGL can be used to give hardware-accelerated 3D rendering capabilities to VNC and other X proxies that either lack OpenGL support or provide it through a software renderer.  In a LAN environment, VGL can also be used with its built-in high-performance image transport (the VGL Transport), which sends the rendered frames to the VirtualGL Client application for compositing on a client-side 2D X server.

This package includes the VirtualGL Client application (vglclient) for Intel-based Macs and the vglconnect script.  It is not necessary to install this package when using VirtualGL with VNC and other X proxies.  XQuartz must be installed prior to using the VirtualGL Client.

Refer to the VirtualGL User's Guide (a link to which is provided in the Applications folder) for usage information.
