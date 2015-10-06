#include "Defs.h"
#include <stdio.h>
#include <stdlib.h>
#include "glew.h"
#include "glut.h"
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#include <Windows.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/matrix_inverse.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)  // sprintf unsafe

void DMSG(const char * msg)
{
	wchar_t dst[1024];
	mbstowcs (dst, msg, 1024);
	OutputDebugString(dst);
}


#define for_i(size) for( size_t i=0; i<size; i++ )
#define for_j(size) for( size_t j=0; j<size; j++ )
#define for_k(size) for( size_t k=0; k<size; k++ )
#define for_m(size) for( size_t m=0; m<size; m++ )

#define for_it( container ) for( auto it = (container).begin(); it != (container).end(); ++it )
#define for_jt( container ) for( auto jt = (container).begin(); jt != (container).end(); ++jt )
#define for_kt( container ) for( auto kt = (container).begin(); kt != (container).end(); ++kt )
#define for_mt( container ) for( auto mt = (container).begin(); mt != (container).end(); ++mt )


void GEDiagnostics::WriteGLTexturesToDisk()
{
	auto texInspector = [](const TexInspectorIn & in, TexInspectorOut & out)
	{
		out.fWriteToDisk = true;
	};

	WriteGLTexturesToDisk(texInspector);
}

