#include "Defs.h"
#include <stdio.h>
#include <stdlib.h>
#include "glew.h"
#include "glut.h"
#include <vector>
#include <map>
#include <fstream>
#include <glm.hpp>

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

