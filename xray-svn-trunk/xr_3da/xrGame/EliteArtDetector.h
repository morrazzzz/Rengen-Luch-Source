#pragma once
#include "ArtDetectorBase.h"
#include "../feel_touch.h"
#include "DetectList.h"

class CZoneList;
class CUIArtefactDetectorElite;

class CEliteArtDetector : public CArtDetectorBase,
						public Feel::Touch
{
	typedef CArtDetectorBase	inherited;
public:
	CEliteArtDetector();
	virtual			~CEliteArtDetector();
	virtual void	render_item_3d_ui();
	virtual bool	render_item_3d_ui_query();
	virtual LPCSTR	ui_xml_tag() const { return "elite"; }


	virtual void			feel_touch_new(CObject* O);
	virtual void			feel_touch_delete(CObject* O);
	virtual BOOL			feel_touch_contact(CObject* O);
	//virtual BOOL			feel_touch_on_contact		(CObject* O);

	float					DetectorFeel(CObject* object);
protected:
	virtual void 	UpdateAf();
	virtual void 	CreateUI();
	CUIArtefactDetectorElite& ui();
};

class CScientificArtDetector :public CEliteArtDetector
{
	typedef CEliteArtDetector	inherited;
public:
	CScientificArtDetector();
	virtual			~CScientificArtDetector();
	virtual void 	LoadCfg(LPCSTR section);
	virtual void 	BeforeDetachFromParent(bool just_before_destroy);
	virtual void 	ScheduledUpdate(u32 dt);

	virtual LPCSTR	ui_xml_tag() const { return "scientific"; }
protected:
	virtual void	UpdateWork();
	CZoneList		m_zones;
};


