#include "stdafx.h"
#include "SimpleArtDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "player_hud.h"
#include "artifact.h"

extern BOOL	gameObjectLightShadows_;

CSimpleArtDetector::CSimpleArtDetector(void)
{
}

CSimpleArtDetector::~CSimpleArtDetector(void)
{}

void CSimpleArtDetector::CreateUI()
{
	R_ASSERT(NULL==m_ui);
	m_ui				= xr_new <CUIArtefactDetectorSimple>();
	ui().construct		(this);

	if (for_test){
		Fvector						P;
		P.set(this->Position());
		feel_touch_update(P, fortestrangetocheck);

		
		for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
		{
			CObject* item = *it;
			if (item){
				Fvector dir, to;

				CInventoryItem* invitem = smart_cast<CInventoryItem*>(item);
				item->Center(to);
				float range = dir.sub(to, this->Position()).magnitude();
				Msg("Artefact %s, distance is %f", invitem->object().SectionName().c_str(), range);
			}
		}
		
	}
}

//----------------------------------------------------------------------
void CSimpleArtDetector::feel_touch_new(CObject* O)
{
}

void CSimpleArtDetector::feel_touch_delete(CObject* O)
{
}

BOOL CSimpleArtDetector::feel_touch_contact(CObject *O)
{
	CArtefact* artefact = smart_cast<CArtefact*>(O);

	if (artefact)
	{
		for (u16 id = 0; id < af_types.size(); ++id) 
		{

			if ((xr_strcmp(O->SectionName(), af_types[id]) == 0) || (xr_strcmp(af_types[id], "all") == 0)) 
			{
				return true;
			}
		}
		return false;
	}
	else{ return false; }

}

//----------------------------------------------------------------------

void CSimpleArtDetector::UpdateAf()
{
	LPCSTR closest_art = "null";
	CArtefact* pCurrentAf = nullptr;
	feel_touch_update_delay = feel_touch_update_delay + 1;
	if (feel_touch_update_delay >= 5)//��� �� ������� �������� �� ���� �������
	{
		feel_touch_update_delay = 0;
		Fvector						P;
		P.set(this->Position());
		feel_touch_update(P, foverallrangetocheck);
	}

	float disttoclosestart = 0.0f;

	for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
	{
		float disttoart = DetectorFeel(*it);
		if (disttoart != -10.0f)
		{

			//���� ���������� ����� ��� �� �������(������ ������ �����), �� ���� �� ��������
			if (disttoclosestart <= 0.0f)
			{
				disttoclosestart = disttoart;
				closest_art = closestart;
				pCurrentAf = smart_cast<CArtefact*>(*it);
			}
			//����� ����� ������� ���...
			if (disttoclosestart > disttoart)
			{
				disttoclosestart = disttoart;
				closest_art = closestart;
				pCurrentAf = smart_cast<CArtefact*>(*it);
			}
		}
	}

	//���������� ������� ������� ������������ �������
	if (disttoclosestart == 0.0f)
	{
		return;
	}
	else
	{
		R_ASSERT(pCurrentAf);
		cur_periodperiod = disttoclosestart / (fdetect_radius * pCurrentAf->detect_radius_koef);
	}

	//����� �� ����������� ����. ������
	if (cur_periodperiod < 0.11f)
	{
		cur_periodperiod = 0.11f;
	}

	if (snd_timetime > cur_periodperiod)
	{
		if (!reaction_sound_off)
		{
			if (detect_sndsnd_line || pCurrentAf->custom_detect_sound_string)
			{
				//������� ����������� ������ ������ ����� ��� ��������� �����
				freqq = 1.8f - cur_periodperiod;

				if (freqq < 0.8f)
					freqq = 0.8f;

				if (pCurrentAf->custom_detect_sound_string)
				{
					pCurrentAf->custom_detect_sound.play_at_pos(this, this->Position());
					pCurrentAf->custom_detect_sound.set_frequency(freqq);
				}
				else if (detect_sndsnd_line)
				{
					detect_snd.play_at_pos(this, this->Position());
					detect_snd.set_frequency(freqq);
				}

				snd_timetime = 0;
			}
		}
		ui().Flash(true, cur_periodperiod/10);
	}
	else
		snd_timetime += TimeDelta();
}

//������ ��� �������� ����� � ��������� �������, � �� � ��� ������ ���

