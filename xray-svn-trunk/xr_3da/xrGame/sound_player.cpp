////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_player.cpp
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sound player
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sound_player.h"
#include "script_engine.h"
#include "ai/stalker/ai_stalker_space.h"
#include "ai_space.h"
#include "../xr_object.h"
#include "../Include/xrRender/Kinematics.h"
#include "../bone.h"
#include "profiler.h"
#include "sound_collection_storage.h"
#include "object_broker.h"
#include "ai/stalker/ai_stalker.h"
#include "mt_config.h"

typedef fastdelegate::FastDelegate3 < CSoundPlayer::CSoundCollectionParamsFull, u32, u32 > sound_load_delegate;

struct s_thread_void
{
	sound_load_delegate delegate_to_process; // a void to process
	CSoundPlayer::CSoundCollectionParamsFull sound_paams;
	u32 internal_type;
	u32 call_id; // for debuging
	u32 returned_sounds_size; // this size should match "future-found" size of sounds
};

class CFindByDelegate
{
public:
	CFindByDelegate(sound_load_delegate input_delegate) { Delegate = input_delegate; }

	IC bool operator() (s_thread_void input) { return Delegate == input.delegate_to_process; }
private:
	sound_load_delegate Delegate;
};

extern ENGINE_API BOOL mtUseCustomAffinity_;

xr_vector<s_thread_void> sound_thread_voids; // pool of voids sent to be executed in sound aux thread
Mutex ThreadSoundPoolProtection; // Simultaneous pool access protection
bool sload_thread_spwaned = false; // flag "if thread is spawned already"

u32 call_id_debug = 0;

void SoundLoading_Thread(void *context)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	while (true)
	{
		Sleep(1);
		if (Device.IsAuxThreadsMustExit()) // exit thread, if engine has told to
		{
			Device.SoundsThreadExit_Sync.Set();
			return;
		}

		if (sound_thread_voids.size() > 0)
		{

			ThreadSoundPoolProtection.lock(); //Protect pool, while copying it to temporary one

			xr_vector<s_thread_void> temp_pool_copy; // make a copy of corrent pool, so that the access to pool is not raced by threads and main thread can safely push in more voids
			temp_pool_copy = sound_thread_voids;

			ThreadSoundPoolProtection.unlock();


			for (u32 i = 0; i < temp_pool_copy.size(); ++i) 
			{
				CTimer time; time.Start();
				//Msg("[Reading Params %u], call_id = %u", i, temp_pool_copy[i].call_id);

				sound_load_delegate ddelegate_to_execute;
				ddelegate_to_execute = temp_pool_copy[i].delegate_to_process;

				//Msg("# [Sound loading thread]:Processing void %u, with call_id = %u", i, temp_pool_copy[i].call_id); // temporary debug

				ddelegate_to_execute(temp_pool_copy[i].sound_paams, temp_pool_copy[i].internal_type, temp_pool_copy[i].returned_sounds_size);

				//Msg("# [Sound loading thread]:Processed void %u, with call_id = %u; Saved processing time = %f, frame finnished = %u", i, temp_pool_copy[i].call_id, time.GetElapsed_sec()*1000.f, CurrentFrame()); // temporary debug
			}


			ThreadSoundPoolProtection.lock(); //Protect pool, while thread cleans executed voids from pool

			//Msg("[Pool Size1 %u]", sound_thread_voids.size());
			for (xr_vector<s_thread_void>::const_iterator it = temp_pool_copy.begin(); it != temp_pool_copy.end(); ++it)
			{

				xr_vector<s_thread_void>::const_iterator located_void = std::find_if
					(
					sound_thread_voids.begin(),
					sound_thread_voids.end(),
					CFindByDelegate(it->delegate_to_process)
					);

				if (located_void != sound_thread_voids.end())
				{
					//Msg("[Erasing] call_id = %u", located_void->call_id);
					sound_thread_voids.erase(located_void);
				}
			}
			//Msg("[Pool Size2 %u]", sound_thread_voids.size());

			ThreadSoundPoolProtection.unlock();

		}
	}
}


