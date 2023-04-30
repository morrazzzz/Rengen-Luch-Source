#pragma once

#include "../inventory_item.h"
#include "../character_info_defs.h"
#include "../ui_defs.h"
#include "UICellItem.h"
#include "../WeaponMagazined.h"

class CUIStatic;
class CInventory;
class CInventoryBox;

//размеры сетки в текстуре инвентаря
#define INV_GRID_WIDTH			50.0f
#define INV_GRID_HEIGHT			50.0f

//размеры сетки в текстуре иконок персонажей
#define ICON_GRID_WIDTH			64.0f
#define ICON_GRID_HEIGHT		64.0f
//размер иконки персонажа для инвенторя и торговли
#define CHAR_ICON_WIDTH			2
#define CHAR_ICON_HEIGHT		2	

namespace InventoryUtilities
{

//сравнивает элементы по пространству занимаемому ими в рюкзаке
//для сортировки
bool GreaterRoomInRuck			(PIItem item1, PIItem item2);
bool GreaterRoomInRuckCellItem	(CUICellItem* cell1, CUICellItem* cell12);
//для проверки свободного места
bool FreeRoom_inBelt	(const TIItemContainer& item_list, PIItem item, int width, int height);

//получить shader на иконки инвенторя
const ui_shader& GetEquipmentIconsShader();
//get shader for outfit icons in upgrade menu
const ui_shader& GetOutfitUpgradeIconsShader();
//get shader for weapon icons in upgrade menu
const ui_shader& GetWeaponUpgradeIconsShader();
//удаляем все шейдеры
void DestroyShaders();
void CreateShaders();

void UpdateWeaponUpgradeIconsShader(CUIStatic* item);
void UpdateOutfitUpgradeIconsShader(CUIStatic* item);

// Отобразить вес контейнера
void UpdateWeightContainer(CUIStatic &wnd, CInventory *pInventory, LPCSTR prefixStr = nullptr);

void UpdateWeight(CUIStatic &wnd, CInventoryOwner* inv_owner, bool withPrefix = false);
void UpdateWeight(CUIStatic &wnd, CInventoryBox* inv_box, bool withPrefix = false);

// Функции получения строки-идентификатора ранга и отношения по их числовому идентификатору
LPCSTR	GetRankAsText				(CHARACTER_RANK_VALUE		rankID);
LPCSTR	GetReputationAsText			(CHARACTER_REPUTATION_VALUE rankID);
LPCSTR	GetGoodwillAsText			(CHARACTER_GOODWILL			goodwill);

void	ClearCharacterInfoStrings	();

void	SendInfoToActor				(LPCSTR info_id);
bool	HasActorInfo				(LPCSTR info_id);
u32		GetGoodwillColor			(CHARACTER_GOODWILL gw);
u32		GetRelationColor			(ALife::ERelationType r);
u32		GetReputationColor			(CHARACTER_REPUTATION_VALUE rv);

enum EAccossoryType
{ 
	eNotAccessory,
	eAmmo,
	eAddon 
};

	//Determines if specified item section is within the specified addons or ammo lists
EAccossoryType			GetWeaponAccessoryType		(const shared_str&, const xr_vector<shared_str>& ammoTypes, const xr_vector<shared_str>& addons);

	//Gets the addons for specified weapon
xr_vector<shared_str>	GetAddonsForWeapon			(CWeaponMagazined* weapon);

	//Gets the sum of inv_grid_width param for items in specified TIItemContainer
u32						GetUsedSlots_Width			(TIItemContainer& container);

// For category based sorting out
struct SLeaveCategory
{
	Flags32 filt;

	SLeaveCategory(Flags32& filter)
	{
		filt = filter;
	}

	bool operator() (const PIItem item) const
	{
		return !(!!filt.test(item->GetInventoryCategory()));
	}
};

};