void GEDiagnostics::WriteGLTexturesToDisk(
	std::function<void(const TexInspectorIn & in, TexInspectorOut & out)> inTexInspector,
	std::function<void(const FloatTexConverterIn & in, FloatTexConverterOut & out)> inFloatTexConverter)
{
#pragma warning(push)
#pragma warning(disable : 4996)  // sprintf unsafe

	auto defaultFloatTexConverter = [&](const FloatTexConverterIn & in, FloatTexConverterOut & out)
	{
		size_t size = in.fWidth * in.fHeight * in.fFloatsPerPixel;

		glm::vec4 minData(FLT_MAX);
		glm::vec4 maxData(-FLT_MAX);

		for( size_t i=0; i<size; i+=in.fFloatsPerPixel )
		{
			for_j( in.fFloatsPerPixel )
			{
				minData[j] = glm::min(minData[j], in.fFloatData[i+j]);
				maxData[j] = glm::max(maxData[j], in.fFloatData[i+j]);
			}
		}

		glm::vec4 range;
		for_j( in.fFloatsPerPixel )
			range[j] = maxData[j] - minData[j];

		for( size_t i=0; i<size; i+=in.fFloatsPerPixel )
			for_j( in.fFloatsPerPixel )
			out.fByteData[i+j] = (unsigned char) (glm::clamp<float>((in.fFloatData[i+j]-minData[j]) / range[j], 0, 1) * 255);
	};


#if GS_WIN
	const char * logPath = "c:\\a\\";
#else
	const char * logPath = "/a/";
#endif

#if GS_WIN
	HDC dc = wglGetCurrentDC();
	HGLRC rc = wglGetCurrentContext();

	gSDK->Kludge(kKludgeGetDCRC, 0, 0);	
#endif

	static std::vector<unsigned char> byteData;
	static std::vector<float> floatData;

	int activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

	int originalBoundTex = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &originalBoundTex);

	int maxColorAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

	int depthAttachment = -1;
	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthAttachment);

	std::map<GLuint, int> colorAttachments;
	for( int i=0; i<maxColorAttachments; i++ )
	{
		int name;
		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

		if( name != 0 ) colorAttachments[name] = i;
	}

	int maxTextureUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

	std::map<GLuint, int> boundTextures;
	for( int i=0; i<maxTextureUnits; i++ ) {
		int boundTexture;
		glActiveTexture(GL_TEXTURE0+i);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);

		if( boundTexture ) {
			boundTextures[boundTexture] = i;
		}
	}

	glActiveTexture(activeTexture);

	// run through a bunch of potential opengl texture ids
	for( int i=1; i<200; i++ ) 
	{  
		if( glIsTexture(i) ) 
		{
			glBindTexture(GL_TEXTURE_2D, i);

			int width, height, format, components;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);  // Danger, GL_TEXTURE_INTERNAL_FORMAT equals GL_TEXTURE_COMPONENTS
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &components);

			bool floatFormat = false;

			if( format == 1 )
			{
				format = GL_RED;
				components = 1;
			}
			else if( format == 2 ) 
			{
				format = GL_RG;
				components = 2;
			}
			else if( format == 3 || format == GL_RGB8 ) 
			{
				format = GL_RGB;
				components = 3;
			}
			else if( format == 4 || format == GL_RGBA8 ) 
			{
				format = GL_RGBA;
				components = 4;
			}
			else if( format==GL_DEPTH24_STENCIL8 || format==GL_DEPTH_COMPONENT || format==GL_DEPTH_COMPONENT16 || format==GL_DEPTH_COMPONENT24 || format==GL_DEPTH_COMPONENT32 ) 
			{
				floatFormat = true;
				format = GL_DEPTH_COMPONENT;
				components = 1;
			}
			else
			{
				floatFormat = true;

				switch( format ) {
				case GL_RGBA:
				case GL_RGBA8:
#if GS_WIN
				case GL_RGBA16F:
				case GL_RGBA32F:
#else
				case GL_RGBA16F_ARB:
				case GL_RGBA32F_ARB:
#endif
					format = GL_RGBA;
					components = 4;
					break;
				case GL_RGB:
#if GS_WIN
				case GL_RGB16F:
				case GL_RGB32F:
#else
				case GL_RGB16F_ARB:
				case GL_RGB32F_ARB:
#endif
					format = GL_RGB;
					components = 3;
					break;
				case GL_RG:
				case GL_RG16F:
				case GL_RG32F:
					format = GL_RG;
					components = 2;
					break;
				case GL_RED:
				case GL_R32F:
					format = GL_RED;
					components = 1;
					break;
				default:
					format = GL_RGB;
					components = 3;
					break;
				}
			}


			char filename[256];
			int len=0;

			len += sprintf(filename+len, "%stexture%d %dx%d %dBpp", logPath, i, width, height, components);
			if( boundTextures.find(i) != boundTextures.end() ) len += sprintf(filename+len, " GL_TEXTURE%d", boundTextures[i]);
			if( colorAttachments.find(i) != colorAttachments.end() ) len += sprintf(filename+len, " GL_COLOR_ATTACHMENT%d", colorAttachments[i]);
			if( i == depthAttachment ) len += sprintf(filename+len, " GL_DEPTH_ATTACHMENT");


			int size = width * height * components;
			if(size == 0)
				continue;
			byteData.resize(size);


			if( floatFormat )
			{
				floatData.resize(size);

				glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, &floatData[0]);

				FloatTexConverterIn in(width, height, components, &floatData[0]);
				FloatTexConverterOut out(&byteData[0]);

				if( inFloatTexConverter )
					inFloatTexConverter(in, out);
				else
					defaultFloatTexConverter(in, out);
			}
			else
				glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &byteData[0]);

			int err = glGetError();
			err = 0;

			TexInspectorOut texInspectorOut;

			if( inTexInspector )
			{
				TexInspectorIn in(width, height, components, &byteData[0], filename);
				inTexInspector(in, texInspectorOut);
			}

			if( texInspectorOut.fWriteToDisk )
			{




#if WRITE_PNG_FILES

				len += sprintf(filename+len, ".png");
				SaveImageToDisk(width, height, components, &byteData[0], filename);
#else
				len += sprintf(filename+len, ".raw");

				std::ofstream out(filename, std::ios_base::binary);
				out.write((const char *)&byteData[0], size);
				out.close();
#endif
			}
		}
	}

	// Write stencil buffer to disk
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		byteData.resize(viewport[0]+viewport[2] * viewport[1]+viewport[3]);

		glReadPixels(0, 0, viewport[0]+viewport[2], viewport[1]+viewport[3], GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &byteData[0]);

		char filename[255];
		int len = sprintf(filename, "%sstencil %dx%d 1Bpp", logPath, viewport[0]+viewport[2], viewport[1]+viewport[3]);

#if WRITE_PNG_FILES
		len += sprintf(filename+len, ".png");
		SaveImageToDisk(viewport[0]+viewport[2], viewport[1]+viewport[3], 1, &byteData[0], filename);
#else
		len += sprintf(filename+len, ".raw");
		std::ofstream out(filename, std::ios_base::binary);
		size_t size = (viewport[0]+viewport[2]) * (viewport[1]+viewport[3]);
		out.write((const char *)&byteData[0], size);
		out.close();
#endif
	}


	glBindTexture(GL_TEXTURE_2D, originalBoundTex);

#pragma warning(pop)  // sprintf unsafe

#if GS_WIN
	wglMakeCurrent(dc, rc);
#endif
}


struct DiffChecker
{
	DiffChecker(char * inText, ptrdiff_t & inLen, bool inFirst)
		: fText(inText), fLen(inLen), fFirst(inFirst) { }

