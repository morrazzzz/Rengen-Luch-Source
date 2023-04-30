#include "stdafx.h"

#include "ObjectAxisManager.h"

#include "customobject.h"

#include "../ECore/Editor/ui_main.h"
#include "UI_LevelMain.h"
#include "../../xr_3da/xrRender/D3DUtils.h"
#include "ESceneCustomOTools.h"
#include "Scene.h"

#include "UI_LevelTools.h"

// Global Vars
CCustomObject* activeMouseObject_ = NULL;

extern bool isRegularMoving;
extern bool isByAxisMoving;
extern bool needRecalcBeforeRedraw_;

AxisManager::AxisManager()
{
	indicatorX_pos_.set(0, 0, 0);
	indicatorY_pos_.set(0, 0, 0);
	indicatorZ_pos_.set(0, 0, 0);

	indicatorX_size_ = 1.f;
	indicatorY_size_ = 1.f;
	indicatorZ_size_ = 1.f;

	indicatorX_captured_ = 0;
	indicatorY_captured_ = 0;
	indicatorZ_captured_ = 0;

	capturedAxis_ = 0;
}

AxisManager::~AxisManager()
{

}

void AxisManager::MovementAxisUpdate()
{
	// Do only if option is on AND object is selected AND current edit mode is object's edit mode AND action tool is "Move"
	if (GetOwningObject()->Selected() && GetOwningObject()->ClassID == LTools->CurrentClassID() && Tools->GetAction() == etaMove)
	{
		Fmatrix& base_m = GetOwningObject()->FTransformRP;

		// Find coordinate offset depending on camera
		float adapted = base_m.c.x * EDevice.mFullTransform._14 + base_m.c.y * EDevice.mFullTransform._24 + base_m.c.z * EDevice.mFullTransform._34 + EDevice.mFullTransform._44;
		float indicator_offset_f = adapted * 0.1f;

		indicatorX_size_ = indicatorY_size_ = indicatorZ_size_ = indicator_offset_f / 10.f; // capture indicators sizes

		UpdateXPos(indicator_offset_f);
		UpdateYPos(indicator_offset_f);
		UpdateZPos(indicator_offset_f);

		if (activeMouseObject_)
		{
			if (!isByAxisMoving && activeMouseObject_ == GetOwningObject()) // Try uncapture if not in process of "by Axis" movement
				if (capturedAxis_ == 0)
				{
					if (!TryCaptureAxis(indicatorX_pos_, indicatorX_size_))
					{
						capturedAxis_ = 99;

						UI->GetD3DWindow()->Cursor = crSizeAll;
						indicatorX_captured_ = 0;
						activeMouseObject_ = NULL;

						UI->RedrawScene();
					}
				}
				else if (capturedAxis_ == 1)
				{
					if (!TryCaptureAxis(indicatorY_pos_, indicatorY_size_))
					{
						capturedAxis_ = 99;

						UI->GetD3DWindow()->Cursor = crSizeAll;
						indicatorY_captured_ = 0;
						activeMouseObject_ = NULL;

						UI->RedrawScene();
					}
				}
				else if (capturedAxis_ == 2)
				{
					if (!TryCaptureAxis(indicatorZ_pos_, indicatorZ_size_))
					{
						capturedAxis_ = 99;

						UI->GetD3DWindow()->Cursor = crSizeAll;
						indicatorZ_captured_ = 0;
						activeMouseObject_ = NULL;

						UI->RedrawScene();
					}
				}

		}
		else if (!isRegularMoving) // Try capture drag point if we are not already moving using regular tool
		{
			if (TryCaptureAxis(indicatorX_pos_, indicatorX_size_))
			{
				capturedAxis_ = 0;

				UI->GetD3DWindow()->Cursor = crCross;
				indicatorX_captured_ = 1;
				activeMouseObject_ = GetOwningObject();

				UI->RedrawScene();
			}
			else if (TryCaptureAxis(indicatorY_pos_, indicatorY_size_))
			{
				capturedAxis_ = 1;

				UI->GetD3DWindow()->Cursor = crCross;
				indicatorY_captured_ = 1;
				activeMouseObject_ = GetOwningObject();

				UI->RedrawScene();
			}
			else if (TryCaptureAxis(indicatorZ_pos_, indicatorZ_size_))
			{
				capturedAxis_ = 2;

				UI->GetD3DWindow()->Cursor = crCross;
				indicatorZ_captured_ = 1;
				activeMouseObject_ = GetOwningObject();

				UI->RedrawScene();
			}
		}
	}
}

bool AxisManager::TryCaptureAxis(Fvector indicator_pos, float indicator_size)
{
	// Ray cast cursor to capture indicator
	Fvector ray2;

	Fvector start = UI->m_CurrentRStart;
	Fvector direction = UI->m_CurrentRNorm;

	ray2.sub(indicator_pos, start);
	float d = ray2.dotproduct(direction);

	if (d > 0)
	{
		float d2 = ray2.magnitude();

		if (((d2 * d2 - d * d) < (indicator_size * indicator_size)) && (d > indicator_size))
		{
			float range = UI->ZFar();

			if (d < range)
			{
				return true;
			}
		}
	}

	return false;
}

void AxisManager::UpdateXPos(float indicator_offset_f)
{
	//Get Self X Using matrix of Object

	Fmatrix base_m = GetOwningObject()->FTransformRP;  // Local transform orientation

	if (Tools->GetSettings(etfCSParent)) // Global transform orientation
		base_m = GetOwningObject()->FTransformP;

	Fvector xx;

	xx.mul(base_m.i, indicator_offset_f);
	xx.add(base_m.c);

	indicatorX_pos_ = xx;
}

