#include "pch_script.h"
#include "UIGameCustom.h"
#include "level.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "ui/UIMultiTextStatic.h"
#include "object_broker.h"

#include "InventoryOwner.h"

#include "ui/UIPdaWnd.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIInventoryWnd.h"
#include "ui/UIStashWnd.h"
#include "ui/UIWeaponSelectWnd.h"

#include "actor.h"
#include "inventory.h"
#include "game_cl_base.h"

bool predicate_sort_stat(const SDrawStaticStruct* s1, const SDrawStaticStruct* s2) 
{
	return ( s1->IsActual() > s2->IsActual() );
}

struct predicate_find_stat 
{
	LPCSTR	m_id;
	predicate_find_stat(LPCSTR id):m_id(id)	{}
	bool	operator() (SDrawStaticStruct* s) 
	{
		return ( s->m_name==m_id );
	}
};

CUIGameCustom::CUIGameCustom()
	:m_pgameCaptions(NULL), m_msgs_xml(NULL), m_actor_menu_item(NULL), m_window(NULL), m_InventoryMenu(NULL), m_PdaMenu(NULL), m_UIStashWnd(NULL), UIMainIngameWnd(NULL), m_pMessagesWnd(NULL), m_WeatherEditor(NULL), m_WeaponSelect(NULL)
{
	crosshairBlocks_.zero();
	gameIndicatorsBlocks_.zero();
	ginputBlocks_.zero();
	keyboardBlocks_.zero();
	gameHUDEffectsBlocks_.zero();
}
bool g_b_ClearGameCaptions = false;

CUIGameCustom::~CUIGameCustom()
{
	delete_data				(m_custom_statics);
	g_b_ClearGameCaptions	= false;
}


void CUIGameCustom::OnFrame() 
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	CDialogHolder::OnFrame();

	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();
	for(;it!=it_e;++it)
		(*it)->Update();

	std::sort(	it, it_e, predicate_sort_stat );
	
	while(!m_custom_statics.empty() && !m_custom_statics.back()->IsActual())
	{
		delete_data					(m_custom_statics.back());
		m_custom_statics.pop_back	();
	}
	
	if(g_b_ClearGameCaptions)
	{
		delete_data				(m_custom_statics);
		g_b_ClearGameCaptions	= false;
	}
	m_window->Update();

	//update windows
	if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW) )
		UIMainIngameWnd->Update	();

	m_pMessagesWnd->Update();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_UIGameCustom_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

void CUIGameCustom::Render()
{
	GameCaptions()->Draw();

	st_vec_it it = m_custom_statics.begin();
	st_vec_it it_e = m_custom_statics.end();

	for (; it != it_e; ++it)
		if (!(*it)->onTop)
			(*it)->Draw();

	m_window->Draw();

	CEntity* pEntity = smart_cast<CEntity*>(Level().CurrentEntity());
	if (pEntity)
	{
		CActor* pActor = smart_cast<CActor*>(pEntity);
	        if (pActor && pActor->HUDview() && pActor->g_Alive() &&
	            psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2))
	        {

			CInventory& inventory = pActor->inventory();
			for (auto slot_idx = inventory.FirstSlot(); slot_idx <= inventory.LastSlot(); ++slot_idx)
			{
				CInventoryItem* item = inventory.ItemFromSlot(slot_idx);
				if (item && item->render_item_ui_query())
				{
					item->render_item_ui();
				}
			}
		}

		if( GameIndicatorsShown() && psHUD_Flags.is(HUD_DRAW) )
		{
			UIMainIngameWnd->Draw();
			m_pMessagesWnd->Draw();
		} else {  //hack - draw messagess wnd in scope mode
			if (!m_PdaMenu->GetVisible())
				m_pMessagesWnd->Draw();
		}	
	}
	else
		m_pMessagesWnd->Draw();

	DoRenderDialogs();

	it = m_custom_statics.begin();
	it_e = m_custom_statics.end();

	for (; it != it_e; ++it)
		if ((*it)->onTop)
			(*it)->Draw();

}