	template<typename T>
	T & operator()(T & inStaticT, T inT)
	{
		if( !fFirst && inStaticT != inT )
			fLen += sprintf(fText+fLen, "****");

		inStaticT = inT;

		return inStaticT;
	}

	template<typename T>
	T & operator()(size_t inSize, T * inStaticT, T * inT)
	{
		if( !fFirst )
		{
			size_t i = 0;
			for( ; i<inSize; i++ )
				if( inStaticT[i] != inT[i] )
					break;

			if( i < inSize )
				fLen += sprintf(fText+fLen, "****");
		}

		for_i( inSize )
			inStaticT[i] = inT[i];

		return inStaticT[0];
	}

	bool fFirst;
	char * fText;
	ptrdiff_t & fLen;
};


// Diagnostics functions
void GL_PrintState() 
{
	static bool first = true;

	int i;
	GLint is[16];
	GLboolean b;

	char text[1024];
	ptrdiff_t len = 0;

	DiffChecker diffCheck(text, len, first);

#if GS_WIN
	void* ptr = wglGetCurrentContext();
	static void * context;
	len += sprintf(text+len, "gl context = %p\n", diffCheck(context, ptr));
#endif

	len += sprintf(text+len, "glPixelStore\n");

	glGetIntegerv(GL_PACK_ALIGNMENT, &i);
	static int packAlignment;
	len += sprintf(text+len, "\tpack alignment = %d\n", diffCheck(packAlignment, i));

	glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &i);
	static int packImageHeight;
	len += sprintf(text+len, "\tpack image height = %d\n", diffCheck(packImageHeight, i));

	glGetIntegerv(GL_PACK_ROW_LENGTH, &i);
	static int packRowLength;
	len += sprintf(text+len, "\tpack row length = %d\n", diffCheck(packRowLength, i));

	glGetIntegerv(GL_PACK_SKIP_PIXELS, &i);
	static int packSkipPixels;
	len += sprintf(text+len, "\tpack skip pixels = %d\n", diffCheck(packSkipPixels, i));

	glGetIntegerv(GL_PACK_SKIP_ROWS, &i);
	static int packSkipRows;
	len += sprintf(text+len, "\tpack skip rows = %d\n", diffCheck(packSkipRows, i));

	glGetIntegerv(GL_UNPACK_ALIGNMENT, &i);
	static int unpackAlignment;
	len += sprintf(text+len, "\tunpack alignment = %d\n", diffCheck(unpackAlignment, i));

	glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &i);
	static int unpackImageHeight;
	len += sprintf(text+len, "\tunpack image height = %d\n", diffCheck(unpackImageHeight, i));

	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &i);
	static int unpackRowLength;
	len += sprintf(text+len, "\tunpack row length = %d\n", diffCheck(unpackRowLength, i));

	glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &i);
	static int unpackSkipPixels;
	len += sprintf(text+len, "\tunpack skip pixels = %d\n", diffCheck(unpackSkipPixels, i));

	glGetIntegerv(GL_UNPACK_SKIP_ROWS, &i);
	static int unpackSkipRows;
	len += sprintf(text+len, "\tunpack skip rows = %d\n", diffCheck(unpackSkipRows, i));

	// flush now -- DMSG only supports 1024 characters
	DMSG(text);
	
	text[len = 0] = 0;


	glGetIntegerv(GL_COLOR_WRITEMASK, is);
	static GLint colorWriteMask[4];
	len += sprintf(text+len, "glColorMask(%d, %d, %d, %d)\n", diffCheck(4, colorWriteMask, is), is[1], is[2], is[3]);

	b = glIsEnabled(GL_BLEND);
	static GLboolean blendingEnabled;
	len += sprintf(text+len, "blending enabled = %d\n", diffCheck(blendingEnabled, b));

	if( b )
	{
		glGetIntegerv(GL_BLEND_EQUATION_RGB, &is[0]);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &is[1]);
		static int blendEquationSeparate[2];
		len += sprintf(text+len, "\tglBlendEquationSeparate(0x%x, 0x%x)\n", diffCheck(2, blendEquationSeparate, is), is[1]);

		glGetIntegerv(GL_BLEND_SRC_RGB, &is[0]);
		glGetIntegerv(GL_BLEND_DST_RGB, &is[1]);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &is[2]);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &is[3]);
		static int blendFuncSeparate[4];
		len += sprintf(text+len, "\tglBlendFuncSeparate(0x%x, 0x%x, 0x%x, 0x%x)\n", diffCheck(4, blendFuncSeparate, is), is[1], is[2], is[3]);
	}

	b = glIsEnabled(GL_DEPTH_TEST);
	static GLboolean depthTestEnabled;
	len += sprintf(text+len, "depth test enabled = %d\n", diffCheck(depthTestEnabled, b));

	if( b ) {
		glGetIntegerv(GL_DEPTH_FUNC, &i);
		static int depthFunc;
		len += sprintf(text+len, "\tglDepthFunc(0x%x)\n", diffCheck(depthFunc, i));

		glGetIntegerv(GL_DEPTH_WRITEMASK, &i);
		static GLint depthWriteMask;
		len += sprintf(text+len, "\tglDepthMask(%d)\n", diffCheck(depthWriteMask, i));
	}

	// flush now -- DMSG only supports 1024 characters
	DMSG(text);
	text[len = 0] = 0;


	b = glIsEnabled(GL_STENCIL_TEST);
	static GLboolean stencilTestEnabled;
	len += sprintf(text+len, "stencil test enabled = %d\n", diffCheck(stencilTestEnabled, b));

	if( b )
	{
		glGetIntegerv(GL_STENCIL_FUNC, &is[0]);
		glGetIntegerv(GL_STENCIL_REF, &is[1]);
		glGetIntegerv(GL_STENCIL_VALUE_MASK, &is[2]);
		static GLint stencilFunc_Ref_Mask[3];
		len += sprintf(text+len, "\tglStencilFunc(0x%x, 0x%x, 0x%x)\n", diffCheck(3, stencilFunc_Ref_Mask, is), is[1], is[2]);

		glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &is[0]);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &is[1]);
		glGetIntegerv(GL_STENCIL_FAIL, &is[2]);
		static GLint stencilFail_PassDepthFail_PassDepthPass[3];
		len += sprintf(text+len, "\tglStencilOp(0x%x, 0x%x, 0x%x)\n", diffCheck(3, stencilFail_PassDepthFail_PassDepthPass, is), is[1], is[2]);
	}

	b = glIsEnabled(GL_ALPHA_TEST);
	static GLboolean alphaTestEnabled;
	len += sprintf(text+len, "alpha test enabled = %d\n", diffCheck(alphaTestEnabled, b));

	b = glIsEnabled(GL_SCISSOR_TEST);
	static GLboolean scissorTestEnabled;
	len += sprintf(text+len, "scissor test enabled = %d\n", diffCheck(scissorTestEnabled, b));

	b = glIsEnabled(GL_CULL_FACE);
	static GLboolean cullFaceEnabled;
	len += sprintf(text+len, "cull face enabled = %d\n", diffCheck(cullFaceEnabled, b));

	if( b ) {
		glGetIntegerv(GL_CULL_FACE_MODE, &i);
		static int cullFaceMode;
		len += sprintf(text+len, "\tglCullFace(0x%x)\n", diffCheck(cullFaceMode, i));

		glGetIntegerv(GL_FRONT_FACE, &i);
		static int frontFace;
		len += sprintf(text+len, "\tglFrontFace(0x%x)\n", diffCheck(frontFace, i));
	}


	b = glIsEnabled(GL_POLYGON_OFFSET_FILL);
	static GLboolean polygonOffsetFillEnabled;
	len += sprintf(text+len, "polygon offset fill enabled = %d\n", diffCheck(polygonOffsetFillEnabled, b));

	b = glIsEnabled(GL_POLYGON_OFFSET_LINE);
	static GLboolean polygonOffsetLineEnabled;
	len += sprintf(text+len, "polygon offset line enabled = %d\n", diffCheck(polygonOffsetLineEnabled, b));

