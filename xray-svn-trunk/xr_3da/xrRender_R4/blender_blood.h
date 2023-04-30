#pragma once

class CBlender_Blood: public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Blood"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_Blood();
	virtual ~CBlender_Blood();
};