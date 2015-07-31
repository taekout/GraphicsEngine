#include "Shader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include "glew.h"
#include "Defs.h"


Shader::Shader(void)
	: fShaderIndex(-1)
{
	//EShaderKind::eShaderBasic
	const unsigned int kOutColorID = 0;
	const unsigned int kInPosID= 0;
	const unsigned int kInNormals = 1;
	const unsigned int kInUV = 2;

	setShaders(Shader::EShaderKind::eShaderBasic, "./GLSL/white.vert", "./GLSL/white.frag");

	glBindFragDataLocation(GetProgram(), kOutColorID, "outColor");
	glBindAttribLocation(GetProgram(), kInPosID, "inPositions");
	glBindAttribLocation(GetProgram(), kInNormals, "inNormals");
	glBindAttribLocation(GetProgram(), kInUV, "inUV");

	printOpenGLError();

	LinkShaders();
	UseProgram(eShaderNothing);
	//EShaderKind::eShaderTexture
	
	
	setShaders(Shader::EShaderKind::eShaderTexture, "./GLSL/texture.vert", "./GLSL/texture.frag");
	glBindFragDataLocation(GetProgram(), kOutColorID, "outColor");
	glBindAttribLocation(GetProgram(), kInPosID, "inPositions");
	glBindAttribLocation(GetProgram(), kInNormals, "inNormals");
	glBindAttribLocation(GetProgram(), kInUV, "inUV");
	
	printOpenGLError();
	
	LinkShaders();
	UseProgram(eShaderNothing);
}


Shader::~Shader(void)
{
}

char * Shader::textFileRead(char  *fn) {
	FILE *fp = NULL;
	char *content = NULL;

	int count=0;

	if (fn != NULL) {
		fopen_s(&fp, fn,"rt");

		if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
/*	fstream fp;
	char * content = NULL;
	int count = 0;

	if(fn != NULL)
	{
		fp.open(fn, fstream::in);
		if( fp.is_open() ) 
		{
			fp.seekg(0,	ios::end);
			count = fp.tellg();
			fp.seekg(0, ios::beg);

			if( count > 0 )
			{
				content = new char[count];
				fp.read(content, count );
				content[count] = '\0';
			}
			fp.close();
		}
	}
	return content;*/
}

std::string *Shader::ShaderFileRead(std::string filename, std::string shaderKind)
{
	//std::string vertFilename;
	//std::string fragFilename;

	std::string data = textFileRead((char *)filename.c_str());
	std::string *final = textProcess(data);
	//if(shaderKind == "vertex shader") {
	//	vertFilename = filename;
	//}
	//else if(shaderKind == "fragment shader") {
	//	fragFilename = filename;
	//}

	// Register the shader files.
	return final;
}

std::string *Shader::textProcess(std::string stringData)
{
	std::string *final = new std::string;
	size_t pos, filenamePos, filenamePos2;
	while(stringData.find("#include", 0) != std::string::npos)
	{	// Parse #include and get the file name
		pos = stringData.find("#include", 0);
		filenamePos = stringData.find("\"", pos);
		filenamePos ++;
		filenamePos2 = stringData.find("\"", filenamePos);
		std::string filename = stringData.substr(filenamePos, filenamePos2-filenamePos);
		*final += stringData.substr(0, pos);
		stringData.erase(0, filenamePos2+1);
		// Add the corresponding file content.
		std::string contentToAdd = textFileRead((char*)filename.c_str());
		stringData.insert(0, contentToAdd);
	}
	// There is still the remaining part after having found the tokens. So add it.
	*final += stringData;

	// Now step1(find #include and insert the file content after removing the include syntax) is done.
	// Step 2 - find @# and remove @ token.
	while(final ->find("@#", 0) != std::string::npos) {
		size_t pos = final ->find("@#", 0);
		final ->erase(pos, 1);
	}
	return final;
}

int Shader::textFileWrite(char *fn, char *s) {
	FILE *fp = NULL;
	int status = 0;

	if (fn != NULL) {
		fopen_s(&fp, fn,"w");

		if (fp != NULL) {
			
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
/*	fstream fp;
	int status = 0;
	if(fn != NULL)
	{
		fp.open(fn, fstream::out);
		if(fp.is_open())
		{
			status = !fp.write(s, strlen(s)).bad();
		}
	}
	return(status);*/
}

void Shader::printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);	
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
        free(infoLog);

		if(infologLength == 1)
			printf("shader fine.\n");
    }
}

void Shader::printProgramInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
        free(infoLog);

		if(infologLength == 1)
			printf("shader fine.\n");
    }
	
}