#if GL_VERSION_2_1	
	static std::pair<GLboolean, glm::dvec4> clipPlanes[6];
	for_i( 6 )
	{
		std::pair<GLboolean, glm::dvec4> clipPlane;
		GLenum index = GLenum(GL_CLIP_PLANE0+i);
		clipPlane.first = glIsEnabled(index);
		glGetClipPlane(index, &clipPlane.second[0]);

		len += sprintf(text+len, "  clipPlane%d enabled=%d equation=%f, %f, %f, %f\n", i, diffCheck(clipPlanes[i], clipPlane).first,
			clipPlane.second[0], clipPlane.second[1], clipPlane.second[2], clipPlane.second[3]);
	}
#endif

	glGetIntegerv(GL_VIEWPORT, is);
	static int viewport[4];
	len += sprintf(text+len, "glViewport(%d, %d, %d, %d)\n", diffCheck(4, viewport, is), is[1], is[2], is[3]);

	// flush now -- DMSG only supports 1024 characters
	DMSG(text);
	text[len = 0] = 0;

#if GL_VERSION_3_2
	for_i( 6 )
		is[i] = glIsEnabled(GL_CLIP_DISTANCE0+GLenum(i));
	static int clipDistEnabled[6];
	len += sprintf(text+len, "GL_CLIP_DISTANCEi enabled = %d, %d, %d, %d, %d, %d\n", diffCheck(6, clipDistEnabled, is), is[1], is[2], is[3], is[4], is[5]);
