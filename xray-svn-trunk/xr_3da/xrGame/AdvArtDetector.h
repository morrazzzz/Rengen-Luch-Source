#pragma once
#include "ArtDetectorBase.h"
#include "../feel_touch.h"

class CUIArtefactDetectorAdv;

class CAdvArtDetector :public CArtDetectorBase,
						 public Feel::Touch
{
	typedef CArtDetectorBase	inherited;

public:
					CAdvArtDetector();
	virtual			~CAdvArtDetector();

	virtual void			feel_touch_new(CObject* O);
	virtual void			feel_touch_delete(CObject* O);
	virtual BOOL			feel_touch_contact(CObject* O);

	virtual void			on_a_hud_attach();
	//virtual void			on_b_hud_detach();

	float					DetectorFeel(CObject* object);
protected:
	virtual void 	UpdateAf();
	virtual void 	CreateUI();
	CUIArtefactDetectorAdv&	ui();

};