void SpawnSoundLoadThread() // spawn thread that will execute sound collections loading
{
	if (!sload_thread_spwaned)
	{
		sload_thread_spwaned = true;
		thread_spawn(SoundLoading_Thread, "Sound Loading Thread", 0, 0);
		Msg("# Sound Loading Thread Spawned");
	}
}

CSoundPlayer::CSoundPlayer			(CObject *object)
{
	VERIFY							(object);
	m_object						= object;
	seed							(u32(CPU::QPC() & 0xffffffff));
	m_sound_prefix					= "";
}

CSoundPlayer::~CSoundPlayer			()
{
	clear							();
}

void CSoundPlayer::clear			()
{
	npc_sound_pool_protect.lock();
	m_sound_collections.clear();
	npc_sound_pool_protect.unlock();

	xr_vector<CSoundSingle>::iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::iterator	E = m_playing_sounds.end();
	for ( ; I != E; ++I)
		(*I).destroy				();

	m_playing_sounds.clear			();

	m_sound_mask					= 0;
}

void CSoundPlayer::reinit			()
{
}

void CSoundPlayer::reload			(LPCSTR section)
{
	VERIFY							(m_playing_sounds.empty());
	clear							();
	m_sound_prefix					= "";
}

void CSoundPlayer::unload			()
{
	remove_active_sounds			(u32(-1));
	VERIFY							(m_playing_sounds.empty());
}


u32 CSoundPlayer::add				(bool in_aux_thread, LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name, CSound_UserDataPtr data)
{
	//Msg("CSoundPlayer::add");

	CTimer time; time.Start();
	npc_sound_pool_protect.lock();

	SOUND_COLLECTIONS::iterator I = m_sound_collections.find(internal_type);
	if (m_sound_collections.end() != I)
	{
		npc_sound_pool_protect.unlock();

		R_ASSERT2(false, make_string("Sound Collection not Found: Used prefix %s, path %s", m_sound_prefix.c_str(), prefix));

		return(0);
	}

	npc_sound_pool_protect.unlock();

	CSoundCollectionParamsFull			sound_params;
	sound_params.m_priority				= priority;
	sound_params.m_synchro_mask			= mask;
	sound_params.m_bone_name			= bone_name;
	sound_params.m_sound_prefix			= prefix;
	sound_params.m_sound_player_prefix	= m_sound_prefix;
	sound_params.m_max_count			= max_count;
	sound_params.m_type					= type;

	//forward-find size of future loaded sounds
	int size = 0;
	for (int j = 0, N = _GetItemCount(*sound_params.m_sound_prefix); j < N; ++j)
	{
		string_path	fn, s, temp;
		LPSTR S = (LPSTR)&s;

		_GetItem(*sound_params.m_sound_prefix, j, temp);
		strconcat(sizeof(s), S, *sound_params.m_sound_player_prefix, temp);
		if (FS.exist(fn, "$game_sounds$", S, ".ogg"))
		{
			size++;
		}

		for (u32 i = 0; i < sound_params.m_max_count; ++i)
		{
			string256 name;
			xr_sprintf(name, "%s%d", S, i);

			if (FS.exist(fn, "$game_sounds$", name, ".ogg"))
			{
				size++;
			}
		}
	}

	if (g_mt_config.test(mtLoadNPCSounds) && in_aux_thread)
	{
		call_id_debug++;

		sound_load_delegate new_delegate;
		new_delegate = sound_load_delegate(this, &CSoundPlayer::add_new);

		s_thread_void params;
		params.delegate_to_process	= new_delegate;
		params.sound_paams			= sound_params;
		params.internal_type		= internal_type;
		params.call_id				= call_id_debug;
		params.returned_sounds_size = size;

		ThreadSoundPoolProtection.lock(); // Protect Access to pool while we are pushing-in new void
		sound_thread_voids.push_back(params);
		ThreadSoundPoolProtection.unlock();

		//Msg("Push_back void at frame %u call_id = %u", CurrentFrame(), params.call_id);
	}
	else
	{
		add_new(sound_params, internal_type, size);
	}

	//Msg("CSoundPlayer::add2 %fms size = %u", time.GetElapsed_sec()*1000.f, size);

	//if (size == 0) Msg("!Sounds are missing: prefix %s, path %s", m_sound_prefix.c_str(), prefix);

	return(size);
}

