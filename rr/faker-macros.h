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

#define shimfuncdpy2(rettype, funcname, at1, a1, at2, a2, ret)  \
	rettype funcname(at1 a1, at2 a2) {  \
		dpy=_localdpy;  \
		ret _##funcname(a1, a2);  \
	}

#define shimfuncdpy3(rettype, funcname, at1, a1, at2, a2, at3, a3, ret)  \
	rettype funcname(at1 a1, at2 a2, at3 a3) {  \
		dpy=_localdpy;  \
		ret _##funcname(a1, a2, a3);  \
	}

#define shimfuncdpy4(rettype, funcname, at1, a1, at2, a2, at3, a3, at4, a4, ret)  \
	rettype funcname(at1 a1, at2 a2, at3 a3, at4 a4) {  \
		dpy=_localdpy;  \
		ret _##funcname(a1, a2, a3, a4);  \
	}

#define shimfuncdpy5(rettype, funcname, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, ret)  \
	rettype funcname(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) {  \
		dpy=_localdpy;  \
		ret _##funcname(a1, a2, a3, a4, a5);  \
	}

#define shimfuncdpyvis2(rettype, funcname, at1, a1, at2, a2, ret)  \
	rettype funcname(at1 a1, at2 a2) {  \
		if(dpy!=_localdpy) \
		{ \
			if(vis)  \
			{  \
				XVisualInfo *_localvis=_MatchVisual(dpy, vis);  \
				if(_localvis) vis=_localvis;  \
			}  \
			dpy=_localdpy; \
		} \
		ret _##funcname(a1, a2);  \
	}

#define shimfuncdpyvis3(rettype, funcname, at1, a1, at2, a2, at3, a3, ret)  \
	rettype funcname(at1 a1, at2 a2, at3 a3) {  \
		if(dpy!=_localdpy) \
		{ \
			if(vis)  \
			{  \
				XVisualInfo *_localvis=_MatchVisual(dpy, vis);  \
				if(_localvis) vis=_localvis;  \
			}  \
			dpy=_localdpy; \
		} \
		ret _##funcname(a1, a2, a3);  \
	}

#define shimfuncdpyvis4(rettype, funcname, at1, a1, at2, a2, at3, a3, at4, a4, ret)  \
	rettype funcname(at1 a1, at2 a2, at3 a3, at4 a4) {  \
		if(dpy!=_localdpy) \
		{ \
			if(vis)  \
			{  \
				XVisualInfo *_localvis=_MatchVisual(dpy, vis);  \
				if(_localvis) vis=_localvis;  \
			}  \
			dpy=_localdpy; \
		} \
		ret _##funcname(a1, a2, a3, a4);  \
	}
