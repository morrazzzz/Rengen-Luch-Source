#ifndef __XR_INPUT__
#define __XR_INPUT__

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class	ENGINE_API				IInputReceiver;

//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
//�������� ������
const int mouse_device_key		= 1;
const int keyboard_device_key	= 2;
const int all_device_key		= mouse_device_key | keyboard_device_key;
const int default_key			= mouse_device_key | keyboard_device_key ;

class ENGINE_API CInput
#ifndef M_BORLAND
	:
	public pureFrame,
	public pureAppActivate,
	public pureAppDeactivate
#endif
{
public:
	enum {
		COUNT_MOUSE_BUTTONS			= 8,
		COUNT_MOUSE_AXIS			= 3,
		COUNT_KB_BUTTONS			= 256
	};
	struct sxr_mouse
	{
		DIDEVCAPS					capabilities;
		DIDEVICEINSTANCE			deviceInfo;
		DIDEVICEOBJECTINSTANCE		objectInfo;
		u32							mouse_dt;
	};
	struct sxr_key
	{
		DIDEVCAPS					capabilities;
		DIDEVICEINSTANCE			deviceInfo;
		DIDEVICEOBJECTINSTANCE		objectInfo;
	};
private:
	LPDIRECTINPUT8				pDI;			// The DInput object
	LPDIRECTINPUTDEVICE8		pMouse;			// The DIDevice7 interface
	LPDIRECTINPUTDEVICE8		pKeyboard;		// The DIDevice7 interface
	//----------------------
	u32							timeStamp	[COUNT_MOUSE_AXIS];
	u32							timeSave	[COUNT_MOUSE_AXIS];
	int 						offs		[COUNT_MOUSE_AXIS];
	BOOL						mouseState	[COUNT_MOUSE_BUTTONS];

	//----------------------
	BOOL						KBState		[COUNT_KB_BUTTONS];

	HRESULT						CreateInputDevice(	LPDIRECTINPUTDEVICE8* device, GUID guidDevice,
													const DIDATAFORMAT* pdidDataFormat, u32 dwFlags,
													u32 buf_size );

//	xr_stack<IInputReceiver*>	cbStack;
	xr_vector<IInputReceiver*>	cbStack;

	void						MouseUpdate					( );
	void						KeyUpdate					( );

public:
	sxr_mouse					mouse_property;
	sxr_key						key_property;
	u32							dwCurTime;

	void						SetAllAcquire				( BOOL bAcquire = TRUE );
	void						SetMouseAcquire				( BOOL bAcquire );
	void						SetKBDAcquire				( BOOL bAcquire );

	void						iCapture					( IInputReceiver *pc );
	void						iRelease					( IInputReceiver *pc );
	BOOL						iGetAsyncKeyState			( int dik );
	BOOL						iGetAsyncBtnState			( int btn );
	void						iGetLastMouseDelta			( Ivector2& p )	{ p.set(offs[0],offs[1]); }

	CInput						( BOOL bExclusive = true, int deviceForInit = default_key);
	~CInput						( );

	virtual void				OnFrame						();
	virtual void				OnAppActivate				();
	virtual void				OnAppDeactivate				();

	IInputReceiver*				CurrentIR					();
	
	// lost alpha start
	int GetLastKeyPressed() { return (int)m_last_key_pressed; }
	int GetLastKeyReleased() { return (int)m_last_key_released; }
	int GetButtonCount() { return COUNT_KB_BUTTONS; }

public:
			void				exclusive_mouse				(const bool &exclusive);
			void				exclusive_keyboard			(const bool &exclusive);
			void				unacquire_mouse();
			void				unacquire_keyboard();
			void				acquire_mouse				(const bool &exclusive);
			void				acquire_keyboard			(const bool &exclusive);

			IC		bool		our_app_has_mouse_priority();

			bool				get_dik_name				(int dik, LPSTR dest, int dest_sz);
//lost alpha start
private:
	u32 m_last_key_pressed;
	u32 m_last_key_released;
};

extern ENGINE_API CInput *		pInput;

#endif //__XR_INPUT__
