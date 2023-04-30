#include "stdafx.h"
#pragma hdrstop

#include "SceneObject.h"
#include "Scene.h"
//----------------------------------------------------
#define RENTGEN_LUCH_VERSION 0x0123
#define COP_CS_VERSION 0x0012
#define	SOC_OR_LA_VERSION 0x0011
#define	OL_OR_BUILDS_VERSION 0x0010

#define SCENEOBJ_CURRENT_VERSION SOC_OR_LA_VERSION

//----------------------------------------------------
#define SCENEOBJ_CHUNK_VERSION		  	0x0900
#define SCENEOBJ_CHUNK_REFERENCE     	0x0902
#define SCENEOBJ_CHUNK_PLACEMENT     	0x0904
#define SCENEOBJ_CHUNK_FLAGS			0x0905
//#define SCENEOBJ_CHUNK_OMOTIONS			0xF914
//#define SCENEOBJ_CHUNK_ACTIVE_OMOTION	0xF915
//#define SCENEOBJ_CHUNK_SOUNDS			0xF920
//#define SCENEOBJ_CHUNK_ACTIVE_SOUND		0xF921

bool CSceneObject::Load(IReader& F)
{
    bool bRes = true;
	do{
        u32 version = 0;
        string1024 buf;

        R_ASSERT(F.r_chunk(SCENEOBJ_CHUNK_VERSION, &version));

        if ((version != SCENEOBJ_CURRENT_VERSION))
		{
			if (version == OL_OR_BUILDS_VERSION)
				ELog.Msg(mtInformation, "CSceneObject: Loading Object from 'Obvilion Lost' or other builds. Object data version %u. Our data version %u", version, SCENEOBJ_CURRENT_VERSION);

			if (version == SOC_OR_LA_VERSION)
				ELog.Msg(mtInformation, "CSceneObject: Loading Object from 'SoC' or 'LA' addon. Object data version %u. Our data version %u", version, SCENEOBJ_CURRENT_VERSION);

			if (version == COP_CS_VERSION)
				ELog.Msg(mtInformation, "CSceneObject: Loading Object from 'CoP' or 'CS' addons. Object data version %u. Our data version %u", version, SCENEOBJ_CURRENT_VERSION);
        }

		if (version == OL_OR_BUILDS_VERSION)
		{
	        R_ASSERT(F.find_chunk(SCENEOBJ_CHUNK_PLACEMENT));

    	    F.r_fvector3(FPosition);
	        F.r_fvector3(FRotation);
    	    F.r_fvector3(FScale);
        }

		CCustomObject::Load(F);

        R_ASSERT(F.find_chunk(SCENEOBJ_CHUNK_REFERENCE));

		if (version != COP_CS_VERSION)
		{
			m_Version = F.r_s32();
			F.r_s32(); // advance (old read vers)
		}

        F.r_stringZ	(buf,sizeof(buf));

        if (!SetReference(buf))
        {
            ELog.Msg            (mtError, "Scene Object Error: '%s' not found in library. Need to select substitute or skip it forever", buf);

            bRes                = false;

            int mr              = mrNone;
			int comp_inquiry	= mrNone;

			LPCSTR replacer_path = NULL;

			bool apply_replacer = false;
			bool ask_for_manual_replace = true;

            xr_string       _new_name;
            bool b_found    = Scene->GetSubstObjectName(buf, _new_name); // look for already selected substitute

            if(b_found)
            {
				if (!Scene->applySelectedFromLibToAllFound_ && !Scene->skipLibraryRememberedInquiry_)
				{
					xr_string _message;
					_message = "Apply already selected from library:\nObject [" + xr_string(buf) + "] not found. Relace it with [" + _new_name + "]? \nPress 'No' to check object compatability ltx or to select a substitute from library";

					mr = ELog.DlgMsg(mtConfirmation, TMsgDlgButtons() << mbYes << mbNo << mbYesToAll << mbNoToAll, _message.c_str());
				}

				if (mr == 10)
				{
					Scene->applySelectedFromLibToAllFound_ = true;
				}
				else if (mr == mrNoToAll)
				{
					Scene->skipLibraryRememberedInquiry_ = true;
				}

				if (mrYes == mr || Scene->applySelectedFromLibToAllFound_)
                {
					Msg("Trying replacing by already selected from lib: %s -> %s", buf, _new_name.c_str());
                    bRes = SetReference(_new_name.c_str());

					if (!bRes)
						ELog.DlgMsg(mtError, "Error while replacing [%s] by [%s]", buf, _new_name.c_str());
                }
            }

            if(!bRes)
            {
				if (!Scene->skipLTXReplacerInquiry_)
				{
					if (Scene->missingObjsCompTable_)
					{
						string128 object_compatibility_section = MISSING_OBJ_COMP_TABLE_SECTION;

						if (Scene->missingObjsCompTable_->section_exist(object_compatibility_section)) // Check the section existance
						{
							if (Scene->missingObjsCompTable_->line_exist(object_compatibility_section, buf))
							{
								replacer_path = Scene->missingObjsCompTable_->r_string(object_compatibility_section, buf);

								Msg("There is replacer: new object = %s", replacer_path);

								if (!Scene->applyReplacerToAllFound_)
								{
									comp_inquiry = ELog.DlgMsg(mtConfirmation, TMsgDlgButtons() << mbYes << mbNo << mbYesToAll << mbNoToAll, "Compatibility table auto apply:\nObject not found [%s]. There is a compatible object specified in %s. \nDo you want to replace it with that one? [%s] -> [%s]. \nPress No to All to skip this message for this map", buf, MISSING_OBJ_COMP_TABLE_LTX, buf, replacer_path);

									if ((comp_inquiry == 10 || comp_inquiry == mrYes) && replacer_path)
									{
										ask_for_manual_replace = false;

										apply_replacer = true;

										if (comp_inquiry == 10)
											Scene->applyReplacerToAllFound_ = true;
									}

									if (comp_inquiry == mrNoToAll)
									{
										Scene->skipLTXReplacerInquiry_ = true;
									}
								}
								else
								{
									ask_for_manual_replace = false;
									apply_replacer = true;
								}
							}
						}
					}
				}

				if (!psDeviceFlags.is(rsSkipAllObjects))
				{
					if (mr == mrNone && ask_for_manual_replace)
						mr = ELog.DlgMsg(mtConfirmation, TMsgDlgButtons() << mbYes << mbNo << mbNoToAll, "Library Select: \nObject not found [%s]. Do you want to select it from library? \nPressing 'No' will lead to the deletion of this object, after you save the map \nPress No to All to skip this message for this map", buf);
                	else
                	    mr = mrNone;

					// Skip inquiry of selecting replacements. The sdk will still ask for replacements of already chosen ones
					if(mr == mrNoToAll)
					{
						psDeviceFlags.set(rsSkipAllObjects, TRUE);
						mr = mrNo;
					}
				}
				else 
					mr = mrNo;

				// Do select from library
				if (ask_for_manual_replace)
				{
					LPCSTR new_val = 0;

					if ((mr == mrNone || mr == mrYes) && TfrmChoseItem::SelectItem(smObject, new_val, 1))
					{
						bRes = SetReference(new_val);
						if (bRes)
						{
							Scene->RegisterSubstObjectName(buf, new_val); // This add the refference to the list, so that if same object is found - sdk will ask to use already specified one too

							Msg("Succesfuly replaced [%s] by [%s]", buf, new_val);
						}
						else
							ELog.DlgMsg(mtError, "Error while replacing [%s] by [%s]", buf, new_val);
					}
				}
				 
				// Apply replacer specified in LTX
				if (apply_replacer)
				{
					Msg("Try object compatability ltx replacer: [%s] -> [%s]", buf, replacer_path);

					bRes = SetReference(replacer_path);

					if (bRes)
						Msg("Succesfuly replaced [%s] by [%s]", buf, replacer_path);
					else
						ELog.DlgMsg(mtError, "Error while replacing [%s] by [%s]", buf, replacer_path);
				}
            }

            Scene->Modified();
        }

		if (EPrefs->object_flags.is(epoCheckVerObj) && version != COP_CS_VERSION)
		{
			if(!CheckVersion())
				ELog.Msg(mtError, "CSceneObject: '%s' different file version!", buf);
		}

        // flags
        if (F.find_chunk(SCENEOBJ_CHUNK_FLAGS))
		{
        	m_Flags.assign(F.r_u32());
        }

        if (!bRes)
			break;

    }while(0);

    return bRes;
}

void CSceneObject::Save(IWriter& F)
{
	CCustomObject::Save(F);

	F.open_chunk	(SCENEOBJ_CHUNK_VERSION);
	F.w_u16			(SCENEOBJ_CURRENT_VERSION);
	F.close_chunk	();

    // reference object version
    F.open_chunk	(SCENEOBJ_CHUNK_REFERENCE);
	
	R_ASSERT2(m_pReference, make_string("Empty SceneObject REFS. %s %s", RefName(), GetName()));

    F.w_s32			(m_pReference->Version());
    F.w_s32			(0); // reserved
    F.w_stringZ		(m_ReferenceName);
    F.close_chunk	();

    F.open_chunk	(SCENEOBJ_CHUNK_FLAGS);
	F.w_u32			(m_Flags.flags);
    F.close_chunk	();
}
//----------------------------------------------------


