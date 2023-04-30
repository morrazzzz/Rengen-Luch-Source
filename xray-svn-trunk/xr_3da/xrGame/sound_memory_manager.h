
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#pragma once

#include "memory_space.h"
#include "sound_memory_rank_K.h"

#ifdef DEBUG
#	define USE_SELECTED_SOUND
#endif

namespace MemorySpace
{
	struct CSoundObject;
};

enum ESoundTypes;

class CCustomMonster;
class CAI_Stalker;

class CSoundMemoryManager
{
public:
	typedef MemorySpace::CSoundObject					CSoundObject;
	typedef xr_vector<CSoundObject>						SOUNDS;
	typedef xr_map<ESoundTypes,u32>						PRIORITIES;

private:
	struct CDelayedSoundObject
	{
		ALife::_OBJECT_ID	m_object_id;
		CSoundObject		m_sound_object;
	};

private:
	typedef xr_vector<CDelayedSoundObject>				DELAYED_SOUND_OBJECTS;

private:
	CSound_UserDataVisitor		*m_visitor;
	DELAYED_SOUND_OBJECTS		m_delayed_objects;

private:
	SOUNDS						soundMemory_;
	PRIORITIES					m_priorities;
	u32							m_max_sound_count;

	SOUNDS						sObjectsForSaving_;

	// Домножители параметров слышимости от ранга нпс
	SSoundParametersKoef		soundRankCoef_;

	SOUNDS						storedHeardAllies_;
	SOUNDS						storedHeardEnemies_;
	
private:
	u32							m_last_sound_time;
	u32							m_sound_decrease_quant;
	float						m_decrease_factor;
	float						m_min_sound_threshold;
	float						m_sound_threshold;
	float						m_self_sound_factor;

private:
	float						m_weapon_factor;
	float						m_item_factor;
	float						m_npc_factor;
	float						m_anomaly_factor;
	float						m_world_factor;

private:
#ifdef USE_SELECTED_SOUND
	CSoundObject				*m_selected_sound;
#endif

private:
	IC		void				update_sound_threshold	();
	IC		u32					priority				(const CSoundObject &sound) const;
			void				add						(const CSoundObject &sound_object, bool check_for_existance = false);
			void				add						(const CObject *object, int sound_type, const Fvector &position, float sound_power);

protected:
	IC		void				priority				(const ESoundTypes &sound_type, u32 priority);

public:
	IC							CSoundMemoryManager		(CCustomMonster *object, CAI_Stalker *stalker, CSound_UserDataVisitor *visitor);
	virtual						~CSoundMemoryManager	();
	virtual	void				LoadCfg					(LPCSTR section);
	virtual	void				reinit					();
	virtual	void				reload					(LPCSTR section);
	virtual void				feel_sound_new			(CObject* who, int eType, CSound_UserDataPtr user_data, const Fvector &Position, float power);
	virtual	void				update					();
			void				remove_links			(CObject *object);

			bool				IsOKToCheckInvalid		() { return m_delayed_objects.empty(); };


	// update prestored list of visible now alives for getting it in perfomance inportant places
			void				UpdateStoredHeard		();

	// returns visible_now allies
	const SOUNDS&				GetHeardAllies		() { return storedHeardAllies_; };
	// returns visible_now enemies
	const SOUNDS&				GetHeardEnemies		() { return storedHeardEnemies_; };

	CCustomMonster				*m_object;
	CAI_Stalker					*m_stalker;

public:
			void				enable					(const CObject *object, bool enable);

			//update the section for rank dependance. Usualy comes from rank change call back
			void				UpdateHearing_RankDependance();

public:
	const	CSoundObject*		GetIfIsInPool			(const CGameObject *game_object);

	IC		const SOUNDS		&GetMemorizedSounds		() const;
	IC		SOUNDS*				GetMemorizedSoundsP		();

	u32							forgetSndTime_;
	u8							countOfSavedAlliesSounds_;

#ifdef USE_SELECTED_SOUND
	IC		const CSoundObject	*sound					() const;
#endif
	IC		void				set_squad_objects		(SOUNDS *squad_objects);

	IC		const SSoundParametersKoef& HearingRankDependancy() const;

	IC		float				GetSoundThreshold() const;

	IC		u32					GetLastSoundTime() { return m_last_sound_time; };

	IC		float				GetWeaponSoundValue() const;
	IC		float				GetItemSoundValue() const;
	IC		float				GetNpcSoundValue() const;
	IC		float				GetAnomalySoundValue() const;
	IC		float				GetWorldSoundValue() const;

public:
	IC		void				set_threshold			(float threshold);
	IC		void				restore_threshold		();

public:
			void				save					(NET_Packet &packet);
			void				load					(IReader &packet);
			void				on_requested_spawn		(CObject *object);

private:
			void				clear_delayed_objects	();
};

#include "sound_memory_manager_inline.h"