void CSoundPlayer::add_new(CSoundCollectionParamsFull sound_params, u32 internal_type, u32 returned_sounds_size)
{
	//Msg("CSoundPlayer::add_new1 %s", sound_params.m_sound_prefix.c_str());

	typedef CSoundCollectionStorage::SOUND_COLLECTION_PAIR	SOUND_COLLECTION_PAIR;

	const SOUND_COLLECTION_PAIR &pair = sound_collection_storage().object(sound_params);

	R_ASSERT(pair.first == (CSoundCollectionParams&)sound_params);
	R_ASSERT(pair.second);
	R_ASSERT(returned_sounds_size == pair.second->m_sounds.size());

	//Msg("CSoundPlayer::add_new2 %s, %u", sound_params.m_sound_prefix.c_str(), pair.second->m_sounds.size());

	npc_sound_pool_protect.lock();
	m_sound_collections.insert(std::make_pair(internal_type, std::make_pair(sound_params, pair.second)));
	npc_sound_pool_protect.unlock();
}


void CSoundPlayer::remove			(u32 internal_type)
{
	npc_sound_pool_protect.lock();

	SOUND_COLLECTIONS::iterator		I = m_sound_collections.find(internal_type);
	VERIFY							(m_sound_collections.end() != I);
	m_sound_collections.erase(I);

	npc_sound_pool_protect.unlock();
}

bool CSoundPlayer::check_sound_legacy(u32 internal_type) const
{
	SOUND_COLLECTIONS::const_iterator	J = m_sound_collections.find(internal_type);
	if (m_sound_collections.end() == J) {
#ifdef DEBUG
		ai().script_engine().script_log(eLuaMessageTypeMessage,"Can't find sound with internal type %d (sound_script = %d)",internal_type,StalkerSpace::eStalkerSoundScript);
#endif
		return						(false);
	}

	VERIFY								(m_sound_collections.end() != J);
	const CSoundCollectionParamsFull	&sound = (*J).second.first;
	if (sound.m_synchro_mask & m_sound_mask)
		return						(false);

	xr_vector<CSoundSingle>::const_iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::const_iterator	E = m_playing_sounds.end();
	for ( ; I != E; ++I)
		if ((*I).m_synchro_mask & sound.m_synchro_mask)
			if ((*I).m_priority <= sound.m_priority)
				return				(false);
	return							(true);
}

void CSoundPlayer::update			(float time_delta)
{
	START_PROFILE("Sound Player")
	remove_inappropriate_sounds		(m_sound_mask);
	update_playing_sounds			();
	STOP_PROFILE
}

void CSoundPlayer::remove_inappropriate_sounds(u32 sound_mask)
{
	m_playing_sounds.erase				(
		std::remove_if(
			m_playing_sounds.begin(),
			m_playing_sounds.end(),
			CInappropriateSoundPredicate(sound_mask)
		),
		m_playing_sounds.end()
	);
}

void CSoundPlayer::update_playing_sounds()
{
	xr_vector<CSoundSingle>::iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::iterator	E = m_playing_sounds.end();
	for ( ; I != E; ++I) {
		if ((*I).m_sound->_feedback())
			(*I).m_sound->_feedback()->set_position(compute_sound_point(*I));
		else
			if (!(*I).started() && (EngineTimeU() >= (*I).m_start_time))
				(*I).play_at_pos		(m_object,compute_sound_point(*I));
			//else
		//		if (m_callback)
		//			m_callback((*I).
	}
}

