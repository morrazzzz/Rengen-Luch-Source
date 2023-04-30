#include "stdafx.h"
#pragma hdrstop

#include "ESceneGroupTools.h"
#include "GroupObject.h"
#include "SceneObject.h"

// chunks
static const u16 GROUP_TOOLS_VERSION  	= 0x0000;
//----------------------------------------------------
enum{
    CHUNK_VERSION			= 0x1001ul,
};
//----------------------------------------------------

bool ESceneGroupTools::Load(IReader& F)
{
	u16 version 	= 0;
    if(F.r_chunk(CHUNK_VERSION,&version))
        if( version!=GROUP_TOOLS_VERSION ){
            ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
            return false;
        }

	if (!inherited::Load(F)) return false;

    return true;
}
//----------------------------------------------------

void ESceneGroupTools::Save(IWriter& F)
{
	inherited::Save	(F);

	F.w_chunk		(CHUNK_VERSION,(u16*)&GROUP_TOOLS_VERSION,sizeof(GROUP_TOOLS_VERSION));
}
//----------------------------------------------------
 
bool ESceneGroupTools::LoadSelection(IReader& F)
{
	u16 version 	= 0;
    R_ASSERT(F.r_chunk(CHUNK_VERSION,&version));
    if( version!=GROUP_TOOLS_VERSION ){
        ELog.DlgMsg( mtError, "%s tools: Unsupported version.",ClassDesc());
        return false;
    }

	return inherited::LoadSelection(F);
}
//----------------------------------------------------

void ESceneGroupTools::SaveSelection(IWriter& F)
{
	F.w_chunk		(CHUNK_VERSION,(u16*)&GROUP_TOOLS_VERSION,sizeof(GROUP_TOOLS_VERSION));
    
	inherited::SaveSelection(F);
}
//----------------------------------------------------

void ESceneGroupTools::GetStaticDesc(int& v_cnt, int& f_cnt)
{
	for (ObjectIt it=m_Objects.begin(); it!=m_Objects.end(); it++){
    	CGroupObject* group = (CGroupObject*)(*it);
	    ObjectIt _O1 = group->GetObjects().begin();
    	ObjectIt _E1 = group->GetObjects().end();
	    for(;_O1!=_E1;_O1++){
	    	CSceneObject* obj = dynamic_cast<CSceneObject*>(*_O1);
			if (obj&&obj->IsStatic()){
				f_cnt	+= obj->GetFaceCount();
    	    	v_cnt  	+= obj->GetVertexCount();
	        }
        }
    }
}

#include "../ECore/Editor/ui_main.h"
void ESceneGroupTools::SelectObjects(bool flag)
{
	for (ObjectIt g_iter = m_Objects.begin(); g_iter != m_Objects.end(); g_iter++)
	{
		CGroupObject* g_object = dynamic_cast<CGroupObject*>((*g_iter));
		if (g_object)
		{
			if ((g_object)->Visible())
			{
				(g_object)->Select(flag);


				for (ObjectIt m_iter = g_object->GetObjects().begin(); m_iter != g_object->GetObjects().end(); m_iter++)
				{
					CCustomObject* g_member = (*m_iter);
					if (g_member)
					{
						if (flag == true)
						{
							// allow picking True only if group is Open
							if (g_object->IsOpened())
								g_member->Select(true);
						}
						else
							// if diselecting closed group - deselect members(even though mebmers should not be picked any way for closed group)
							g_member->Select(false);
					}

				}

			}
		}
	}

	UI->RedrawScene();
}
 