void CUIGameCustom::AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color)
{
	GameCaptions()->addCustomMessage(id,x,y,font_size,pFont,(CGameFont::EAligment)alignment,color,"");
}

void CUIGameCustom::AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color, float flicker )
{
	AddCustomMessage(id,x,y,font_size, pFont, alignment, color);
	GameCaptions()->customizeMessage(id, CUITextBanner::tbsFlicker)->fPeriod = flicker;
}

void CUIGameCustom::CustomMessageOut(LPCSTR id, LPCSTR msg, u32 color)
{
	GameCaptions()->setCaption(id,msg,color,true);
}

void CUIGameCustom::RemoveCustomMessage		(LPCSTR id)
{
	GameCaptions()->removeCustomMessage(id);
}


SDrawStaticStruct* CUIGameCustom::AddCustomStatic(LPCSTR id, bool bSingleInstance)
{
	if (bSingleInstance)
	{
		st_vec::iterator it = std::find_if(m_custom_statics.begin(), m_custom_statics.end(), predicate_find_stat(id));
		
		if (it != m_custom_statics.end())
			return (*it);
	}
	
	CUIXmlInit xml_init;
	SDrawStaticStruct* sss;
	m_custom_statics.push_back(xr_new <SDrawStaticStruct>());
	sss = m_custom_statics.back();

	sss->m_static					= xr_new <CUIStatic>();
	sss->m_name						= id;

	xml_init.InitStatic				(*m_msgs_xml, id, 0, sss->m_static);

	float ttl = m_msgs_xml->ReadAttribFlt(id, 0, "ttl", -1);
	bool on_top = !!m_msgs_xml->ReadAttribInt(id, 0, "on_top", 0);

	sss->onTop = on_top;

	if(ttl > 0.0f)
		sss->m_endTime = EngineTime() + ttl;

	return sss;
}
 
SDrawStaticStruct* CUIGameCustom::GetCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id));
	if(it!=m_custom_statics.end())
		return (*it);

	return NULL;
}

void CUIGameCustom::RemoveCustomStatic(LPCSTR id)
{
	st_vec::iterator it = std::find_if(m_custom_statics.begin(),m_custom_statics.end(), predicate_find_stat(id) );
	if(it!=m_custom_statics.end())
	{
			delete_data				(*it);
		m_custom_statics.erase	(it);
	}
}

#include "ui/UIGameTutorial.h"

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

void CUIGameCustom::reset_ui()
{
	if(g_tutorial2)
	{ 
		g_tutorial2->Destroy	();
		xr_delete				(g_tutorial2);
	}

	if(g_tutorial)
	{
		g_tutorial->Destroy	();
		xr_delete(g_tutorial);
	}
}

void CUIGameCustom::UnLoad()
{
	xr_delete					(m_pgameCaptions);
	xr_delete					(m_msgs_xml);
	xr_delete					(m_actor_menu_item);
	xr_delete					(m_window);
	xr_delete					(UIMainIngameWnd);
	xr_delete					(m_pMessagesWnd);
	xr_delete					(m_InventoryMenu);
	xr_delete					(m_PdaMenu);
	xr_delete					(m_UIStashWnd);
	xr_delete					(m_WeatherEditor);
	xr_delete					(m_WeaponSelect);
}

