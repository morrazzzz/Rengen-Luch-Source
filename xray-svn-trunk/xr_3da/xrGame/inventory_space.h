#pragma once

#define CMD_START	(1<<0)
#define CMD_STOP	(1<<1)

#define SLOTS_TOTAL			15

enum ESlotId : u8
{
	NO_ACTIVE_SLOT = 0,
	KNIFE_SLOT = 1,
	PISTOL_SLOT,
	RIFLE_SLOT,
	GRENADE_SLOT,
	APPARATUS_SLOT,	//Binoccular
	BOLT_SLOT,
	OUTFIT_SLOT,
	PDA_SLOT,
	DETECTOR_SLOT,	// Artefact Detector
	TORCH_SLOT,
	ARTEFACT_SLOT,	// Deprecated/not used
	HELMET_SLOT,
	PNV_SLOT,
	ANOM_DET_SLOT,
	RIFLE_2_SLOT,
	LAST_SLOT		= RIFLE_2_SLOT
};

#define RUCK_HEIGHT			280
#define RUCK_WIDTH			7

class CInventoryItem;
class CInventory;

typedef CInventoryItem*				PIItem;
typedef xr_vector<PIItem>			TIItemContainer;
typedef u8							TSlotId;

enum EItemPlace
{			
	eItemPlaceUndefined = 0,
	eItemPlaceSlot,
	eItemPlaceBelt,
	eItemPlaceRuck,
	eItemPlaceArtBelt
};

struct SInvItemPlace
{
	union{
		struct{
			u16 type				: 4;
			u16 slot_id				: 6;
			u16 base_slot_id		: 6;
		};
		u16	value;
	};
};

extern u32	INV_STATE_LADDER;
extern u32	INV_STATE_CAR;
extern u32	INV_STATE_BLOCK_ALL;
extern u32	INV_STATE_INV_WND;

struct II_BriefInfo
{
	shared_str		name;
	shared_str		icon;
	shared_str		cur_ammo;
	shared_str		fmj_ammo;
	shared_str		ap_ammo;
	shared_str		fire_mode;
	shared_str		section;

	shared_str		grenade;

	II_BriefInfo() { clear(); }
	
	IC void clear()
	{
		name		= "";
		icon		= "";
		cur_ammo	= "";
		fmj_ammo	= "";
		ap_ammo		= "";
		fire_mode	= "";
		grenade		= "";
	}
};
