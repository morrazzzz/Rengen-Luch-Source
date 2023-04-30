//////////////////////////////////////////////////////////////////////
// ExplosiveRocket.h:	ракета, которой стреляет RocketLauncher 
//						взрывается при столкновении
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CustomRocket.h"
#include "Explosive.h"
#include "inventory_item.h"

class CExplosiveRocket : 
			public CCustomRocket,
			public CInventoryItem,
			public CExplosive
{
private:
	typedef CCustomRocket inherited;
	friend CRocketLauncher;
public:
	CExplosiveRocket(void);
	virtual ~CExplosiveRocket(void);
	virtual DLL_Pure	*_construct	();

	virtual CExplosive					*cast_explosive			()						{return this;}
	virtual CInventoryItem				*cast_inventory_item	()						{return this;}
	virtual CAttachableItem				*cast_attachable_item	()						{return this;}
	virtual CWeapon						*cast_weapon			()						{return NULL;}
	virtual CGameObject					*cast_game_object		()						{return this;}
	virtual IDamageSource*				cast_IDamageSource()							{return CExplosive::cast_IDamageSource();}
	virtual void						on_activate_physic_shell();

	virtual void LoadCfg				(LPCSTR section);
	
	virtual void reinit					();
	virtual void reload					(LPCSTR section);
	
	virtual BOOL SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void DestroyClientObj		();
	virtual	void RemoveLinksToCLObj		(CObject* O );
	virtual void ExportDataToServer		(NET_Packet& P) {inherited::ExportDataToServer(P);}
	
	virtual void save					(NET_Packet &output_packet) {inherited::save(output_packet);}
	virtual void load					(IReader &input_packet)		{inherited::load(input_packet);}
	virtual BOOL net_SaveRelevant		()							{return inherited::net_SaveRelevant();}
	
	virtual void AfterDetachFromParent	();
	virtual void BeforeDetachFromParent	(bool just_before_destroy);
	virtual void AfterAttachToParent	()		{inherited::AfterAttachToParent();}
	virtual void BeforeAttachToParent	()		{inherited::BeforeAttachToParent();}
	
	virtual void renderable_Render		(IRenderBuffer& render_buffer)				{inherited::renderable_Render(render_buffer);}

	virtual void UpdateCL				();

	virtual void Contact(const Fvector &pos, const Fvector &normal);

	virtual void OnEvent (NET_Packet& P, u16 type) ;

	virtual	void Hit	(SHit* pHDS)
						{ inherited::Hit(pHDS); };

	virtual BOOL			UsedAI_Locations	()				{return inherited::UsedAI_Locations();}

#ifdef DEBUG
	virtual void			OnRender			();
#endif

	virtual void			activate_physic_shell	();
	virtual void			setup_physic_shell		();
	virtual void			create_physic_shell		();

	virtual bool			Useful				() const;
protected:
	virtual bool	use_parent_ai_locations	() const
	{
		return CAttachableItem::use_parent_ai_locations();
	}
};