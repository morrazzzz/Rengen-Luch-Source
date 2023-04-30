#pragma once
#include "inventory_item_object.h"

struct SCartridgeParam
{
	float	kDist, kDisp, kHit/*, kCritical*/, kImpulse, kAP, kAirRes, kSpeed;
	int		buckShot;
	float	impair;
	float	fWallmarkSize;
	u8		u8ColorID;

	float	aiAproximateEffectiveDistanceK_;

	IC void Init()
	{
		kDist = kDisp = kHit = kImpulse = kSpeed = 1.0f;
//		kCritical = 0.0f;
		kAP       = 0.0f;
		kAirRes   = 0.0f;
		buckShot  = 1;
		impair    = 1.0f;
		fWallmarkSize = 0.0f;
		u8ColorID     = 0;

		aiAproximateEffectiveDistanceK_ = 0.f;
	}
};

class CCartridge // : public IAnticheatDumpable
{
public:
	CCartridge();
	void Load(LPCSTR section, u8 LocalAmmoType);

	shared_str	m_ammoSect;
	enum{
		cfTracer				= (1<<0),
		cfRicochet				= (1<<1),
		cfCanBeUnlimited		= (1<<2),
		cfExplosive				= (1<<3),
		cfMagneticBeam			= (1<<4),
	};
	SCartridgeParam param_s;

	u8		m_LocalAmmoType;

	
	u16		bullet_material_idx;
	Flags8	m_flags;

	shared_str	m_InvShortName;

	void	DumpActiveParams(shared_str const & section_name, CInifile & dst_ini) const;
};

class CWeaponAmmo :	
	public CInventoryItemObject {
	typedef CInventoryItemObject		inherited;
public:
									CWeaponAmmo			(void);
	virtual							~CWeaponAmmo		(void);

	virtual CWeaponAmmo				*cast_weapon_ammo	()	{return this;}
	virtual void					LoadCfg				(LPCSTR section);
	
	virtual BOOL					SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void					DestroyClientObj	();
	virtual void					ExportDataToServer	(NET_Packet& P);
	
	virtual void					BeforeAttachToParent	();
	virtual void					BeforeDetachFromParent	(bool just_before_destroy);
	
	virtual void					UpdateCL			();
	
	virtual void					renderable_Render	(IRenderBuffer& render_buffer);

	virtual bool					Useful				() const;
	virtual float					Weight				() const;
	virtual	u32						Cost				() const;

	bool							Get					(CCartridge &cartridge);

	SCartridgeParam cartridge_param;

	u16			m_boxSize;
	u16			m_boxCurr;
	bool		m_tracer;

public:
	virtual CInventoryItem *can_make_killing	(const CInventory *inventory) const;
};