#endif

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &i);
	static int boundDrawFBO;
	len += sprintf(text+len, "bound draw fbo = %d\n", diffCheck(boundDrawFBO, i));

	int maxColorAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

	std::map<GLenum, std::string> attachmentType;
	attachmentType[GL_NONE] = "GL_NONE";
	attachmentType[GL_FRAMEBUFFER_DEFAULT] = "GL_FRAMEBUFFER_DEFAULT";
	attachmentType[GL_TEXTURE] = "GL_TEXTURE";
	attachmentType[GL_RENDERBUFFER] = "GL_RENDERBUFFER";

	static int fboAttachmentData[16*3]; // [attachment0, type0, name0,  attachment1, type1, name1,  etc..]

	for( int j=0; j<maxColorAttachments; ++j ) {
		is[0] = j;

		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+j, 
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &is[1]);

		glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+j, 
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &is[2]);

		len += sprintf(text+len, "\tattachment%d  type=%s  name=%d\n", diffCheck(3, fboAttachmentData+j*3, is), attachmentType[is[1]].c_str(), is[2]);

		DMSG(text);  // flush DMSG can only print 1024 characters
		text[len = 0] = 0;

	}

	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &i);
	static int depthAttachment;
	len += sprintf(text+len, "GL_DEPTH_ATTACHMENT name=%d\n", diffCheck(depthAttachment, i));

	glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, 
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &i);
	static int stencilAttachment;
	len += sprintf(text+len, "GL_STENCIL_ATTACHMENT name=%d\n", diffCheck(stencilAttachment, i));


	i = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	static int drawFramebufferComplete;
	len += sprintf(text+len, "draw FBO check = %d  [%d]\n", diffCheck(drawFramebufferComplete, i), GL_FRAMEBUFFER_COMPLETE);

	i = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
	static int readFramebufferComplete;
	len += sprintf(text+len, "read FBO check = %d  [%d]\n", diffCheck(readFramebufferComplete, i), GL_FRAMEBUFFER_COMPLETE);


	int maxDrawBuffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

	std::map<GLenum, std::string> drawBuffers;  // maps glDrawBuffer(s) values to human readable strings

	drawBuffers[GL_NONE] = "GL_NONE";
	drawBuffers[GL_FRONT_LEFT] = "GL_FRONT_LEFT";
	drawBuffers[GL_FRONT_RIGHT] = "GL_FRONT_RIGHT";
	drawBuffers[GL_BACK_LEFT] = "GL_BACK_LEFT";
	drawBuffers[GL_BACK_RIGHT] = "GL_BACK_RIGHT";

	for( int i=0; i<maxDrawBuffers; ++i ) 
	{
		char colorAttachment[256];
		sprintf(colorAttachment, "GL_COLOR_ATTACHMENT%d", i);
		drawBuffers[GL_COLOR_ATTACHMENT0+i] = colorAttachment;
	}

	len += sprintf(text+len, "draw buffers\n\t");

	static int boundDrawBuffers[16];

	// write the draw buffers
	for( int i=0; i<maxDrawBuffers; ++i )
		glGetIntegerv(GL_DRAW_BUFFER0+i, &is[i]);

	bool diff = false;
	for( int i=0; i<maxDrawBuffers; ++i )
	{
		if( !first && boundDrawBuffers[i] != is[i] )
			diff = true;

		boundDrawBuffers[i] = is[i];
	}

	if( diff )
		len += sprintf(text+len, "****");

	for( int i=0; i<maxDrawBuffers; ++i )
		len += sprintf(text+len, "%s ", drawBuffers[boundDrawBuffers[i]].c_str());

	len += sprintf(text+len, "\n");


	glGetIntegerv(GL_CURRENT_PROGRAM, &i);
	static int program;
	len += sprintf(text+len, "current program = %d\n", diffCheck(program, i));

	if( glIsProgram(i) ) {
		int activeUniforms;
		glGetProgramiv(i, GL_ACTIVE_UNIFORMS, &activeUniforms);

		std::map<int, int> componentMap;
		componentMap[GL_FLOAT] = 1;
		componentMap[GL_FLOAT_VEC2] = 2;
		componentMap[GL_FLOAT_VEC3] = 3;
		componentMap[GL_FLOAT_VEC4] = 4;
		componentMap[GL_FLOAT_MAT2] = 4;
		componentMap[GL_FLOAT_MAT3] = 9;
		componentMap[GL_FLOAT_MAT4] = 16;
		componentMap[GL_FLOAT_MAT2x3] = 6;
		componentMap[GL_FLOAT_MAT2x4] = 8;
		componentMap[GL_FLOAT_MAT3x2] = 6;
		componentMap[GL_FLOAT_MAT3x4] = 12;
		componentMap[GL_FLOAT_MAT4x2] = 8;
		componentMap[GL_FLOAT_MAT4x3] = 12;
		componentMap[GL_INT] = 1;
		componentMap[GL_INT_VEC2] = 2;
		componentMap[GL_INT_VEC3] = 3;
		componentMap[GL_INT_VEC4] = 4;
		componentMap[GL_UNSIGNED_INT] = 1;
		//		componentMap[GL_UNSIGNED_INT_VEC2] = 2;
		//		componentMap[GL_UNSIGNED_INT_VEC3] = 3;
		//		componentMap[GL_UNSIGNED_INT_VEC4] = 4;
		componentMap[GL_BOOL] = 1;
		componentMap[GL_BOOL_VEC2] = 2;
		componentMap[GL_BOOL_VEC3] = 3;
		componentMap[GL_BOOL_VEC4] = 4;
		// the rest are 1

		for(GLuint j=0; j < GLuint(activeUniforms); j++ ) {
			int length;
			int size;
			GLenum type;
			char name[256];
			glGetActiveUniform(i, j, 255, &length, &size, &type, name);
			int loc = glGetUniformLocation(i, name);

			char * bracketOffset = strstr(name, "[");
			if( bracketOffset )
				*bracketOffset = 0;

			int ints[16];
			float floats[16];

			int components = 1;

			if( componentMap.find(type) != componentMap.end() )
				components = componentMap[type];


			if( type == GL_FLOAT 
				|| type == GL_FLOAT_VEC2
				|| type == GL_FLOAT_VEC3
				|| type == GL_FLOAT_VEC4
				|| type == GL_FLOAT_MAT2
				|| type == GL_FLOAT_MAT3
				|| type == GL_FLOAT_MAT4
				|| type == GL_FLOAT_MAT2x3
				|| type == GL_FLOAT_MAT2x4
				|| type == GL_FLOAT_MAT3x2
				|| type == GL_FLOAT_MAT3x4
				|| type == GL_FLOAT_MAT4x2
				|| type == GL_FLOAT_MAT4x3 ) 
			{
				for( int k=0; k<size; k++ ) 
				{
					char element[32];
					element[0] = 0;

					if( size > 1 )
						sprintf(element, "[%d]", k);

					glGetUniformfv(i, loc+k, floats);
					len += sprintf(text+len, "\t%s%s = ", name, element);

					len += sprintf(text+len, "%f", floats[0]);
					for( int c=1; c<components; c++ )
						len += sprintf(text+len, ", %f", floats[c]);

					len += sprintf(text+len, "\n");
				}

			} 
			else 
			{
				for( int k=0; k<size; k++ ) 
				{
					char element[32];
					element[0] = 0;

					if( size > 1 )
						sprintf(element, "[%d]", k);

					glGetUniformiv(i, loc+k, ints);
					len += sprintf(text+len, "\t%s%s = ", name, element);

					len += sprintf(text+len, "%d", ints[0]);
					for( int c=1; c<components; c++ )
						len += sprintf(text+len, ", %f", floats[c]);

					len += sprintf(text+len, "\n");
				}

			}

			// flush now -- DMSG only supports 1024 characters
			DMSG(text);			
			text[len = 0] = 0;

		}
	}

