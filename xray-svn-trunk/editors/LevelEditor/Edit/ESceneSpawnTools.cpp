#include "stdafx.h"
#pragma hdrstop

#include "ESceneSpawnTools.h"
#include "ui_leveltools.h"
#include "ESceneSpawnControls.h"
#include "FrameSpawn.h"
#include "Scene.h"
#include "SceneObject.h"
#include "spawnpoint.h"
#include "builder.h"
#include "../../../xr_3da/xrGame/LevelGameDef.h"
#include "EditLibrary.h"

#include "../ECore/Editor/ui_main.h"
#include "../../xr_3da/xrGame/xrServer_Objects_Abstract.h"
#include "../ECore/Editor/Library.h"

static HMODULE hXRSE_FACTORY = 0;
static LPCSTR xrse_factory_library	= "xrSE_Factory.dll";
static LPCSTR create_entity_func 	= "_create_entity@4"; 
static LPCSTR destroy_entity_func 	= "_destroy_entity@4";
Tcreate_entity 	create_entity;
Tdestroy_entity destroy_entity;

extern bool previewObjectEnabled_;
extern Fmatrix previewRotation_;
extern float previewScale_;


CEditableObject* ESceneSpawnTools::get_draw_visual(u8 _RP_TeamID, u8 _RP_Type, const u8& _GameType)
{
	CEditableObject* ret = NULL;

	if(m_draw_RP_visuals.empty())
    {
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\artefakt_ah"));     		//0
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\artefakt_cta_blue"));     //1
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\artefakt_cta_green"));    //2
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\telo_ah_cta_blue"));      //3
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\telo_ah_cta_green"));     //4
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\telo_dm"));               //5
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\telo_tdm_blue"));         //6
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\telo_tdm_green"));        //7
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\spectator"));        		//8
        m_draw_RP_visuals.push_back(Lib.CreateEditObject("editor\\item_spawn"));       		//9
    }

    return ret;
}

void __stdcall  FillSpawnItems	(ChooseItemVec& lst, void* param)
{
	LPCSTR gcs					= (LPCSTR)param;
    ObjectList objects;
    Scene->GetQueryObjects		(objects,OBJCLASS_SPAWNPOINT,-1,-1,-1);
    
    xr_string itm;
    int cnt 					= _GetItemCount(gcs);
    for (int k=0; k<cnt; k++){
        _GetItem				(gcs,k,itm);
        for (ObjectIt it=objects.begin(); it!=objects.end(); it++)
            if ((*it)->OnChooseQuery(itm.c_str()))	lst.push_back(SChooseItem((*it)->Name,""));
    }
}

