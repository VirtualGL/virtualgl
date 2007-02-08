#ifdef linux
TURBOJPEG
#endif
{
	global:
		tjInitCompress;
		tjCompress;
		TJBUFSIZE;
		tjInitDecompress;
		tjDecompressHeader;
		tjDecompress;
		tjDestroy;
		tjGetErrorStr;
	local:
		*;
};
