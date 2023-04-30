#pragma once

#include "eatable_item_object.h"

class CBattery : public CEatableItemObject
{
	private:
		typedef	CEatableItemObject inherited;

	public:
									CBattery				();
		virtual						~CBattery				();
		
		virtual		void			LoadCfg					(LPCSTR section);
		
		virtual		void			DestroyClientObj		();
		virtual		BOOL			SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
			
		virtual		void			AfterAttachToParent		();
		virtual		void			BeforeDetachFromParent	(bool just_before_destroy);
		
		virtual		void			ScheduledUpdate			(u32 dt);
		virtual		void			UpdateCL				();
		
		virtual		void			renderable_Render		(IRenderBuffer& render_buffer);
		
		virtual		bool			UseBy					(CEntityAlive* npc);

		virtual		bool			Empty						() const;

	
};