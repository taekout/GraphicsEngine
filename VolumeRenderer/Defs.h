#pragma once

#include <functional>

#define CHECK_OPENGL_ERROR CheckOpenGLErrors(__LINE__, __FILE__);

enum EDirection {
	left,
	right,
	forward,
	backward,
	up,
	down
};

#define printOpenGLError() printOglError(__FILE__, __LINE__)

int printOglError(char *file, int line);

#define forit( container )	for(auto i = container.begin() ; i != container.end() ; ++i)
#define fori( array, size )	for(size_t i = 0 ; i < size ; i++)

unsigned int loadBMP_custom(const char * imagepath);


struct TexInspectorIn
{
	TexInspectorIn(size_t inWidth,
		size_t inHeight,
		size_t inBytesPerPixel,
		unsigned char * inByteData,
		const std::string & inTexName)
	{
		fWidth = inWidth;
		fHeight = inHeight;
		fBytesPerPixel = inBytesPerPixel;
		fByteData = inByteData;
		fTexName = inTexName;
	}

	size_t fWidth;
	size_t fHeight;
	size_t fBytesPerPixel;
	unsigned char * fByteData;
	std::string fTexName;
};

struct TexInspectorOut
{
	TexInspectorOut()
	{
		fWriteToDisk = false;
	}

	bool fWriteToDisk;
};

struct FloatTexConverterIn
{
	FloatTexConverterIn(size_t inWidth, size_t inHeight, size_t inFloatsPerPixel, float * inFloatData)
	{
		fWidth = inWidth;
		fHeight = inHeight;
		fFloatsPerPixel = inFloatsPerPixel;
		fFloatData = inFloatData;
	}

	size_t fWidth;
	size_t fHeight;
	size_t fFloatsPerPixel;
	float * fFloatData;
};

struct FloatTexConverterOut
{
	FloatTexConverterOut(unsigned char * inByteData)
	{
		fByteData = inByteData;
	}

	unsigned char * fByteData;
};

class GEDiagnostics
{
public:
	static void WriteGLTexturesToDisk();

	static void WriteGLTexturesToDisk(
		std::function<void(const TexInspectorIn & in, TexInspectorOut & out)> inTexInspector,
		std::function<void(const FloatTexConverterIn & in, FloatTexConverterOut & out)> inFloatTexConverter = std::function<void(const FloatTexConverterIn & in, FloatTexConverterOut & out)>());
};




