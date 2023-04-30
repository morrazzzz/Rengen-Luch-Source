#pragma once

#include "../CustomHUD.h"
#include "HitMarker.h"

extern	u32	ui_hud_type;

class CHUDTarget;
class CUIGameCustom;

class CHUDManager :
	public CCustomHUD
{
	friend class CUI;
private:
	CUIGameCustom*			pUIGame;
	CHitMarker				HitMarker;
	CHUDTarget*				m_pHUDTarget;
	bool					b_online;
public:
							CHUDManager			();
	virtual					~CHUDManager		();
	virtual		void		OnEvent				(EVENT E, u64 P1, u64 P2);
	
				bool		CheckHUDDrawConds	();

	virtual		void		Render_First		(IRenderBuffer& render_buffer);
	virtual		void		Render_Last			(IRenderBuffer& render_buffer);	   
	virtual		void		OnFrame				();

	virtual		void		RenderUI			();

		CUIGameCustom*		GetGameUI			(){return pUIGame;}

				void		HitMarked			(int idx, float power, const Fvector& dir);
				bool		AddGrenade_ForMark	( CGrenade* grn );
				void		Update_GrenadeView	( Fvector& pos_actor );

	//������� ������� �� ������� ������� HUD
	collide::rq_result&		GetCurrentRayQuery	();


	//�������� �������� ���� ������� � ����������� �� ������� ���������
	void					SetCrosshairDisp	(float dispf, float disps = 0.f);

	void					ShowCrosshair		(bool show);

	void					SetHitmarkType		(LPCSTR tex_name);
	void					SetGrenadeMarkType	( LPCSTR tex_name );

	virtual void			OnScreenResolutionChanged();
	virtual	void			Load				();
	virtual void			OnDisconnected		();
	virtual void			OnConnected			();

	virtual	void			RenderActiveItemUI	();
	virtual	bool			RenderActiveItemUIQuery();

	virtual void			RemoveLinksToCLObj	(CObject *object);
	//Lain: added
				void		SetRenderable       (bool renderable) { psHUD_Flags.set(HUD_DRAW_RT2,renderable); }
};

IC CHUDManager&			HUD()		{ return *((CHUDManager*)g_hud);}