bool CSoundPlayer::need_bone_data	() const
{
	xr_vector<CSoundSingle>::const_iterator	I = m_playing_sounds.begin();
	xr_vector<CSoundSingle>::const_iterator	E = m_playing_sounds.end();
	for ( ; I != E; ++I) {
		if ((*I).m_sound->_feedback())
			return					(true);
		else
			if (!(*I).started() && (EngineTimeU() >= (*I).m_start_time))
				return				(true);
	}
	return							(false);
}
/*
void CSoundPlayer::set_object_callback			(object_sound_callback& callback)
{
	m_callback		= callback;
}
*/
void CSoundPlayer::play				(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time, u32 id)
{
	if (!CheckNPCDifficultyForPlaying(internal_type))
		return;

	npc_sound_pool_protect.lock();

	if (!check_sound_legacy(internal_type)){
		npc_sound_pool_protect.unlock();
		return;
	}
	SOUND_COLLECTIONS::iterator	I = m_sound_collections.find(internal_type);
	VERIFY(m_sound_collections.end() != I);
	CSoundCollectionParamsFull	&sound = (*I).second.first;
	if ((*I).second.second->m_sounds.empty()) {
#ifdef DEBUG
	if (psAI_Flags2.test(aiDebugMsg)) {
		Msg						("- There are no sounds in sound collection \"%s\" with internal type %d (sound_script = %d)",*sound.m_sound_prefix,internal_type,StalkerSpace::eStalkerSoundScript);
	}
#endif		
		npc_sound_pool_protect.unlock();
		return;
	}

	npc_sound_pool_protect.unlock();

	remove_inappropriate_sounds	(sound.m_synchro_mask);

	CSoundSingle				sound_single;
	(CSoundParams&)sound_single	= (CSoundParams&)sound;
	sound_single.m_bone_id		= smart_cast<IKinematics*>(m_object->Visual())->LL_BoneID(sound.m_bone_name);

	sound_single.m_sound		= xr_new <ref_sound>();
	/**
	sound_single.m_sound->clone	(
		*(*I).second.second->m_sounds[
			id == u32(-1)
			?
			(*I).second.second->random(
				(*I).second.second->m_sounds.size()
			)
			:
			id
		],
		st_Effect,
		sg_SourceType
	);
	/**/
	sound_single.m_sound->clone	(
		(*I).second.second->random(id),
		st_Effect,
		sg_SourceType
	);

	sound_single.m_sound->_p->g_object		= m_object;
	sound_single.m_sound->_p->g_userdata	= (*I).second.first.m_data;
	VERIFY						(sound_single.m_sound->_handle());

	VERIFY						(max_start_time >= min_start_time);
	VERIFY						(max_stop_time >= min_stop_time);
	u32							random_time = 0;
	
	if (max_start_time)
		random_time				= (max_start_time > min_start_time) ? random(max_start_time - min_start_time) + min_start_time : max_start_time;

	sound_single.m_start_time	= EngineTimeU() + random_time;
	
	random_time					= 0;
	if (max_stop_time)
		random_time				= (max_stop_time > min_stop_time) ? random(max_stop_time - min_stop_time) + min_stop_time : max_stop_time;

	sound_single.m_stop_time	= sound_single.m_start_time + u32(sound_single.m_sound->_handle()->length_sec() * 1000.f) + random_time;
	m_playing_sounds.push_back	(sound_single);
	
	if (EngineTimeU() >= m_playing_sounds.back().m_start_time)
		m_playing_sounds.back().play_at_pos(m_object,compute_sound_point(m_playing_sounds.back()));
}

IC	Fvector CSoundPlayer::compute_sound_point(const CSoundSingle &sound)
{
	Fmatrix						l_tMatrix;
	l_tMatrix.mul_43			(m_object->XFORM(),smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(sound.m_bone_id).mTransform);
	return						(l_tMatrix.c);
}

