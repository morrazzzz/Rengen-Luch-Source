#include "pch_script.h"
#include "InventoryBox.h"
#include "level.h"
#include "actor.h"
#include "game_object_space.h"

#include "script_callback_ex.h"
#include "script_game_object.h"
#include "inventory_item.h"
#include "xrServer_Objects_ALife.h"
#include "UIGameCustom.h"

CInventoryBox::CInventoryBox()
{
	m_in_use   = false;
	m_can_take = true;
	m_closed   = false;
}

CInventoryBox::~CInventoryBox()
{
}

void CInventoryBox::LoadCfg(LPCSTR section)
{
	READ_IF_EXISTS(pSettings, r_float ,section, "inv_max_weight", 35.f);
}

void CInventoryBox::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent	(P, type);

	switch (type)
	{
	//case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);  VERIFY(itm);
			m_items.push_back	(id);
			itm->H_SetParent	(this);
			itm->setVisible		(FALSE);
			itm->setEnabled		(FALSE);
		}break;

	//case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);  VERIFY(itm);
			xr_vector<u16>::iterator it;
			it = std::find(m_items.begin(),m_items.end(),id); VERIFY(it!=m_items.end());
			m_items.erase		(it);
			itm->H_SetParent	(NULL,!P.r_eof() && P.r_u8());

			if( m_in_use )
			{
				CGameObject* GO		= smart_cast<CGameObject*>(itm);
				Actor()->callback(GameObject::eInvBoxItemTake)( this->lua_game_object(), GO->lua_game_object() );
			}
		}break;
	};
}

void CInventoryBox::UpdateCL()
{
	inherited::UpdateCL();
}

void CInventoryBox::DestroyClientObj()
{
	inherited::DestroyClientObj();
}
BOOL CInventoryBox::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	inherited::SpawnAndImportSOData(data_containing_so);
	
	setVisible				(TRUE);
	setEnabled				(TRUE);
	set_tip_text			("inventory_box_use");
	
	CSE_ALifeInventoryBox*	pSE_box = smart_cast<CSE_ALifeInventoryBox*>(data_containing_so);
	if (pSE_box )
	{
		m_can_take = pSE_box->m_can_take;
		m_closed   = pSE_box->m_closed;
		set_tip_text( pSE_box->m_tip_text.c_str() );
	}

	return					TRUE;
}

void CInventoryBox::RemoveLinksToCLObj(CObject* O)
{
	inherited::RemoveLinksToCLObj(O);
}
#include "inventory_item.h"
void CInventoryBox::AddAvailableItems(TIItemContainer& items_container) const
{
	xr_vector<u16>::const_iterator it = m_items.begin();
	xr_vector<u16>::const_iterator it_e = m_items.end();

	for(;it!=it_e;++it)
	{
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(*it));VERIFY(itm);
		items_container.push_back	(itm);
	}
}

float CInventoryBox::CalcTotalWeight()
{
	float weight = 0;

	xr_vector<u16>::const_iterator it = m_items.begin();
	xr_vector<u16>::const_iterator it_e = m_items.end();

	for (; it != it_e; ++it)
	{
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(*it)); VERIFY(itm);

		weight += itm->Weight();
	}

	m_fTotalWeight = weight;

	return m_fTotalWeight;
}

void CInventoryBox::set_can_take(bool status)
{
	m_can_take = status;
	SE_update_status();
}

void CInventoryBox::set_closed(bool status, LPCSTR reason)
{
	m_closed = status;

	if (reason && xr_strlen(reason))
	{
		set_tip_text(reason);
	}
	else
	{
		set_tip_text("inventory_box_use");
	}
	SE_update_status();
}

void CInventoryBox::SE_update_status()
{
	NET_Packet P;
	CGameObject::u_EventGen(P, GE_INV_BOX_STATUS, ID());
	P.w_u8((m_can_take) ? 1 : 0);
	P.w_u8((m_closed) ? 1 : 0);
	P.w_stringZ(tip_text());
	CGameObject::u_EventSend(P);
}