void CUIGameCustom::Load()
{
	if(g_pGameLevel)
	{
		R_ASSERT				(NULL==m_pgameCaptions);
		m_pgameCaptions				= xr_new <CUICaption>();

		R_ASSERT				(NULL==m_msgs_xml);
		m_msgs_xml				= xr_new <CUIXml>();
		m_msgs_xml->Load		(CONFIG_PATH, UI_PATH, "ui_custom_msgs.xml");

		R_ASSERT				(NULL==m_actor_menu_item);
		m_actor_menu_item		= xr_new <CUIXml>();
		m_actor_menu_item->Load	(CONFIG_PATH, UI_PATH, "actor_menu_item.xml");

		R_ASSERT				(NULL==m_PdaMenu);
		m_PdaMenu				= xr_new <CUIPdaWnd>			();
		
		R_ASSERT				(NULL==m_window);
		m_window				= xr_new <CUIWindow>			();

		R_ASSERT				(NULL==UIMainIngameWnd);
		UIMainIngameWnd			= xr_new <CUIMainIngameWnd>	();
		UIMainIngameWnd->Init	();

		R_ASSERT				(NULL==m_InventoryMenu);
		m_InventoryMenu			= xr_new <CUIInventoryWnd>	();

		R_ASSERT				(NULL==m_UIStashWnd);
		m_UIStashWnd			= xr_new <CUIStashWnd>		();

		R_ASSERT				(NULL==m_pMessagesWnd);
		m_pMessagesWnd			= xr_new <CUIMessagesWindow>();

		R_ASSERT				(NULL==m_WeatherEditor);
		m_WeatherEditor			= xr_new <CUIWeatherEditor>();

		R_ASSERT				(NULL==m_WeaponSelect);
		m_WeaponSelect			= xr_new <CUIWeaponSelectWnd>();
		
		Init					(0);
		Init					(1);
		Init					(2);
	}
}

void CUIGameCustom::OnConnected()
{
	if(g_pGameLevel)
	{
		if(!UIMainIngameWnd)
			Load();

		UIMainIngameWnd->OnConnected();
	}
}

void CUIGameCustom::CommonMessageOut(LPCSTR text)
{
	m_pMessagesWnd->AddLogMessage(text);
}

SDrawStaticStruct::SDrawStaticStruct	()
{
	m_static	= NULL;
	m_endTime	= -1.0f;
	onTop		= false;
}

void SDrawStaticStruct::destroy()
{
	delete_data(m_static);
}

bool SDrawStaticStruct::IsActual() const
{
	if(m_endTime<0)			return true;
	return (EngineTime() < m_endTime);
}

void SDrawStaticStruct::SetText(LPCSTR text)
{
	m_static->Show(text!=NULL);
	if(text)
	{
		m_static->TextItemControl()->SetTextST(text);
		m_static->ResetColorAnimation();
	}
}


void SDrawStaticStruct::Draw()
{
	if(m_static->IsShown())
		m_static->Draw();
}

void SDrawStaticStruct::Update()
{
	if(IsActual() && m_static->IsShown())	
		m_static->Update();
}

bool CUIGameCustom::ShowActorMenu()
{
	if (m_InventoryMenu->IsShown())
	{
		m_InventoryMenu->HideDialog();
	}
	else
	{
		HidePdaMenu();

		m_InventoryMenu->ShowDialog(true);
	}
	return true;
}

void CUIGameCustom::HideActorMenu()
{
	if (m_InventoryMenu->IsShown())
	{
		m_InventoryMenu->HideDialog();
	}
}

void CUIGameCustom::HideMessagesWindow()
{
	if (m_pMessagesWnd->IsShown())
		m_pMessagesWnd->Show(false);
}

void CUIGameCustom::ShowMessagesWindow()
{
	if (!m_pMessagesWnd->IsShown())
		m_pMessagesWnd->Show(true);
}

bool CUIGameCustom::ShowPdaMenu()
{
	HideActorMenu();
	m_PdaMenu->ShowDialog(true);
	return true;
}

void CUIGameCustom::HidePdaMenu()
{
	if (m_PdaMenu->IsShown())
	{
		m_PdaMenu->HideDialog();
	}
}

void CUIGameCustom::UpdatePda()
{
	PdaMenu().Update();
}

void CUIGameCustom::update_fake_indicators(u8 type, float power)
{
	R_ASSERT(false);
	//UIMainIngameWnd->get_hud_states()->FakeUpdateIndicatorType(type, power);
}

void CUIGameCustom::enable_fake_indicators(bool enable)
{
	R_ASSERT(false);
	//UIMainIngameWnd->get_hud_states()->EnableFakeIndicators(enable);
}