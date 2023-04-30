#include "stdafx.h"
#pragma hdrstop

#include "ESceneControlsCustom.h"
#include "ui_leveltools.h"
#include "scene.h"
#include "bottombar.h"
#include "ui_levelmain.h"
#include "leftbar.h"

extern CCustomObject* activeMouseObject_;

bool isRegularMoving = false;
bool isByAxisMoving = false;


extern bool previewObjectEnabled_;
bool mouseCapturedInPreview_ = false;
extern float previewScale_;

// values for preview window, where it captures controls
#define PREVIEW_W_W 500
#define PREVIEW_H_H 450

TUI_CustomControl::TUI_CustomControl(int st, int act, ESceneCustomMTools* parent){
	parent_tool		= parent; VERIFY(parent);
	sub_target		= st;
    action			= act;
}

bool TUI_CustomControl::Start  (TShiftState _Shift)
{

	// If mouse is in preview window rect - campture input, and skip object space window controls
	if (previewObjectEnabled_ &&  UI->m_CurrentCp.x > iFloor(UI->GetRealWidth() - PREVIEW_W_W) && UI->m_CurrentCp.y > iFloor(UI->GetRealHeight() - PREVIEW_H_H))
	{
		mouseCapturedInPreview_ = true;

		return PreviewMStart(_Shift);
	}

	switch(action){
	case etaSelect: return SelectStart(_Shift);
	case etaAdd: 	return AddStart(_Shift);
	case etaMove: 	return MovingStart(_Shift);
	case etaRotate: return RotateStart(_Shift);
	case etaScale: 	return ScaleStart(_Shift);
    }
	
    return false;
}

bool TUI_CustomControl::End    (TShiftState _Shift)
{
	// If mouse is in preview window rect - campture input, and skip object space window controls
	if (previewObjectEnabled_ && mouseCapturedInPreview_)
	{
		mouseCapturedInPreview_ = false;

		return PreviewMEnd(_Shift);
	}

	switch(action){
	case etaSelect: return SelectEnd(_Shift);
	case etaAdd: 	return AddEnd(_Shift);
	case etaMove: 	return MovingEnd(_Shift);
	case etaRotate: return RotateEnd(_Shift);
	case etaScale: 	return ScaleEnd(_Shift);
    }
	
    return false;
}

void TUI_CustomControl::Move   (TShiftState _Shift)
{
	// If mouse is in preview window rect - campture input, and skip object space window controls
	if (previewObjectEnabled_ && mouseCapturedInPreview_)
	{
		PreviewMMove(_Shift);

		return;
	}

	switch(action)
	{
	case etaSelect:	SelectProcess(_Shift); break;
	case etaAdd: 	AddProcess(_Shift);    break;
	case etaMove: 	MovingProcess(_Shift); break;
	case etaRotate:	RotateProcess(_Shift); break;
	case etaScale: 	ScaleProcess(_Shift);  break;
    }
}

bool TUI_CustomControl::HiddenMode()
{
	if (previewObjectEnabled_ && mouseCapturedInPreview_)
	{
		return true;
	}

	switch(action){
	case etaSelect:	return false;
	case etaAdd: 	return false;
	case etaMove:
	{
		return true;
	}
	case etaRotate:	return true;
	case etaScale: 	return true;
    }
    return false;
}


//------------------------------------------------------------------------------
// add
//------------------------------------------------------------------------------


