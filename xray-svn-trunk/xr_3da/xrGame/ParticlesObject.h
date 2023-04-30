#ifndef ParticlesObjectH
#define ParticlesObjectH

#include "../PS_instance.h"

extern const Fvector zero_vel;

class CParticlesObject:	
	public CPS_Instance
{
	typedef CPS_Instance	inherited;

	u32					dwLastTime;
	void				Init				(LPCSTR p_name, IRender_Sector* S, BOOL bAutoRemove);
	void				UpdateSpatial		();

protected:
	bool				m_bLooped;			//флаг, что система зациклена
	bool				m_bStopping;		//вызвана функция Stop()

protected:

protected:
	virtual				~CParticlesObject	();

public:
						CParticlesObject	(LPCSTR p_name, BOOL bAutoRemove, bool destroy_on_game_load);

	virtual	void		Update				();

	virtual bool		shedule_Needed		()	{return true;};
	virtual float		shedule_Scale		()	;
	virtual void		ScheduledUpdate		(u32 dt);

	virtual void		renderable_Render	(IRenderBuffer& render_buffer);

	void				PerformAllTheWork	();
	void	__stdcall	PerformAllTheWork_mt();

	Fvector&			Position			();
	void				SetXFORM			(const Fmatrix& m);
	IC	Fmatrix&		XFORM				()	{return renderable.xform;}
	void				UpdateParent		(const Fmatrix& m, const Fvector& vel);

	void				play_at_pos			(const Fvector& pos, BOOL xform=FALSE);
	virtual void		Play				(bool bHudMode);
	virtual void		PlayStatic			(bool bHudMode);
	void				Stop				(BOOL bDefferedStop=TRUE);
	
	bool				IsLooped			() {return m_bLooped;}
	bool				IsAutoRemove		();
	bool				IsPlaying			();
	void				SetAutoRemove		(bool auto_remove);

	virtual	void		spatial_move_intern	();

	const shared_str			Name		();
public:
	static CParticlesObject*	Create		(LPCSTR p_name, BOOL bAutoRemove=TRUE, bool remove_on_game_load = true)
	{
		return xr_new <CParticlesObject>(p_name, bAutoRemove, remove_on_game_load);
	}
};

#endif /*ParticlesObjectH*/
