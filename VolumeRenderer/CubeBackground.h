#pragma once
#include <vector>
#include <array>

class IBackground
{
public:
	IBackground() {}
	~IBackground() {}

	virtual void SetTexIDs(unsigned int) = 0;
	virtual void SetCubeBackgroundTex(const std::array<unsigned int, 6> & texIDs) = 0;

	virtual unsigned int GetNumTex() = 0;
	virtual unsigned int GetTex(unsigned int index) = 0;
};

class CubeBackground : public IBackground
{
public:
	CubeBackground() {}
	~CubeBackground() {}

	virtual void SetTexIDs(unsigned int);
	virtual void SetCubeBackgroundTex(const std::array<unsigned int, 6> & texIDs);

	virtual unsigned int GetNumTex();
	virtual unsigned int GetTex(unsigned int index);

protected:
	std::array<unsigned int, 6> fTexIDs;
};