CCustomObject* __fastcall TUI_CustomControl::DefaultAddObject(TShiftState Shift, TBeforeAppendCallback before, TAfterAppendCallback after)
{
	if (Shift == ssRBOnly)
	{
		ExecCommand(COMMAND_SHOWCONTEXTMENU, parent_tool->ClassID);
		return 0;
	}

	Fvector p, n;
	CCustomObject* obj = 0;

	if (!LUI->PickGround(p, UI->m_CurrentRStart, UI->m_CurrentRNorm, 1, &n))
	{
		p.x = UI->m_CurrentRStart.x + UI->m_CurrentRNorm.x;
		p.y = UI->m_CurrentRStart.y + UI->m_CurrentRNorm.y;
		p.z = UI->m_CurrentRStart.z + UI->m_CurrentRNorm.z;
	}
	{
		// before callback
		SBeforeAppendCallbackParams P;

		if (before&&!before(&P))
			return 0;

		string256 namebuffer;
		Scene->GenObjectName(parent_tool->ClassID, namebuffer, P.name_prefix.c_str());
		obj = Scene->GetOTools(parent_tool->ClassID)->CreateObject(P.data, namebuffer);

		if (!obj->Valid())
		{
			xr_delete(obj);

			return 0;
		}

		// after callback
		if (after&&!after(Shift, obj))
		{
			xr_delete(obj);

			return 0;
		}

		obj->MoveTo(p, n);

		if (Tools->GetSettings(etfRandomRot))
			obj->FRotation.set(0.f, Random.randF(-PI, PI), 0.f);

		Scene->SelectObjects(false, parent_tool->ClassID);
		Scene->AppendObject(obj);

		if (Shift.Contains(ssCtrl))
			ExecCommand(COMMAND_SHOW_PROPERTIES);

		if (!Shift.Contains(ssAlt))
			ResetActionToSelect();
	}

	return obj;
}

bool __fastcall TUI_CustomControl::AddStart(TShiftState Shift)
{
	DefaultAddObject(Shift,0);

    return false;
}
void __fastcall TUI_CustomControl::AddProcess(TShiftState _Shift)
{
}
bool __fastcall TUI_CustomControl::AddEnd(TShiftState _Shift)
{
    return true;
}

