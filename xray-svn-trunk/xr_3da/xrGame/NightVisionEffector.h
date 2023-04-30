#pragma once

#include "HudSound.h"


class CActor;

///
/// \brief
///		Effector for night vision, high contrast, etc...
///
class CNightVisionEffector
{
public:
	enum EPlaySounds
	{
		eStartSound,
		eStopSound,
		eIdleSound,
		eBrokeSound
	};

	CNightVisionEffector(const char* snd_sect);
	~CNightVisionEffector();

	void StartPPE(const shared_str& sect, CActor* pA, bool play_sound = true);
	void StopPPE(const float factor, bool play_sound = true);
	void StartShader(const shared_str& sect, CActor* pA, bool play_sound = true);
	void StopShader(bool play_sound = true);
	bool IsActive();
	void OnDisabled(CActor* pA, bool play_sound = true);
	void PlaySounds(EPlaySounds which);

	bool HasSounds() const { return m_has_sounds; }

private:
	CActor* m_pActor;
	bool m_has_sounds;
	HUD_SOUND_COLLECTION m_sounds;
};

