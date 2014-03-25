VirtualGL is a toolkit that allows most Unix/Linux OpenGL applications to be remotely displayed with hardware 3D acceleration to thin clients, regardless of whether the clients have 3D capabilities, and regardless of the size of the 3D data being rendered or the speed of the network.

Using the vglrun script on a Unix or Linux server, the VirtualGL "faker" is loaded into an OpenGL application.  The faker then intercepts a handful of GLX calls, which it reroutes to the server's X display (the "3D X Server", which presumably has a 3D accelerator attached.)  The GLX commands are also dynamically modified such that all rendering is redirected into a Pbuffer instead of a window.  As each frame is rendered by the application, the faker reads back the pixels from the 3D accelerator and sends them to the "2D X Server" for compositing into the appropriate X Window. 

VirtualGL can be used to give hardware-accelerated 3D capabilities to VNC or other X proxies that either lack OpenGL support or provide it through software rendering.  In a LAN environment, VGL can also be used with its built-in high-performance image transport, which sends the rendered 3D images to a remote client (vglclient) for compositing on a remote X server.

This package includes the VirtualGL Client applications (vglconnect and vglclient) for Intel-based Macintosh computers.  It is not necessary to install this package when using VirtualGL with VNC and other X proxies.  The Macintosh X11 application (which is not installed by default but which can be installed from the OS X distribution discs) must be installed prior to using the VirtualGL Client.

Refer to the VirtualGL documentation (a link to which is provided in the Applications folder) for usage information.