bool TUI_CustomControl::CheckSnapList(TShiftState Shift)
{
	if (fraLeftBar->ebSnapListMode->Down)
	{
		CCustomObject* O = Scene->RayPickObject(UI->ZFar(), UI->m_CurrentRStart, UI->m_CurrentRNorm, OBJCLASS_SCENEOBJECT, 0, 0);

		if (O)
		{
			if (Scene->FindObjectInSnapList(O))
			{
				if (Shift.Contains(ssAlt))
				{
					Scene->DelFromSnapList(O);
				}
				else if (Shift.Contains(ssCtrl))
				{
					Scene->DelFromSnapList(O);
				}
			}
			else
			{
				if (!Shift.Contains(ssCtrl) && !Shift.Contains(ssAlt))
				{
					Scene->AddToSnapList(O);
				}
				else if (Shift.Contains(ssCtrl))
				{
					Scene->AddToSnapList(O);
				}
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}


//------------------------------------------------------------------------------
// total select
//------------------------------------------------------------------------------


bool __fastcall TUI_CustomControl::SelectStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();

	if (CheckSnapList(Shift))
		return false;

	if (Shift == ssRBOnly)
	{
		ExecCommand(COMMAND_SHOWCONTEXTMENU, parent_tool->ClassID);

		return false;
	}

	if (!(Shift.Contains(ssCtrl) || Shift.Contains(ssAlt)))
		Scene->SelectObjects(false, cls);

	// Allow render selection rect
	UI->EnableSelectionRect(true);
	UI->UpdateSelectionRect(UI->m_StartCp, UI->m_CurrentCp);

	return true;
}

void __fastcall TUI_CustomControl::SelectProcess(TShiftState _Shift)
{
	UI->UpdateSelectionRect(UI->m_StartCp, UI->m_CurrentCp);
}

bool __fastcall TUI_CustomControl::SelectEnd(TShiftState _Shift)
{
	bool doo_rect_selection = false;

	int x_shift = UI->m_CurrentCp.x - UI->m_StartCp.x;
	int y_shift = UI->m_CurrentCp.y - UI->m_StartCp.y;

	// Check if developer moved mouse a little bit - if yes - that probably means he wants to select with rectangle
	if (x_shift > 5 || x_shift < -5 || y_shift > 5 || y_shift < -5)
		doo_rect_selection = true;

	// Do Rect selection or do single select
	if (doo_rect_selection)
		Scene->FrustumSelect(_Shift.Contains(ssAlt) ? 0 : 1, LTools->CurrentClassID());
	else
		Scene->RaySelect(_Shift.Contains(ssCtrl) ? -1 : _Shift.Contains(ssAlt) ? 0 : 1, parent_tool->ClassID);

	UI->EnableSelectionRect(false);

	// Calculate vartexes and faces count for displaing in button bar (for modelers)
	u32 vertexes = 0;
	u32 faces = 0;

	Scene->CalculateVertsFaces(vertexes, faces);

	LUI->UpdateVertsFacesInfo(vertexes, faces);

	return true;
}


//------------------------------------------------------------------------------------
// moving
//------------------------------------------------------------------------------------


bool __fastcall TUI_CustomControl::MovingStart(TShiftState Shift)
{
	// Move with mouse or move by pulling object axises
	if (activeMouseObject_)
	{
		isByAxisMoving = true;
		return ByAxisMoveStart(Shift);
	}
	else
		isRegularMoving = true;

	ObjClassID cls = LTools->CurrentClassID();

	if (Shift == ssRBOnly)
	{
		ExecCommand(COMMAND_SHOWCONTEXTMENU, parent_tool->ClassID);
		return false;
	}

	if (Scene->SelectionCount(true, cls) == 0)
		return false;

	// Move isntantly to the cursor pos
	if (Shift.Contains(ssCtrl))
	{
		ObjectList lst;
		if (Scene->GetQueryObjects(lst, LTools->CurrentClassID(), 1, 1, 0))
		{
			if (lst.size() == 1)
			{
				Fvector p, n;
				UI->IR_GetMousePosReal(EDevice.m_hRenderWnd, UI->m_CurrentCp);
				EDevice.m_Camera.MouseRayFromPoint(UI->m_CurrentRStart, UI->m_CurrentRNorm, UI->m_CurrentCp);

				if (LUI->PickGround(p, UI->m_CurrentRStart, UI->m_CurrentRNorm, 1, &n))
				{
					for (ObjectIt _F = lst.begin(); _F != lst.end(); _F++)
						(*_F)->MoveTo(p, n);

					Scene->UndoSave();
				}
			}
			else // ??? does not work or no affect...
			{
				Fvector p, n;
				Fvector D = { 0, -1, 0 };

				for (ObjectIt _F = lst.begin(); _F != lst.end(); _F++)
				{
					if (LUI->PickGround(p, (*_F)->PPosition, D, 1, &n))
					{
						(*_F)->MoveTo(p, n);
					}
				}
			}
		}

		return false;
	}
	else // Determine the moving vector and make a needed 3d vector axises
	{
		if (Tools->GetAxis() == etAxisY)
		{
			m_MovingXVector.set(0, 0, 0);
			m_MovingYVector.set(0, 1, 0);
		}
		else if (Tools->GetAxis() == etAxisX)
		{
			m_MovingXVector.set(1, 0, 0);
			m_MovingYVector.set(0, 0, 0);
		}
		else if (Tools->GetAxis() == etAxisZ)
		{
			m_MovingXVector.set(0, 0, 1);
			m_MovingYVector.set(0, 0, 0);
		}
		else if (Tools->GetAxis() == etAxisZX) // ZX plane
		{
			m_MovingXVector.set(EDevice.m_Camera.GetRight());
			m_MovingXVector.y = 0;

			m_MovingYVector.set(EDevice.m_Camera.GetDirection());
			m_MovingYVector.y = 0;

			m_MovingXVector.normalize_safe();
			m_MovingYVector.normalize_safe();
		}
		else if (Tools->GetAxis() == etAxisYX) // YX plane
		{
			m_MovingXVector.set(EDevice.m_Camera.GetRight());
			m_MovingXVector.z = 0.f;

			m_MovingYVector.set(EDevice.m_Camera.GetNormal());
			m_MovingYVector.z = 0.f;

			m_MovingXVector.normalize_safe();
			m_MovingYVector.normalize_safe();
		}
		else if (Tools->GetAxis() == etAxisYZ) // YZ plane
		{
			m_MovingXVector.set(EDevice.m_Camera.GetRight());
			m_MovingXVector.x = 0.f;

			m_MovingYVector.set(EDevice.m_Camera.GetNormal());
			m_MovingYVector.x = 0.f;

			m_MovingXVector.normalize_safe();
			m_MovingYVector.normalize_safe();
		}

		m_MovingReminder.set(0, 0, 0);
	}
	return true;
}


bool __fastcall TUI_CustomControl::DefaultMovingProcess(TShiftState Shift, Fvector& amount)
{
    if (Shift.Contains(ssLeft) || Shift.Contains(ssRight))
    {
		// modify movement by mouse sens and mouse delta
		float sensativity = UI->m_MouseSM;

		if (Shift.Contains(ssAlt)) // precise if alt is pressed
		{
			sensativity *= 0.01f;
		}

		// Camera oriented plane
		if (Tools->GetAxis() == etAxisUndefined)
		{
			Fmatrix base_m = EDevice.m_Camera.GetTransform();

			Fvector Y;
			Fvector X;

			X.mul(base_m.i, sensativity * UI->m_DeltaCpH.x);
			Y.mul(base_m.j, -sensativity * UI->m_DeltaCpH.y);

			X.add(Y);

			amount = X;
		}
		else // object space planes
		{
			amount.mul(m_MovingXVector, sensativity * UI->m_DeltaCpH.x);
			amount.mad(amount, m_MovingYVector, -sensativity * UI->m_DeltaCpH.y);
		}

        if(Tools->GetSettings(etfMSnap))
        {
        	CHECK_SNAP(m_MovingReminder.x,amount.x,Tools->m_MoveSnap);
        	CHECK_SNAP(m_MovingReminder.y,amount.y,Tools->m_MoveSnap);
        	CHECK_SNAP(m_MovingReminder.z,amount.z,Tools->m_MoveSnap);
        }

        return (amount.square_magnitude() > EPS_S);
	}
    return false;
}


void __fastcall TUI_CustomControl::MovingProcess(TShiftState _Shift)
{
	if (activeMouseObject_)
	{
		ByAxisMoveProcess(_Shift);
		return;
	}

	Fvector amount;
	if (DefaultMovingProcess(_Shift, amount))
    {
        ObjectList lst;
        if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(), 1, 1, 0))
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
				(*_F)->Move(amount);
    }
}

