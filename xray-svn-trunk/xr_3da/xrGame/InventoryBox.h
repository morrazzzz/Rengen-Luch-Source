#pragma once
#include "inventory_space.h"
#include "GameObject.h"

class CInventoryBox :public CGameObject
{
	typedef CGameObject									inherited;

	bool	m_can_take;
	bool	m_closed;

	float												m_fMaxWeight;
	float												m_fTotalWeight;
public:
	xr_vector<u16>										m_items;

						CInventoryBox					();
	virtual				~CInventoryBox					();

	virtual		void	LoadCfg							(LPCSTR section);

	virtual		BOOL	SpawnAndImportSOData			(CSE_Abstract* data_containing_so);
	virtual		void	DestroyClientObj				();
	virtual		void	RemoveLinksToCLObj				(CObject* O);
	
	virtual		void	UpdateCL						();
	
	virtual		void	OnEvent							(NET_Packet& P, u16 type);
	
				void	AddAvailableItems				(TIItemContainer& items_container) const;
	IC			bool	IsEmpty							() const {return m_items.empty();}

	float				GetMaxWeight					() const				{ return m_fMaxWeight; };
	void				SetMaxWeight					(float weight)			{ m_fMaxWeight = weight; };
	IC			void	set_in_use						(bool status) { m_in_use = status; }
	IC			bool	in_use							() const { return m_in_use; }
				
				void	set_can_take					(bool status);
	IC			bool	can_take						() const { return m_can_take; }

	float 				GetTotalWeight					() const { VERIFY(m_fTotalWeight >= 0.f); return m_fTotalWeight; };
	float 				CalcTotalWeight					();
				void	set_closed						(bool status, LPCSTR reason);
	IC			bool	closed							() const { return m_closed; }

	bool				IsSafe;
	LPCSTR				SafeCode;
	LPCSTR				UnlockInfo;
	
	bool	m_in_use;

protected:
	void				SE_update_status();
};

