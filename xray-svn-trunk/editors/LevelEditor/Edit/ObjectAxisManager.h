#ifndef AXIS_Manager_included
#define AXIS_Manager_included

class CCustomObject;

class ECORE_API AxisManager
{
	// For mouse capturing point indicator
	Fvector indicatorX_pos_;
	Fvector indicatorY_pos_;
	Fvector indicatorZ_pos_;

	float indicatorX_size_;
	float indicatorY_size_;
	float indicatorZ_size_;

	bool indicatorX_captured_;
	bool indicatorY_captured_;
	bool indicatorZ_captured_;

	// Update Possitions of mouse capturing poits for correct cammera relative display
	void UpdateXPos						(float indicator_offset_f);
	void UpdateYPos						(float indicator_offset_f);
	void UpdateZPos						(float indicator_offset_f);

	// 0 is X, 1 is Y, 2 is Z
	u8 capturedAxis_;

	// Owner of Axis
	CCustomObject* owningObject_;
public:
	AxisManager();
	virtual ~AxisManager();

	void MovementAxisUpdate();
	bool TryCaptureAxis					(Fvector indicator_pos, float indicator_size);
	void DrawAxis();
	// The poits for which developer can pull to move object
	void DrawAxisCapturePoints();

	void ManageMovement					(float x_shift, float y_shift);

	void SetOwningObject				(CCustomObject* object)		{ owningObject_ = object; };
	CCustomObject* GetOwningObject() { return owningObject_; };
};

#endif