void Shader::ShaderFileChangeWatcher(void)
{
	ShaderData &sd = fShaderData[fShaderIndex];

	struct _stat fileinfo;
	// vertex shader file change detection
	if(sd.vertFilename.empty() == true) return;
	if(_stat(sd.vertFilename.c_str(), &fileinfo) != -1) {
		vertTimeStamp = fileinfo.st_mtime;
		if(vertTimeStamp.IsChanged()) {
			;//setShaders(EShaderKind::eShaderTexture, (char *)sd.vertFilename.c_str(), (char *)sd.fragFilename.c_str());
		}
	}
	// fragment shader file change detection
	if(sd.fragFilename.empty() == true) return;
	if(_stat(sd.fragFilename.c_str(), &fileinfo) != -1) {
		fragTimeStamp = fileinfo.st_mtime;
		if(fragTimeStamp.IsChanged())
			;//setShaders(EShaderKind::eShaderTexture, (char *)sd.vertFilename.c_str(), (char *)sd.fragFilename.c_str());
	}
}

void Shader::setShaders(EShaderKind kind, char *vertShader, char * fragShader) {

	if(fShaderIndex != -1) {
		ShaderData & shaderData = fShaderData[fShaderIndex];
		if(shaderData.fProgramID != 0) {
			//glDetachShader(shaderData.fProgramID, shaderData.fVertShaderID);
			//glDetachShader(shaderData.fProgramID, shaderData.fFragShaderID);
			//glDeleteShader(shaderData.fVertShaderID);
			//glDeleteShader(shaderData.fFragShaderID);
			//glDeleteProgram(shaderData.fProgramID);
		}

	}
	
	GLuint programID = 0;
	GLuint vertShaderID = 0;
	GLuint fragShaderID = 0;
	char *vs = NULL,*fs = NULL,*fs2 = NULL;

	vertShaderID = glCreateShader(GL_VERTEX_SHADER);
	fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	std::string *vertData = ShaderFileRead(vertShader, "vertex shader");
	vs = new char[vertData ->length() + 1];
	strcpy_s( vs, vertData ->length() + 1, vertData ->c_str() );
	std::string *fragData = ShaderFileRead(fragShader, "fragment shader");
	fs = new char[fragData ->length() + 1];
	strcpy_s( fs, fragData ->length() + 1, fragData ->c_str() );

	const char * vv = vs;
	const char * ff = fs;
	glShaderSource(vertShaderID, 1, &vv,NULL);
	glShaderSource(fragShaderID, 1, &ff,NULL);
	free(vs);free(fs);

	glCompileShader(vertShaderID);
	glCompileShader(fragShaderID);
	printShaderInfoLog(vertShaderID);
	printShaderInfoLog(fragShaderID);
	programID = glCreateProgram();
	glAttachShader(programID, vertShaderID);
	glAttachShader(programID, fragShaderID);

	ShaderData sd(kind, vertShaderID, fragShaderID, programID, std::string(vertShader), std::string(fragShader));
	fShaderData.push_back(sd);
	fShaderIndex = fShaderData.size() - 1;

	delete vertData, fragData;
}

void Shader::LinkShaders()
{
	ShaderData & sd = fShaderData[fShaderIndex];
	glLinkProgram(sd.fProgramID);
	printProgramInfoLog(sd.fProgramID);

	//glUseProgram(sd.fProgramID);
	//printf("Use a new program.\n");
}

void Shader::UseProgram(EShaderKind kind)
{
	if(kind == eShaderNothing) {
		glUseProgram(0);
		return;
	}
	
	fShaderIndex = kind;
	glUseProgram(fShaderData[fShaderIndex].fProgramID);
}

void Shader::UpdateUniform4fv(char *varName, float data1, float data2, float data3, float data4)
{
	ShaderData &sd = fShaderData[fShaderIndex];

	GLint loc;
	loc = glGetUniformLocation(sd.fProgramID, varName);
	if(loc == -1)
		return;
	GLfloat dataToUpdate[4] = {data1, data2, data3, data4};
	glUniform4fv(loc, 1, dataToUpdate);
}

void Shader::UpdateUniform1f(char *varName, float data)
{
	ShaderData &sd = fShaderData[fShaderIndex];

	GLint loc;
	loc = glGetUniformLocation(sd.fProgramID, varName);
	if(loc == -1)
		return;
	glUniform1f(loc, data);
}
void Shader::UpdateUniform3fv(char *varName, float data1, float data2, float data3)
{
	ShaderData &sd = fShaderData[fShaderIndex];

	GLint loc;
	loc = glGetUniformLocation(sd.fProgramID, varName);
	if(loc == -1)
		return;
	GLfloat dataToUpdate[3] = {data1, data2, data3};
	glUniform3fv(loc, 1, dataToUpdate);
}

void Shader::UpdateUniformMat4(char *varName, float * data)
{
	ShaderData &sd = fShaderData[fShaderIndex];

	GLint loc;
	loc = glGetUniformLocation(sd.fProgramID, varName);
	if(loc == -1)
		return;
	glUniformMatrix4fv(loc, 1, false, data);
}
