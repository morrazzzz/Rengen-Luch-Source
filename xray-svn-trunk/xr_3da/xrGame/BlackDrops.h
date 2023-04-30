///////////////////////////////////////////////////////////////
// BlackDrops.h
// BlackDrops - черные капли
///////////////////////////////////////////////////////////////

#pragma once
#include "artifact.h"

class CBlackDrops : public CArtefact 
{
private:
	typedef CArtefact inherited;
public:
	CBlackDrops(void);
	virtual ~CBlackDrops(void);

	virtual void LoadCfg				(LPCSTR section);

protected:
};