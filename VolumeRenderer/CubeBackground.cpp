#include "CubeBackground.h"
#include "glew.h"

void CubeBackground::SetTexIDs(unsigned int ID)
{
}

void CubeBackground::SetCubeBackgroundTex(const std::array<unsigned int, 6> & texIDs)
{
	fTexIDs = texIDs;
}

unsigned int CubeBackground::GetNumTex()
{
	return 6;
}

unsigned int CubeBackground::GetTex(unsigned int index)
{
	if (index >= fTexIDs.size())
		throw "out of index access to tex unit.";
	return fTexIDs[index];
}

