#pragma once

class CBlender_Visor : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Visor effect"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_Visor();
	virtual ~CBlender_Visor();
};