float CSimpleArtDetector::DetectorFeel(CObject* item)
{
	Fvector dir, to;

	item->Center(to);
	float range = dir.sub(to, Position()).magnitude();
	CInventoryItem* invitem = smart_cast<CInventoryItem*>(item);
	CArtefact* artefact = smart_cast<CArtefact*>(item);

	float gogogo = fdetect_radius *  artefact->detect_radius_koef;
	if (range<gogogo)
	{
		//Msg("feeldetector  %s  %s", invitem->object().SectionNameStr(), invitem->object().SectionName().c_str());
		closestart = invitem->object().SectionNameStr();
		return range;
	}
	return -10.0f;
}

//---------------UI(��������)------------------
CUIArtefactDetectorSimple&  CSimpleArtDetector::ui()
{
	return *((CUIArtefactDetectorSimple*)m_ui);
}


void CUIArtefactDetectorSimple::construct(CSimpleArtDetector* p)
{
	m_parent							= p;
	m_flash_bone						= BI_NONE;
	m_on_off_bone						= BI_NONE;
	Flash								(false, 0.0f);
}

CUIArtefactDetectorSimple::~CUIArtefactDetectorSimple()
{
	//Svet_Dosviduli					(m_flash_light);
	//Svet_Dosviduli					(m_on_off_light);
}

// ������� ������ ������ ����� � ������, ������� �������� ����� � ���� ������ � ������.
void CUIArtefactDetectorSimple::SetBoneVisible(IKinematics* model, CSimpleArtDetector* parent, LPCSTR bonename, bool visible, bool someshit)
{
	if(!parent->HudItemData())	return;

	u16 bone						= BI_NONE;

	IKinematics* modeltouse = model;
	R_ASSERT			(modeltouse);
	R_ASSERT			(bone==BI_NONE);
	bone = model->LL_BoneID	(bonename);
	if(visible == true)
	{
		model->LL_SetBoneVisible(bone, TRUE, TRUE);
	}else
	{
		model->LL_SetBoneVisible(bone, FALSE, TRUE);
	}
}

// ������� ��������� �������
//void CUIArtefactDetectorSimple::ZvukovojSignal(HUD_SOUND_ITEM sound, float fvector1, float fvector2, float fvector3, CSimpleDetector* parent, bool bool1, bool bool2)
//{
//}

// ������� ����� ������� �����
//void CUIArtefactDetectorSimple::ZvukovojSignalChastota(HUD_SOUND_ITEM sound, float freq)
//{
//}

// ������� �������� �����
ref_light CUIArtefactDetectorSimple::Svet_Sozdat()
{
	ref_light lightvar;
	//R_ASSERT						(!light);
	lightvar					= ::Render->light_create();
	
	lightvar->set_type			(IRender_Light::POINT);
	lightvar->set_use_static_occ(false);
	
	return (lightvar);
}

// ������� ���� �����
void CUIArtefactDetectorSimple::Svet_Ten(ref_light light, bool danet)
{
	if(danet == true)
	{
		light->set_shadow(gameObjectLightShadows_ == TRUE ? true : false);
	}else
	{
		light->set_shadow		(false);
	}
}

// ������� ������� �����
void CUIArtefactDetectorSimple::Svet_Dalnost(ref_light light, float range)
{
	light->set_range		(range);
}

// ������� ���-��� �����
void CUIArtefactDetectorSimple::Svet_HUDmode(ref_light light, bool danet)
{
	if(danet == true)
	{
		light->set_hud_mode	(true);
	}else
	{
		light->set_hud_mode	(false);
	}
}

// ������� ���/���� ����

void CUIArtefactDetectorSimple::Svet_VklVykl(ref_light light, bool danet)
{
	light->set_active(danet);
}

// ��������� get ���/���� ����
bool CUIArtefactDetectorSimple::Svet_Get_VklVykl(ref_light light)
{
	bool danet = light->get_active();
	return (danet);
}

// ������� ���� �����
void CUIArtefactDetectorSimple::Svet_Cvet(ref_light light, float alfa, float k, float z, float s, bool useU32, u32 clr)
{

	Fcolor					fclr;
	if(useU32 == true)
	{
		fclr.set				(clr);
	}else
	{
		fclr.a				=alfa;
		fclr.r				=k;
		fclr.g				=z;
		fclr.b				=s;
	}
	light->set_color(fclr);
}

