#include "stdafx.h"
#include "../xr_3da/xr_object.h"
#ifdef DEBUG
#include "ObjectDump.h"

ENGINE_API std::string dbg_object_base_dump_string( const CObject *obj )
{
	if( !obj )
		return make_string("object: NULL ptr");
	return make_string( "object name: %s, section name: %s, visual name: %s \n",
		                 obj->ObjectName().c_str(), 
						 obj->SectionName().c_str(), 
						 obj->Visual() ? obj->VisualName().c_str() : "none" );
}

ENGINE_API std::string dbg_object_poses_dump_string( const CObject *obj )
{
	if(!obj)
		return std::string("");
	
	u32 ps_size = obj->ps_Size();
	std::string buf("");
	for (u32 i = 0; i < ps_size; ++i )
	{
		const CObject::SavedPosition &svp = obj->ps_Element( i );
		buf +=( make_string(" \n %d, time: %d pos: %s  ", i, svp.dwTime, get_string(svp.vPosition).c_str() ) );
	}

	return make_string( "\n XFORM: %s \n position stack : %s \n,  ", get_string(obj->XFORM()).c_str(), buf.c_str() );
}

ENGINE_API std::string dbg_object_visual_geom_dump_string( const CObject *obj )
{
	if( !obj || !obj->Visual() )
			return std::string("");
	const Fbox &box = obj->BoundingBox();
	Fvector c;obj->Center( c );

	return make_string( "\n visual box: %s \n visual center: %s \n visual radius: %f ", 
		get_string(box).c_str(), get_string( c ).c_str(), obj->Radius() );
}

ENGINE_API std::string dbg_object_props_dump_string( const CObject *obj )
{
	if( !obj )
		return  std::string("");
	CObject::ObjectProperties props;
	obj->DBGGetProps( props );

	return make_string( " net_ID :%d, bActiveCounter :%d, bEnabled :%s, bVisible :%s, bDestroy :%s, net_Ready :%s, updatable :%s, bPreDestroy : %s ", 
		props.net_ID, props.bActiveCounter, get_string(bool (!!props.bEnabled ) ).c_str(), get_string(bool (!!props.bVisible ) ).c_str(), 
		get_string(bool (!!props.bDestroy ) ).c_str(), get_string( bool (!!props.net_Ready ) ).c_str(),
		get_string( bool (!!props.is_in_upd_list ) ).c_str(), get_string( bool (!!props.bPreDestroy ) ).c_str() 
		)
		+
		make_string( "\n dbg_update_cl: %d, dwFrame_UpdateCL: %d, Frame :%d, EngineTimeU(): %d  \n",
		obj->dbg_update_cl, obj->dwFrame_UpdateCL, CurrentFrame(), EngineTimeU() );
}
ENGINE_API std::string dbg_object_full_dump_string( const CObject *obj )
{
	return	dbg_object_base_dump_string( obj ) + 
			dbg_object_props_dump_string( obj )+
			dbg_object_poses_dump_string( obj ) +
			dbg_object_visual_geom_dump_string( obj );
			 
}
ENGINE_API std::string dbg_object_full_capped_dump_string( const CObject *obj )
{
	return	std::string("\n object dump: \n" ) +
			dbg_object_full_dump_string( obj );
}
#endif