ESceneSpawnTools::ESceneSpawnTools	():ESceneCustomOTools(OBJCLASS_SPAWNPOINT)
{
	m_Flags.zero();
    TfrmChoseItem::AppendEvents	(smSpawnItem,		"Select Spawn Item",		FillSpawnItems,		0,0,0,0);

    hXRSE_FACTORY	= LoadLibrary(xrse_factory_library);									VERIFY3(hXRSE_FACTORY,"Can't load library:",xrse_factory_library);
    create_entity 	= (Tcreate_entity)	GetProcAddress(hXRSE_FACTORY,create_entity_func);  	VERIFY3(create_entity,"Can't find func:",create_entity_func);
    destroy_entity 	= (Tdestroy_entity)	GetProcAddress(hXRSE_FACTORY,destroy_entity_func);	VERIFY3(destroy_entity,"Can't find func:",destroy_entity_func);

    m_Classes.clear			();
    CInifile::Root const& data 	= pSettings->sections();
    for (CInifile::RootCIt it=data.begin(); it!=data.end(); it++){
    	LPCSTR val;
    	if ((*it)->line_exist	("$spawn",&val)){
        	CLASS_ID cls_id	= pSettings->r_clsid((*it)->Name,"class");
        	shared_str v	= pSettings->r_string_wb((*it)->Name,"$spawn");
        	m_Classes[cls_id].push_back(SChooseItem(*v,*(*it)->Name));
        }
    }

	Msg("Checking Spawn Profiles...");
	for (ClassSpawnMapIt it_t = m_Classes.begin(); it_t != m_Classes.end(); it_t++)
	{
		Msg(LINE_SPACER);

		string16 temp; CLSID2TEXT((*it_t).first, temp);
		Msg("Profiles for Class %s:", temp);

		for (u32 i = 0; i < (*it_t).second.size(); i++)
		{
			Msg("'$spawn' = %s; Section = %s", (*it_t).second[i].name.size() ? (*it_t).second[i].name.c_str() : "NO SPAWN PROFILE", (*it_t).second[i].hint.size() ? (*it_t).second[i].hint.c_str() : "NO ITEM SECTION");

			if ((*it_t).second[i].name.size())
			{
				u32 same_count = 0;
				for (u32 x = 0; x < (*it_t).second.size(); x++)
				{
					bool found_dublicate = false;

					if ((*it_t).second[x].name.size())
					{
						if (!xr_strcmp((*it_t).second[i].name, (*it_t).second[x].name)) // Comparing $spawn parametr of two sections
						{
							if (xr_strcmp((*it_t).second[i].hint, (*it_t).second[x].hint)) // Dont count self to self comparision
							{
								same_count += 1;
								found_dublicate = true;
							}
						}

						if (found_dublicate)
						{
							Msg(LINE_SPACER);
							Msg("!!!Dublicate Spawn Profile Found: Section [%s] has same '$spawn' node with Section [%s]. Dublicating Spawn Profile = '%s'. Dublicates count = %u. Make sure '$spawn' is not inherited",
								(*it_t).second[i].hint.c_str(), (*it_t).second[x].hint.c_str(), (*it_t).second[i].name.c_str(), same_count);
							Msg(LINE_SPACER);

							VERIFY2(0, make_string("Section [%s] has same '$spawn' node with Section [%s]. Dublicating Spawn Profile = '%s'. Dublicates count = %u. Make sure '$spawn' is not inherited",
								(*it_t).second[i].hint.c_str(), (*it_t).second[x].hint.c_str(), (*it_t).second[i].name.c_str(), same_count));
						}
					}
				}
			}
		}
	}

	visualForPreview_ = NULL;
	previewRotation_.rotateX(0.f);
}

ESceneSpawnTools::~ESceneSpawnTools()
{
	FreeLibrary		(hXRSE_FACTORY);
    m_Icons.clear	();

	xr_vector<CEditableObject*>::iterator it 	= m_draw_RP_visuals.begin();
	xr_vector<CEditableObject*>::iterator it_e 	= m_draw_RP_visuals.end();

    for(;it!=it_e;++it)
    {
		Lib.RemoveEditObject(*it);
    }

    m_draw_RP_visuals.clear();

	if (visualForPreview_)
		::Render->model_Delete(visualForPreview_, TRUE);
}

void ESceneSpawnTools::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
    AddControl		(xr_new<TUI_ControlSpawnAdd>(estDefault,etaAdd,		this));
	// frame
    pFrame 			= xr_new<TfraSpawn>((TComponent*)0);
}
//----------------------------------------------------
 
void ESceneSpawnTools::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

void ESceneSpawnTools::FillProp(LPCSTR pref, PropItemVec& items)
{            
//.	PHelper().CreateFlag32(items, PrepareKey(pref,"Common\\Show Spawn Type"),	&m_Flags,		flShowSpawnType);
//.	PHelper().CreateFlag32(items, PrepareKey(pref,"Common\\Trace Visibility"),	&m_Flags,		flPickSpawnType);
	inherited::FillProp	(pref, items);
}
//------------------------------------------------------------------------------

ref_shader ESceneSpawnTools::CreateIcon(shared_str name)
{
    ref_shader S;
    if (pSettings->line_exist(name,"$ed_icon")){
	    LPCSTR tex_name = pSettings->r_string(name,"$ed_icon");
    	S.create("editor\\spawn_icon",tex_name);
        m_Icons[name] = S;
    }else{
        S = 0;
    }
    return S;
}

