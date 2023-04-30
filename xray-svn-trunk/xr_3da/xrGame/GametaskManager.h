#pragma once

#include "GameTaskDefs.h"
#include "object_interfaces.h"

class CGameTaskWrapper;
class CGameTask;
class CMapLocation;
class SGameTaskObjective;

class CGameTaskManager
{
	CGameTaskWrapper*		m_gametasks;
	enum		{eChanged	= (1<<0),};
	Flags8					m_flags;
	u32						m_actual_frame;
protected:
	void					UpdateActiveTask				();
public:

							CGameTaskManager				();
							~CGameTaskManager				();

	void					initialize						(u16 id);
	GameTasksVec&			GetGameTasks					();
	CGameTask*				HasGameTask						(const TASK_ID& id);
	CGameTask*				HasGameTask						(const CMapLocation* ml, bool only_inprocess);
	CGameTask*				GiveGameTaskToActor				(const TASK_ID& id, u32 timeToComplete, bool bCheckExisting=true);
	CGameTask*				GiveGameTaskToActor				(CGameTask* t, u32 timeToComplete, bool bCheckExisting=true);
	void					SetTaskState					(const TASK_ID& id, u16 objective_num, ETaskState state);
	void					SetTaskState					(CGameTask* t, u16 objective_num, ETaskState state);

	u32						ActualFrame						() const {return m_actual_frame;}
	void					ResetStorage					() {};
	
	void					UpdateTasks						();
//.	void					RemoveUserTask					(CMapLocation* ml);
	void					MapLocationRelcase				(CMapLocation* ml);

	CGameTask*				ActiveTask						();
	SGameTaskObjective*		ActiveObjective					();
	void					SetActiveTask					(const TASK_ID& id, u16 idx);

	void					DumpTasks						();
};
