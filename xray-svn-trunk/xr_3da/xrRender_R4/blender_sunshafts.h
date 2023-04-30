#pragma once

class CBlender_ss : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Screen Space SS"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_ss();
	virtual ~CBlender_ss();
};