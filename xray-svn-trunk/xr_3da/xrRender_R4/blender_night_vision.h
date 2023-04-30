#pragma once

class CBlender_Night_Vision : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Night vision effect"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_Night_Vision();
	virtual ~CBlender_Night_Vision();
};