// ������� ������ ������� �����
void CUIArtefactDetectorSimple::Svet_Position(ref_light light, Fvector pos)
{
	light->set_position(pos);
}

// ������� ��������� ����
void CUIArtefactDetectorSimple::Svet_Dosviduli(ref_light light)
{
	light.destroy();
}

#pragma todo("���� ��������� �� � ��� ����� + �������� ������� ��� �� ������� ��� �������� � ������������� ����. �������� ��������� � ������� ����, ������� ������������ ��� �������� ��� ������ �����| ���� �������� ����")
#pragma todo("Lights spawn in incorrect points, also causes fps drop on static renderer. might be fire poits, that are used to position the light| disabled light til fixed")
void CUIArtefactDetectorSimple::Flash(bool bOn, float fRelPower)
{
	if(!m_parent->HudItemData())	return;


	IKinematics* K		= m_parent->HudItemData()->m_model;
	R_ASSERT			(K);
	LPCSTR boner = "light_bone_2";
	if(bOn)
	{
		SetBoneVisible(K,m_parent,boner,true,true);
		m_turn_off_flash_time = EngineTimeU()+iFloor(fRelPower*1000.0f);
	}else
	{
		SetBoneVisible(K,m_parent,boner,false,true);
		m_turn_off_flash_time	= 0;
	}
//if (bOn != Svet_Get_VklVykl(m_flash_light)){
//#pragma todo("���� ��������� �� � ��� �����|Lights spawn in incorrect points")
//		Svet_VklVykl(m_flash_light, bOn);
	//}
}

void CUIArtefactDetectorSimple::setup_internals()
{
	//m_flash_light = Svet_Sozdat();
	//Svet_Ten						(m_flash_light, true);
	//Svet_Dalnost					(m_flash_light, (0.1));
	//Svet_HUDmode					(m_flash_light, true);
	
	//m_on_off_light = Svet_Sozdat();
	//Svet_Ten						(m_on_off_light, false);
	//Svet_Dalnost					(m_on_off_light, (0.1));
	//Svet_HUDmode					(m_on_off_light, true);

	IKinematics* K					= m_parent->HudItemData()->m_model;
	R_ASSERT						(K);

	R_ASSERT						(m_flash_bone==BI_NONE);
	
	m_flash_bone					= K->LL_BoneID	("light_bone_2");
	LPCSTR boner = "light_bone_2";
	SetBoneVisible(K,m_parent,boner,false,true);
	boner = "light_bone_1";
	SetBoneVisible(K,m_parent,boner,true,true);

	//m_pOnOfLAnim					= LALib.FindItem("det_on_off");
	//m_pFlashLAnim					= LALib.FindItem("det_flash");
}

void CUIArtefactDetectorSimple::update()
{
	inherited::update					();

	if(m_parent->HudItemData())
	{
		if(m_flash_bone==BI_NONE)
			setup_internals();

		if(m_turn_off_flash_time && m_turn_off_flash_time<EngineTimeU())
			Flash (false, 0.0f);

		//if (Svet_Get_VklVykl(m_flash_light))
		//	Svet_Position(m_flash_light, get_bone_position(m_parent, "light_bone_2"));
		//Svet_Position(m_flash_light, m_parent->Position());

		//Svet_Position(m_on_off_light, get_bone_position(m_parent, "light_bone_1"));
		//Svet_Position(m_on_off_light, m_parent->Position());
		//if(!Svet_Get_VklVykl(m_on_off_light))

		//	Svet_VklVykl(m_on_off_light, true);

		//if (!Svet_Get_VklVykl(m_flash_light))

		//	Svet_VklVykl(m_flash_light, true);

//		u32 clr					= m_pOnOfLAnim->CalculateRGB(EngineTime(),frame);
		/*
		firedeps		fd;
		m_parent->HudItemData()->setup_firedeps(fd);
		if (m_flash_light->get_active())
			m_flash_light->set_position(fd.vLastFP);

		m_on_off_light->set_position(fd.vLastFP2);
		if (!m_on_off_light->get_active())
			m_on_off_light->set_active(true);

		//int frame = 0;
		Svet_Cvet(m_on_off_light, 0.0, 0.0, 0.0, 0.0, true, 16500000);
		//u32 clr = m_pOnOfLAnim->CalculateRGB(EngineTime(), frame);
		//Fcolor					fclr;
		//fclr.set(clr);
		//m_on_off_light->set_color(fclr);
		*/
	}
}