bool __fastcall TUI_CustomControl::MovingEnd(TShiftState _Shift)
{
	if (activeMouseObject_)
	{
		ByAxisMoveEnd(_Shift);
		isByAxisMoving = false;
	}
	else
		isRegularMoving = false;

	Scene->UndoSave();

    return true;
}


//------------------------------------------------------------------------------------
// rotate
//------------------------------------------------------------------------------------


float angle_between2d_vectors(Fvector2& p1, Fvector2& p2)
{
	float x1 = p1.x, y1 = p1.y;
	float x2 = p2.x, y2 = p2.y;
	
	float res = atan2f(y1 - y2, x1 - x2) / PI * PI;
	
	res = (res < 0) ? res + (PI * 2) : res; // clamp
	
	return res;
}

bool __fastcall TUI_CustomControl::RotateStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();

    if(Shift==ssRBOnly)
	{
		ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID);
		return false;
	}
	
    if(Scene->SelectionCount(true,cls)==0)
		return false;
	
	m_RotateVector.set(0,0,0);
	
	if (etAxisX==Tools->GetAxis()) 		m_RotateVector.set(1,0,0);
	else if (etAxisY==Tools->GetAxis()) m_RotateVector.set(0,1,0);
	else if (etAxisZ==Tools->GetAxis()) m_RotateVector.set(0,0,1);

	// Rotate to cursor position
	if (Shift.Contains(ssCtrl))
	{
		ObjectList lst;
		if (Scene->GetQueryObjects(lst, LTools->CurrentClassID(), 1, 1, 0))
		{
			if (lst.size() == 1)
			{
				Fvector p, n;
				UI->IR_GetMousePosReal(EDevice.m_hRenderWnd, UI->m_CurrentCp);
				EDevice.m_Camera.MouseRayFromPoint(UI->m_CurrentRStart, UI->m_CurrentRNorm, UI->m_CurrentCp);

				if (LUI->PickGround(p, UI->m_CurrentRStart, UI->m_CurrentRNorm, 1, &n))
				{
					for (ObjectIt _F = lst.begin(); _F != lst.end(); _F++)
					{
						Fvector2 op = {(*_F)->PPosition.x, (*_F)->PPosition.z};
						Fvector2 curp = {p.x, p.z};
						
						float angle = angle_between2d_vectors(op, curp) + (PI / 2.f); // adjust to make mesh Z axis face the cursor position, since all meshes are faced to Z axis
					
						if(angle > PI * 2.f) // clamp
							angle -= PI * 2.f;
						
						Fvector r = (*_F)->GetRotation();
						
						r.y = 1 * angle; // only XZ axises(top view)
						
						(*_F)->NumSetRotation(r);
					}
					
					Scene->UndoSave();
				}
			}
		}

		return true;
	}
	
	m_fRotateSnapAngle = 0;
	
    return true;
}

