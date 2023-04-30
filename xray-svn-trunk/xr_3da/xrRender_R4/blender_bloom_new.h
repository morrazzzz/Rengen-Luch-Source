#pragma once

class CBlender_bloom_new : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "New bloom"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_bloom_new();
	virtual ~CBlender_bloom_new();
};