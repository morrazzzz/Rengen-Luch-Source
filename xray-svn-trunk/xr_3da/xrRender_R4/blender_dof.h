#pragma once

class CBlender_DOF : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Depth of Field"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_DOF();
	virtual ~CBlender_DOF();
};
