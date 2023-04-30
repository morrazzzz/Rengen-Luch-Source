#include "stdafx.h"
#include "../xrcdb/ispatial.h"
#include "irenderable.h"

IRenderable::IRenderable()
{
	renderable.xform.identity			();
	renderable.visual					= NULL;
	renderable.pROS						= NULL;
	renderable.pROS_Allowed				= TRUE;
	renderable.needToDelete				= false;
	ISpatial*		self				= dynamic_cast<ISpatial*> (this);
	if (self)		self->spatial.s_type	|= STYPE_RENDERABLE;
}

IRenderable::~IRenderable()
{
	VERIFY(!engineState.test(FRAME_RENDERING));

	if (renderable.pROS)
		Render->ros_destroy(renderable.pROS);

	Render->model_Delete(renderable.visual);
}

IRender_ObjectSpecific*				IRenderable::renderable_ROS				()	
{
#pragma todo("Need to avoid creating resources 'on the go'")
	if (0==renderable.pROS && renderable.pROS_Allowed)		renderable.pROS	= Render->ros_create(this);
	return renderable.pROS	;
}