CSoundPlayer::CSoundCollection::CSoundCollection(const CSoundCollectionParams &params)
{
	m_last_sound_id	= u32(-1);

	seed(u32(CPU::QPC() & 0xffffffff));

	m_sounds.clear();

	for (int j = 0, N = _GetItemCount(*params.m_sound_prefix); j < N; ++j)
	{
		string_path	fn, s, temp;
		LPSTR S = (LPSTR)&s;

		_GetItem(*params.m_sound_prefix, j, temp);
		strconcat(sizeof(s), S, *params.m_sound_player_prefix, temp);
		//Msg("N = %i", N);
		if (FS.exist(fn, "$game_sounds$", S, ".ogg"))
		{
			//Msg("Exist %i", N);
			ref_sound* temp = add(params.m_type, S);
			if (temp)
			{
				//Msg("m_sounds.push_back1");
				m_sounds.push_back(temp);
			}
		}

		for (u32 i = 0; i < params.m_max_count; ++i)
		{
			string256 name;
			xr_sprintf(name, "%s%d", S, i);
			//Msg("params.m_max_count = %u", params.m_max_count);
			if (FS.exist(fn, "$game_sounds$", name, ".ogg"))
			{
				//Msg("Exist %u", params.m_max_count);
				ref_sound* temp = add(params.m_type, name);
				if (temp)
				{
					//Msg("m_sounds.push_back2");
					m_sounds.push_back(temp);
				}
			}
		}
	}

#ifdef DEBUG
if (psAI_Flags2.test(aiDebugMsg)) {
	if (m_sounds.empty())
		Msg							("- There are no sounds with prefix %s",*params.m_sound_prefix);
}
#endif

}

CSoundPlayer::CSoundCollection::~CSoundCollection	()
{
#ifdef DEBUG
	xr_vector<ref_sound*>::iterator	I = m_sounds.begin();
	xr_vector<ref_sound*>::iterator	E = m_sounds.end();
	for ( ; I != E; ++I) {
		VERIFY						(*I);
		VERIFY						(!(*I)->_feedback());
	}
#endif
	delete_data						(m_sounds);
}

const ref_sound &CSoundPlayer::CSoundCollection::random	(const u32 &id)
{
	VERIFY					(!m_sounds.empty());

	if (id != u32(-1)) {
		m_last_sound_id		= id;
		VERIFY				(id < m_sounds.size());
		return				(*m_sounds[id]);
	}

	if (m_sounds.size() <= 2) {
		m_last_sound_id		= CRandom32::random(m_sounds.size());
		return				(*m_sounds[m_last_sound_id]);
	}
	
	u32						result;
	do {
		result				= CRandom32::random(m_sounds.size());
	}
	while (result == m_last_sound_id);

	m_last_sound_id			= result;
	return					(*m_sounds[result]);
}

bool CSoundPlayer::CheckNPCDifficultyForPlaying(u32 sound_mask)
{
	CAI_Stalker* npc = smart_cast<CAI_Stalker*>(m_object);

	if (!npc)
		return true;

	switch (sound_mask)
	{
	case StalkerSpace::EStalkerSounds::eStalkerSoundNeedBackup:
	case StalkerSpace::EStalkerSounds::eStalkerSoundSearch1WithAllies:
	case StalkerSpace::EStalkerSounds::eStalkerSoundSearch1NoAllies:
	case StalkerSpace::EStalkerSounds::eStalkerSoundAttackNoAllies:
	case StalkerSpace::EStalkerSounds::eStalkerSoundBackup:
	case StalkerSpace::EStalkerSounds::eStalkerSoundDetour:
	case StalkerSpace::EStalkerSounds::eStalkerSoundTolls:
	case StalkerSpace::EStalkerSounds::eStalkerSoundThrowGrenade:
		if (Random.randI(1, 101) < npc->GetTalkingChanceWhenFighting())
			return true;
		else
			return false;
	}

	return true;
}
