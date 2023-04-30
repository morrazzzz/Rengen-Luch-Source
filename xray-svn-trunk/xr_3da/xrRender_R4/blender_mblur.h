#pragma once

class CBlender_MBLUR : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Motion Blur"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_MBLUR();
	virtual ~CBlender_MBLUR();
};
