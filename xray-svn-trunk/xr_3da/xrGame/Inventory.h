#pragma once
#include "inventory_item.h"
#include "ArtDetectorBase.h"

class CInventory;
class CInventoryItem;
class CHudItem;
class CInventoryOwner;

class CInventorySlot
{									
public:
							CInventorySlot		();
	virtual					~CInventorySlot		();

	bool					CanBeActivated		() const;
	bool					IsBlocked			() const;

	PIItem					m_pIItem;
	bool					m_bPersistent;
	bool					canCastHud_;
	s8						m_blockCounter; 
};

typedef xr_vector<CInventorySlot> TISlotArr;



class CInventory
{

public:
							CInventory			();
	virtual					~CInventory			();

	virtual void			LoadCfg				(LPCSTR section);

	void					Clear				();

	void					TakeItem			(PIItem item_to_take, bool bNotActivate, bool strict_placement, bool duringSpawn);
	bool					DropItem			(PIItem item_to_drop);
	bool					Eat					(PIItem ItemToEat);

	void					RepackBelt			(PIItem pIItem);
	void					RepackRuck			(PIItem pIItem);

	bool					MoveToSlot			(TSlotId slot, PIItem pIItem, bool bNotActivate = false);
	bool					MoveToBelt			(PIItem pIItem);
	bool					MoveToArtBelt		(PIItem pIItem);
	bool					MoveToRuck			(PIItem pIItem);

	bool 					IsInSlot			(PIItem pIItem) const;
	bool 					IsInBelt			(PIItem pIItem) const;
	bool 					IsInArtBelt			(PIItem pIItem) const;
	bool 					IsInRuck			(PIItem pIItem) const;

	//��������� ����� �� ��������� ���� � ����
	bool 					CanPutInSlot		(PIItem pIItem, TSlotId slot) const;
	//��������� ����� �� ��������� ���� �� ���� ��������
	bool 					CanPutInBelt		(PIItem pIItem, bool forceRoomCheck = true);
	//��������� ����� �� ��������� ���� �� ���� ����������
	bool 					CanPutInArtBelt		(PIItem pIItem);
	//��������� ����� �� ��������� ���� � ������
	bool 					CanPutInRuck		(PIItem pIItem) const;
	bool					CanTakeItem			(CInventoryItem *inventory_item) const;
	
	//Check if belts have enough space
	void					CheckBeltCapacity();
	//Check if sloting is not blocked by other items
	bool					CheckCompatibility(PIItem moving_item) const;
	//Ruck anything, that is incompatible with our item
	void					RuckIncompatible(PIItem priority_item);

	// ������ ��� �������� ���� � ������� ��� �������� � ���
	bool					ActivateSlot		(TSlotId slot, bool bForce = false);
	void					ActivateDetector	();
	bool					TryActivate			(TSlotId slot, bool forced_activation);
	void					TryNextActiveSlot	();
	PIItem					ActiveItem			()const					{return m_iActiveSlot==NO_ACTIVE_SLOT ? NULL :m_slots[m_iActiveSlot].m_pIItem;}

	bool					InputAction			(int cmd, u32 flags);
	void					Update				();

	// ���� ������� �� ������
	PIItem					GetItemBySect		(const char *section, TIItemContainer& container_to_search) const;
	// ���� ������� �� ID �������
	PIItem					GetItemByID			(const u16 id, TIItemContainer& container_to_search) const;
	// ���� ������� �� ID ������
	PIItem					GetItemByClassID	(CLASS_ID cls_id, TIItemContainer& container_to_search) const;
	// ���������� ������� �� ������� �� ������ �������
	PIItem					GetItemByIndex		(int iIndex, TIItemContainer& container_to_search);
	// ���� ������� �� �������� �����
	PIItem					GetItemByName		(LPCSTR gameobject_name, TIItemContainer& container_to_search);

	// ���� ������� � ��� �� �������, �� ����� ��� �� ��� ���� � ��� �� �������
	PIItem					GetNotSameBySect	(const char *section, const PIItem not_same_with, TIItemContainer& container_to_search) const;
	// ���� ������� ��� ���������� �����, �� ����� ��� �� ��� ���� � ��� �� �������
	PIItem					GetNotSameBySlotNum	(const TSlotId slot, const PIItem not_same_with, TIItemContainer& container_to_search) const;

	// ����� ������� ��� ��� � ��. ���� ������� �� ������
	PIItem					GetAmmoFromInv		(const char *section) const;
	// ���� ����� �������� �� ������
	PIItem					GetAmmo				(const char *section, const TIItemContainer& container_to_search) const;

	// get item count
	u32						GetItemsCount();
	// get item count with the same section name
	u32						GetSameItemCountBySect(LPCSTR section, TIItemContainer& container_to_search);
	// get item count with the same slot num
	u32						GetSameItemCountBySlot(TSlotId slot_id, TIItemContainer& container_to_search);

	PIItem					ItemFromSlot		(TSlotId slot) const;

	float 					GetTotalWeight() const;
	float 					CalcTotalWeight();					

	void					SetActiveSlot		(TSlotId ActiveSlot)		{m_iActiveSlot = m_iNextActiveSlot = ActiveSlot; }
	TSlotId					GetActiveSlot		() const					{return m_iActiveSlot;}

