#pragma once

class CBlender_reflections : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "INTERNAL: reflections"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_reflections();
	virtual ~CBlender_reflections();
};

class CBlender_reflections_MSAA : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "INTERNAL: reflections"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_reflections_MSAA();
	virtual ~CBlender_reflections_MSAA();
	virtual		void		SetDefine(LPCSTR name, LPCSTR definition)
	{
		Name = name;
		Definition = definition;
	}

	LPCSTR Name;
	LPCSTR Definition;
};