void AxisManager::UpdateYPos(float indicator_offset_f)
{
	//Get Self Y Using matrix of Object

	Fmatrix base_m = GetOwningObject()->FTransformRP;  // Local transform orientation

	if (Tools->GetSettings(etfCSParent)) // Global transform orientation
		base_m = GetOwningObject()->FTransformP;

	Fvector yy;

	yy.mul(base_m.j, indicator_offset_f);
	yy.add(base_m.c);

	indicatorY_pos_ = yy;
}

void AxisManager::UpdateZPos(float indicator_offset_f)
{
	//Get Self Z Using matrix of Object
	Fmatrix base_m = GetOwningObject()->FTransformRP;  // Local transform orientation

	if (Tools->GetSettings(etfCSParent)) // Global transform orientation
		base_m = GetOwningObject()->FTransformP;

	Fvector zz;

	zz.mul(base_m.k, indicator_offset_f);
	zz.add(base_m.c);

	indicatorZ_pos_ = zz;
}

void AxisManager::DrawAxis()
{
	if (GetOwningObject()->Selected() && GetOwningObject()->ClassID == LTools->CurrentClassID())
	{
		if (needRecalcBeforeRedraw_) // Set in some cases like when tool change has happened
			MovementAxisUpdate();

		if (Tools->GetAction() == etaMove)
		{
			DrawAxisCapturePoints();

			if (Tools->GetSettings(etfCSParent))
				DU_impl.DrawObjectAxis(GetOwningObject()->FTransformP, 0.1f, GetOwningObject()->Selected()); // Global transform orientation
			else
				DU_impl.DrawObjectAxis(GetOwningObject()->FTransformRP, 0.1f, GetOwningObject()->Selected()); // Local transform orientation

		}
		else if (EPrefs->object_flags.is(epoDrawPivot)) // Draw Local transform orientation Axis if settings say so
		{
			DU_impl.DrawObjectAxis(GetOwningObject()->FTransformRP, 0.1f, GetOwningObject()->Selected());
		}
	}
}

void AxisManager::DrawAxisCapturePoints()
{

	Fmatrix object_matrix;

	object_matrix = GetOwningObject()->FTransformRP;

	EDevice.SetRS(D3DRS_ZENABLE, FALSE);

	u32 color_x;
	u32 color_y;
	u32 color_z;

	if (!isRegularMoving)
	{
		color_x = (indicatorX_captured_ == 0) ? 0xffFF1000 : (isByAxisMoving) ? 0xFFFF8819 : 0xffFFF542;
		color_y = (indicatorY_captured_ == 0) ? 0xff00CC00 : (isByAxisMoving) ? 0xFFFF8819 : 0xffFFF542;
		color_z = (indicatorZ_captured_ == 0) ? 0xFF002AFF : (isByAxisMoving) ? 0xFFFF8819 : 0xFFFFF542;
	}
	else
	{
		color_x = color_y = color_z = 0xFFAD9F9C; // Mark with inactive state color
	}

	DU_impl.DrawSphere(object_matrix, indicatorX_pos_, indicatorX_size_, color_x, 0xff404040, TRUE, TRUE, false);

	DU_impl.DrawSphere(object_matrix, indicatorY_pos_, indicatorY_size_, color_y, 0xff404040, TRUE, TRUE, false);

	DU_impl.DrawSphere(object_matrix, indicatorZ_pos_, indicatorZ_size_, color_z, 0xff404040, TRUE, TRUE, false);

	EDevice.SetRS(D3DRS_ZENABLE, TRUE);
}

void AxisManager::ManageMovement(float x_shift, float y_shift)
{
	Fmatrix base_m = GetOwningObject()->FTransformRP;

	Fvector position_change;

	if (Tools->GetSettings(etfCSParent)) // Global transform orientation
	{
		if (capturedAxis_ == 0)
		{
			position_change.x = x_shift;
		}
		else if (capturedAxis_ == 1)
		{
			position_change.y = x_shift;
		}
		else if (capturedAxis_ == 2)
		{
			position_change.z = x_shift;
		}
	}
	else // Local transform orientation
	{
		if (capturedAxis_ == 0)
		{
			position_change.mul(base_m.i, x_shift);
		}
		else if (capturedAxis_ == 1)
		{
			position_change.mul(base_m.j, x_shift);
		}
		else if (capturedAxis_ == 2)
		{
			position_change.mul(base_m.k, x_shift);
		}
	}

	if (!(Tools->GetAxis() == etAxisUndefined) && !(etAxisX == Tools->GetAxis()) && !(etAxisZX == Tools->GetAxis()) && !(etAxisYX == Tools->GetAxis()))
		position_change.x = 0.f;

	if (!(Tools->GetAxis() == etAxisUndefined) && !(etAxisZ == Tools->GetAxis()) && !(etAxisZX == Tools->GetAxis()) && !(etAxisYZ == Tools->GetAxis()))
		position_change.z = 0.f;

	if (!(Tools->GetAxis() == etAxisUndefined) && !(etAxisY == Tools->GetAxis()) && !(etAxisYX == Tools->GetAxis()) && !(etAxisYZ == Tools->GetAxis()))
		position_change.y = 0.f;

	// Prevent undefined moving amount
	if (position_change.x > -65535.f && position_change.y > -65535.f && position_change.z > -65535.f && position_change.x < 65535.f && position_change.y < 65535.f && position_change.z < 65535.f)
		activeMouseObject_->Move(position_change);

	GetOwningObject()->OnUpdateTransform();
}