#if GL_VERSION_3_2
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &i);
	static int boundVertexArray;
	len += sprintf(text+len, "bound VAO = %d\n", diffCheck(boundVertexArray, i));

	glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &i);
	static int vertexAttribBuffer0;
	len += sprintf(text+len, "vertex attrib buffer 0 = %d\n", diffCheck(vertexAttribBuffer0, i));

	if( glIsBuffer(vertexAttribBuffer0) )
	{
		int boundArrayBuffer;
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &boundArrayBuffer);

		glBindBuffer(GL_ARRAY_BUFFER, vertexAttribBuffer0);
		float * currentVertexData = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

		const int kNumFloatsPerVertex = 16;  // see batch.cpp for where this comes from
		static float tempPositionData[3*3];
		for_i( 3 )
		{
			tempPositionData[i*3+0] = currentVertexData[i*kNumFloatsPerVertex+0];
			tempPositionData[i*3+1] = currentVertexData[i*kNumFloatsPerVertex+1];
			tempPositionData[i*3+2] = currentVertexData[i*kNumFloatsPerVertex+2];
		}

		static float positionData[3*3];
		len += sprintf(text+len, "\tposition data = (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)...\n", 
			diffCheck(9, positionData, tempPositionData), tempPositionData[1], tempPositionData[2], 
			tempPositionData[3], tempPositionData[4], tempPositionData[5],
			tempPositionData[6], tempPositionData[7], tempPositionData[8]);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, boundArrayBuffer);
	}