void __fastcall TUI_CustomControl::RotateProcess(TShiftState _Shift)
{
    if (_Shift.Contains(ssLeft))
    {
		float sensativity = UI->m_MouseSR;
		
		if (_Shift.Contains(ssAlt)) // precise if alt is pressed
		{
			sensativity *= 0.01f;
		}
		
        float amount = -UI->m_DeltaCpH.x * sensativity;

        if( Tools->GetSettings(etfASnap) ) CHECK_SNAP(m_fRotateSnapAngle,amount,Tools->m_RotateSnapAngle);

		// rotate localy or globaly
        ObjectList lst;
        if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(),1,1,0))
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
			{
                if( Tools->GetSettings(etfCSParent))
                {
                    (*_F)->RotateParent(m_RotateVector, amount);
                }
				else
				{
                    (*_F)->RotateLocal(m_RotateVector, amount);
				}
			}
    }
}

bool __fastcall TUI_CustomControl::RotateEnd(TShiftState _Shift)
{
	Scene->UndoSave();
    return true;
}


//------------------------------------------------------------------------------
// scale
//------------------------------------------------------------------------------


bool __fastcall TUI_CustomControl::ScaleStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();
    if(Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return false;}
    if(Scene->SelectionCount(true,cls)==0) return false;
	return true;
}

void __fastcall TUI_CustomControl::ScaleProcess(TShiftState _Shift)
{
	float sensativity = UI->m_MouseSS;
		
	if (_Shift.Contains(ssAlt)) // precise if alt is pressed
	{
		sensativity *= 0.01f;
	}
		
	float dy = UI->m_DeltaCpH.x * sensativity;

	if (dy>1.f)
		dy = 1.f;
	else if	(dy<-1.f)
		dy = -1.f;

	Fvector amount;
	amount.set( dy, dy, dy );

	// Scale only selected axises, if non uniform scaling is enabled
    if (Tools->GetSettings(etfNUScale))
	{
		if (!(etAxisUndefined == Tools->GetAxis()) && !(etAxisX == Tools->GetAxis()) && !(etAxisZX == Tools->GetAxis()) && !(etAxisYX == Tools->GetAxis()))
			amount.x = 0.f;

		if (!(etAxisUndefined == Tools->GetAxis()) && !(etAxisZ == Tools->GetAxis()) && !(etAxisZX == Tools->GetAxis()) && !(etAxisYZ == Tools->GetAxis()))
			amount.z = 0.f;

		if (!(etAxisUndefined == Tools->GetAxis()) && !(etAxisY == Tools->GetAxis()) && !(etAxisYX == Tools->GetAxis()) && !(etAxisYZ == Tools->GetAxis()))
			amount.y = 0.f;
    }

    ObjectList lst;

	if (Scene->GetQueryObjects(lst, LTools->CurrentClassID(), 1, 1, 0))
		for (ObjectIt _F = lst.begin(); _F != lst.end(); _F++)
			(*_F)->Scale(amount);
}

