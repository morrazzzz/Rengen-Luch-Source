#pragma once

class CBlender_Rain_Drops : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Rain drops effect"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_Rain_Drops();
	virtual ~CBlender_Rain_Drops();
};