#endif

	DMSG(text);  // flush DMSG can only print 1024 characters
	text[len = 0] = 0;



	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &i);
	static int boundElementArrayBuffer;
	len += sprintf(text+len, "bound element array buffer = %d\n", diffCheck(boundElementArrayBuffer, i));

	if( glIsBuffer(boundElementArrayBuffer) )
	{
		unsigned short * currentIndices = (unsigned short *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
		static unsigned short indices[3];
		len += sprintf(text+len, "\t%d, %d, %d...\n", int(diffCheck(3, indices, currentIndices)), int(currentIndices[1]), int(currentIndices[2]));
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

	DMSG(text);  // flush DMSG can only print 1024 characters
	text[len = 0] = 0;


	first = false;
}

void CheckOpenGLErrors(int lineNo, const std::string& file)
{
	GLenum err = glGetError();



	static bool printState = false;
	if( printState ) {
		GL_PrintState();
	}

	static bool writeTexturesToDisk = false;
	if( writeTexturesToDisk ) {
		GEDiagnostics::WriteGLTexturesToDisk();
	}

#if GS_WIN
	if(!wglGetCurrentContext())
	{
		DSTOP((kDan, "No ACtive Context!"));
		return;
	}
#elif GS_MAC
	if( !CGLGetCurrentContext() )
	{
		DSTOP((kDan, "No ACtive Context!"));
		return;
	}

#endif

	while(err)
	{
		std::stringstream out;
		switch(err)
		{
		case GL_NO_ERROR:
			break;
		case GL_INVALID_ENUM:
			out << "An INVALID ENUM has been detected in OpenGL at line ";
			break;
		case GL_INVALID_VALUE:
			out <<  "An INVALID VALUE has been detected in OpenGL at line ";
			break;
		case GL_INVALID_OPERATION:
			out << "An INVALID OPERATION has been detected in OpenGL at line ";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
			out << "An INVALID FRAMEBUFFER OPERATION has been detected in OpenGL at line ";
			break;
		case GL_OUT_OF_MEMORY:
			out << "An OUT OF MEMORY error has been detected in OpenGL at line ";
			break;
		case GL_STACK_OVERFLOW:
			out << "A STACK OVERFLOW has been detected in OpenGL at line ";
			break;
		case GL_STACK_UNDERFLOW:
			out << "A STACK UNDERFLOW has been detected in OpenGL at line "; 
			break;

		default:
			out << "An unknown error has been detected in OpenGL at line "; 
			break;
		}
		if(err!=GL_NO_ERROR)
		{
			out << lineNo << " in " << file;
#if (DaveD || Justin || JLoy || Hernan || Dan || TShin)
			DSTOP((kDan, out.str().c_str()));
#else
			DMSG(out.str().c_str());
#endif
		}

		if(err == GL_OUT_OF_MEMORY)
			throw "Ran out of memory.";

		err = glGetError();
	}

}



int printOglError(char *file, int line)
{
	//
	// Returns 1 if an OpenGL error occurred, 0 otherwise.
	//
	GLenum glErr;
	int    retCode = 0;

	glErr = glGetError();
	while (glErr != GL_NO_ERROR)
	{
		printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
		retCode = 1;
		glErr = glGetError();
	}
	return retCode;
}


void GL_WriteTexturesToDisk()
{
#pragma warning(push)
#pragma warning(disable : 4996)  // sprintf unsafe

	static std::vector<unsigned char> bytes;

	int activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

	int originalBoundTex = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &originalBoundTex);

	int maxColorAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);


	int depthAttachment = -1;
	glGetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthAttachment);

	std::map<GLuint, int> colorAttachments;
	for( int i=0; i<maxColorAttachments; i++ )
	{
		int name;
		glGetFramebufferAttachmentParameterivEXT(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

		if( name != 0 ) colorAttachments[name] = i;
	}

	int maxTextureUnits;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

	std::map<GLuint, int> boundTextures;
	for( int i=0; i<maxTextureUnits; i++ ) {
		int boundTexture;
		glActiveTexture(GL_TEXTURE0+i);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);

		if( boundTexture ) {
			boundTextures[boundTexture] = i;
		}
	}

	glActiveTexture(activeTexture);

	for( int i=1; i<200; i++ ) {  // save textures to disk

		if( glIsTexture(i) ) {
			glBindTexture(GL_TEXTURE_2D, i);

			int width, height, format, components;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);  // Danger, GL_TEXTURE_INTERNAL_FORMAT equals GL_TEXTURE_COMPONENTS
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &components);

			if( format == 1 ) format = GL_R;
			else if( format == 2 ) format = GL_RG;
			else if( format == 3 || format == GL_RGB8 ) format = GL_RGB;
			else if( format == 4 || format == GL_RGBA8 ) format = GL_RGBA;
			else if( format==GL_DEPTH_COMPONENT24 || format==GL_DEPTH_COMPONENT32 ) format = GL_DEPTH_COMPONENT;

			bool floatFormat = false;

			if( format == GL_RGBA32F_ARB )
			{
				floatFormat = true;
			}

			if( components > 4 ) 
			{
				switch( format ) {
				case GL_RGBA:
				case GL_RGBA8:
#if GS_WIN
				case GL_RGBA16F:
				case GL_RGBA32F:
#else
				case GL_RGBA16F_ARB:
				case GL_RGBA32F_ARB:
#endif
					components = 4;
					break;
				case GL_RGB:
#if GS_WIN
				case GL_RGB16F:
				case GL_RGB32F:
#else
				case GL_RGB16F_ARB:
				case GL_RGB32F_ARB:
#endif
					components = 3;
					break;
				case GL_RG:
				case GL_RG16F:
				case GL_RG32F:
					components = 2;
					break;
				case GL_R:
				case GL_DEPTH_COMPONENT16:
				case GL_DEPTH_COMPONENT24:
				case GL_DEPTH_COMPONENT32:
				default:
					components = 1;
					break;
				}

			}

			int size = width * height * components;
			bytes.resize(size);

			if( floatFormat )
			{
				std::vector<float> floatBytes(size);				
				glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, &floatBytes[0]);

				for( size_t i=0; i<floatBytes.size(); i++ )
				{
					bytes[i] = (unsigned char)glm::min(1.0f, glm::max(0.0f, floatBytes[i]*255.0f));
				}
			}
			else
			{
				glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &bytes[0]);
			}

			int err = glGetError();
			if( err != 0 ) {
				err = 0;
			}

			char filename[256];
			int len=0;