bool __fastcall TUI_CustomControl::ScaleEnd(TShiftState _Shift)
{
	Scene->UndoSave();
    return true;
}


//------------------------------------------------------------------------------
// By Axis movement
//------------------------------------------------------------------------------


bool __fastcall TUI_CustomControl::ByAxisMoveStart(TShiftState _Shift)
{
	return true;
}

bool __fastcall TUI_CustomControl::ByAxisMoveEnd(TShiftState _Shift)
{
	return true;
}

void __fastcall TUI_CustomControl::ByAxisMoveProcess(TShiftState _Shift)
{
	float sensativity = UI->m_MouseSM;

	if (_Shift.Contains(ssShift)) // fast
	{
		sensativity *= 1.5f;
	}
	else if (_Shift.Contains(ssAlt)) // precise
	{
		sensativity *= 0.01f;
	}
	else if (_Shift.Contains(ssCtrl)) // very precise
	{
		sensativity *= 0.001f;
	}

	float x_shift = -sensativity * UI->m_DeltaCpH.x;
	float y_shift = -sensativity * UI->m_DeltaCpH.y;

	if (activeMouseObject_)
		activeMouseObject_->ManageMovement(x_shift, y_shift);
}


//------------------------------------------------------------------------------
// Handle mesh preview controlls. 
//------------------------------------------------------------------------------

extern Fmatrix previewRotation_;

// Scaling preview window mesh
void TUI_CustomControl::ToolsMouseWheel(TShiftState _Shift)
{
}

void TUI_CustomControl::ToolsMouseWheelUp(TShiftState _Shift)
{
	if ((previewObjectEnabled_ &&  UI->m_CurrentCp.x > iFloor(UI->GetRealWidth() - PREVIEW_W_W) && UI->m_CurrentCp.y > iFloor(UI->GetRealHeight() - PREVIEW_H_H)))
	{
		previewScale_ /= 1.2f;
		UI->RedrawScene();
	}
}

void TUI_CustomControl::ToolsMouseWheelDown(TShiftState _Shift)
{
	if ((previewObjectEnabled_ &&  UI->m_CurrentCp.x > iFloor(UI->GetRealWidth() - PREVIEW_W_W) && UI->m_CurrentCp.y > iFloor(UI->GetRealHeight() - PREVIEW_H_H)))
	{
		previewScale_ *= 1.2f;
		UI->RedrawScene();
	}
}

// Rotating preview window mesh
bool __fastcall TUI_CustomControl::PreviewMStart(TShiftState _Shift)
{ 
	return true;
}

void __fastcall TUI_CustomControl::PreviewMMove(TShiftState _Shift)
{

	float sensativity = UI->m_MouseSM;

	float x_shift = -sensativity * UI->m_DeltaCpH.x;
	float y_shift = -sensativity * UI->m_DeltaCpH.y;

	Fmatrix R;
	Fmatrix R2;
	
	if (Tools->GetAxis() == etAxisZ) // rotate X or Z, depending on settings
		R2.rotateZ(x_shift);
	else
		R2.rotateX(x_shift);

	R.rotateY(y_shift);

	previewRotation_.mulB_44(R);
	previewRotation_.mulB_44(R2);
}

bool __fastcall TUI_CustomControl::PreviewMEnd(TShiftState _Shift)
{
        ShowCursor(TRUE);

	return true;
}


