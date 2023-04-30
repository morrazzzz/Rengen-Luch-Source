#include "stdafx.h"
#include "render.h"

IRender_interface::~IRender_interface()		{};

//ENGINE_API	IRender_interface*	Render		= NULL;

// resources
IRender_Light::IRender_Light()
{
	readyToDestroy_ = false;
	alreadyInDestroyQueue_ = false;
}

IRender_Light::~IRender_Light()
{	

}

IRender_Glow::IRender_Glow()
{
	readyToDestroy = false;
	alreadyInDestroyQueue = false;
}

IRender_Glow::~IRender_Glow()
{	

}

void resptrcode_light::_dec()
{
	if (!p_)
		return;

	p_->dwReference--;

	if (!p_->dwReference)
		::Render->light_destroy(p_);
}

void resptrcode_glow::_dec()
{
	if (!p_)
		return;

	p_->dwReference--;

	if (!p_->dwReference)
		::Render->glow_destroy(p_);
}