	void					SetPrevActiveSlot	(TSlotId ActiveSlot)		{m_iPrevActiveSlot = ActiveSlot;}
	TSlotId					GetPrevActiveSlot	() const					{return m_iPrevActiveSlot;}
	void					SetNextActiveSlot	(TSlotId next_active_slot)	{ m_iNextActiveSlot = next_active_slot; }
	TSlotId					GetNextActiveSlot	() const					{return m_iNextActiveSlot;}


	bool 					IsSlotsUseful		() const					{return m_bSlotsUseful;}	 
	void 					SetSlotsUseful		(bool slots_useful)			{m_bSlotsUseful = slots_useful;}
	bool 					IsBeltUseful		() const					{return m_bBeltUseful;}
	void 					SetBeltUseful		(bool belt_useful)			{m_bBeltUseful = belt_useful;}
	bool 					IsHandsOnly			() const					{return m_bHandsOnly;}	 
	void 					SetHandsOnly		(bool hands_only)			{m_bHandsOnly = hands_only;}

	void					SetSlotsBlocked		(u16 mask, bool bBlock, bool unholster = true);
	bool					AreSlotsBlocked		();

	void					SetCurrentDetector	(CArtDetectorBase* detector) { m_currentDetectorInHand = detector;}; 	// ��� ����������. ����������� ��� ����������� ��������� � ���� �� ����� ����� ������
	CArtDetectorBase*		CurrentDetector() const					{ return m_currentDetectorInHand; };

	IC TSlotId				FirstSlot			() const					{return KNIFE_SLOT;}
	IC TSlotId				LastSlot			() const					{return LAST_SLOT;} // not "end"
	IC bool					SlotIsPersistent	(TSlotId slot_id) const {return m_slots[slot_id].m_bPersistent;}

	TIItemContainer			allContainer_;
	TIItemContainer			ruck_;
	TIItemContainer			belt_;
	TIItemContainer			artefactBelt_;

	TISlotArr				m_slots;
	
	// ������� ��������� ���������� �������� �� ������������� ����������� �� ������� ��������
	bool					LuaCheckCanTrade			(CAI_Stalker* npc, PIItem item_to_check) const;
	//���������� ��� ����� PDA � ����� � �����
	void					AddAvailableItems			(TIItemContainer& items_container, bool for_trade) const;

	struct SInventorySelectorPredicate
	{
		virtual bool operator() (PIItem item) = 0;
	};

	void				AddAvailableItems(TIItemContainer& items_container, SInventorySelectorPredicate& pred) const;

	float				GetTakeDist					() const				{return m_fTakeDist;}
	void				SetTakeDist					(float dist)			{m_fTakeDist = dist;}
	
	float				GetMaxWeight				() const				{return m_fMaxWeight;}
	void				SetMaxWeight				(float weight)			{m_fMaxWeight = weight;}
	bool				CanBeDragged				()						{return (m_fTotalWeight<30.f);} // skyloader: dont use < m_fMaxWeight because maximum value is 1000.f for stalkers

	u32					GetBeltMaxSlots				() const;
	u32					GetArtBeltMaxSlots			() const;

	inline	CInventoryOwner*GetOwner				() const				{ return m_pOwner; }
	

	// ������, �� ������� ������� ������
	PIItem				m_pTarget;

	friend class CInventoryOwner;


	u32					ModifyFrame					() const					{ return m_dwModifyFrame; }
	void				InvalidateState()							{ m_dwModifyFrame = CurrentFrame(); }
	void				Items_SetCurrentEntityHud	(bool current_entity);
protected:
	void				UpdateDropTasks		();
	void				UpdateDropItem		(PIItem pIItem);
	TSlotId				GetSlotByKey(int cmd);

	// �������� ���� � ���� ������� ������ �������� ����� �����
    //�������� ��������� � ������� ��������� (��� ����� ������)
	TSlotId 			m_iActiveSlot;
	TSlotId 			m_iNextActiveSlot;
	TSlotId 			m_iPrevActiveSlot;

	// ��� ����������. ����������� ��� ����������� ��������� � ���� �� ����� ����� ������
	CArtDetectorBase*	m_currentDetectorInHand;

	u32					m_needToActivateWeapon; //��� ���������� ����� ������

	CInventoryOwner*	m_pOwner;

	//����, ������������ ������� ����� � ���������
	bool				m_bBeltUseful;
	//����, ����������� ������������� ������
	bool				m_bSlotsUseful;
	//if need to block all slots and inventory
	bool				m_bHandsOnly;

	// ������������ ��� ���������
	float				m_fMaxWeight;
	// ������� ��� � ���������
	float				m_fTotalWeight;

	// ������������ ���-�� ��������
	//�� ������� �����
	u32					m_iMaxBelt;
	//�� ����� ����������
	u32					m_iMaxArtBelt;

	// ������������ ���������� �� ������� ����� ��������� ������
	float				 m_fTakeDist;

	//���� �� ������� ��������� ��������� ��������� � ���������
	u32					m_dwModifyFrame;

	bool				m_drop_last_frame;
};