ref_shader ESceneSpawnTools::GetIcon(shared_str name)
{
	ShaderPairIt it = m_Icons.find(name);
	if (it==m_Icons.end())	return CreateIcon(name);
	else					return it->second;
}
//----------------------------------------------------
#include "EShape.h"

CCustomObject* ESceneSpawnTools::CreateObject(LPVOID data, LPCSTR name)
{
	CSpawnPoint* O	= xr_new<CSpawnPoint>(data,name);
    O->ParentTools		= this;
	if(data && name)
    {
        if(pSettings->line_exist( (LPCSTR)data, "$def_sphere") )
        {
        	float size 			= pSettings->r_float( (LPCSTR)data, "$def_sphere");

            CCustomObject* S	= Scene->GetOTools(OBJCLASS_SHAPE)->CreateObject(0,0);
            CEditShape* shape 	= dynamic_cast<CEditShape*>(S);
            R_ASSERT			(shape);

            Fsphere 			Sph;
            Sph.identity		();
            Sph.R				= size;
            shape->add_sphere	(Sph);
            O->AttachObject		(S);
        }
    }
    return O;
}
//----------------------------------------------------

int ESceneSpawnTools::MultiRenameObjects()
{
	int cnt			= 0;
    for (ObjectIt o_it=m_Objects.begin(); o_it!=m_Objects.end(); o_it++){
    	CCustomObject* obj	= *o_it;
    	if (obj->Selected()){
        	string256			pref;
            strconcat			(sizeof(pref),pref,Scene->LevelPrefix().c_str(),"_",obj->RefName());
            string256 			buf;
        	Scene->GenObjectName(obj->ClassID,buf,pref);
            if (obj->Name!=buf){
	            obj->Name		= buf;
                cnt++; 
            }
        }
    }
    return cnt;
}

void ESceneSpawnTools::OnRender(int priority, bool strictB2F)
{
	inherited::OnRender(priority, strictB2F);

	if (previewObjectEnabled_ && LTools->CurrentClassID() == OBJCLASS_SPAWNPOINT)
	{
		if (visualForPreview_)
		{
			Fmatrix	M = EDevice.m_Camera.GetTransform();

			M.mulB_44(previewRotation_);

			Fmatrix	S;
			S.scale(previewScale_, previewScale_, previewScale_);

			M.mulB_44(S);

			Fvector start, dir;
			Ivector2 pt;

			static int _w = 350;
			static int _h = 200;
			static float _kl = 1.0f;

			pt.x = iFloor(UI->GetRealWidth() - _w);
			pt.y = iFloor(UI->GetRealHeight() - _h);

			EDevice.m_Camera.MouseRayFromPoint(M.c, dir, pt);
			M.c.mad(dir, _kl);

			if (visualForPreview_)
				::Render->model_Render(visualForPreview_, M, 1, false, 1.f);
		}
	}
}

void ESceneSpawnTools::AnObjectSelectedInOList()
{
	if (pFrame)
	{
		LPCSTR ref_name = ((TfraSpawn*)pFrame)->Current();

		if (visualForPreview_)
			::Render->model_Delete(visualForPreview_, FALSE);

		if (ref_name && xr_strcmp(ref_name, RPOINT_CHOOSE_NAME) && xr_strcmp(ref_name, ENVMOD_CHOOSE_NAME))
		{
			ISE_Abstract* spawn_object = create_entity(ref_name);

			if (spawn_object)
			{
				if (spawn_object->visual())
					visualForPreview_ = ::Render->model_Create(spawn_object->visual()->visual_name.c_str());
			}

			if (visualForPreview_)
			{
				previewRotation_.rotateX(0.f);
				previewScale_ = 0.2f;
			}

			destroy_entity(spawn_object);
		}
	}
}

void ESceneSpawnTools::SwitchPreview()
{
	previewObjectEnabled_ = !previewObjectEnabled_;
	UI->RedrawScene();
}