#if GS_WIN
			const char * logPath = "c:\\a\\";
#else
			const char * logPath = "/a/";
#endif
			len += sprintf(filename+len, "%stexture%d %dx%d %dBpp", logPath, i, width, height, components);
			if( boundTextures.find(i) != boundTextures.end() ) len += sprintf(filename+len, " GL_TEXTURE%d", boundTextures[i]);
			if( colorAttachments.find(i) != colorAttachments.end() ) len += sprintf(filename+len, " GL_COLOR_ATTACHMENT%d", colorAttachments[i]);
			if( i == depthAttachment ) len += sprintf(filename+len, " GL_DEPTH_ATTACHMENT");
			len += sprintf(filename+len, ".raw");

			std::ofstream out(filename, std::ios_base::binary);
			out.write((const char *)&bytes[0], size);
			out.close();
		}
	}

	glBindTexture(GL_TEXTURE_2D, originalBoundTex);

#pragma warning(pop)
}

unsigned int loadBMP_custom(const char * imagepath)
{
	// Data read from the header of the BMP file
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = NULL;
	fopen_s(&file, imagepath,"rb");
	if (!file) {
		printf("Image could not be opened\n");
		return 0;
	}

	if ( fread(header, 1, 54, file)!=54 ){ // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}

	if ( header[0]!='B' || header[1]!='M' ){
		printf("Not a correct BMP file\n");
		return 0;
	}

	// Read ints from the byte array
	dataPos    = *(int*)&(header[0x0A]);
	imageSize  = *(int*)&(header[0x22]);
	width      = *(int*)&(header[0x12]);
	height     = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize==0)    imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos==0)      dataPos=54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char [imageSize];

	// Read the actual data from the file into the buffer
	fread(data,1,imageSize,file);

	//Everything is in memory now, the file can be closed
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}


#pragma warning(pop)



