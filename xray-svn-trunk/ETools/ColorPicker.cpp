//---------------------------------------------------------------------------
#include "stdafx.h"
#include "ETools.h"
#include "Commdlg.h"

COLORREF cust_colors[16] = {0};
#pragma todo("Move useWinAPIcolorPicker_ to SDK prefferences")
bool useWinAPIcolorPicker_ = false;

//Fluid Studios ColorPicker Function
extern "C"{
	__declspec(dllimport) bool WINAPI FSColorPickerDoModal(unsigned int * currentColor, const bool currentColorIsDefault, unsigned int * originalColor, const bool originalColorIsDefault, const int initialExpansionState);
};

namespace ETOOLS
{
	ETOOLS_API bool __stdcall SelectColor(u32* currentcolor, bool no_alpha, bool B_G_R)
	{
		VERIFY(currentcolor);

		if (useWinAPIcolorPicker_)
		{
			HWND return_to__Window = GetActiveWindow();

			u32 working_color = B_G_R ? *currentcolor : rgb2bgr(*currentcolor); // Неформалы епаные

			cust_colors[16] = working_color; // Keep Before color copy in preset list

			CHOOSECOLOR color_struct = { 0 }; // Color input parametres for Windows Color Choose

			color_struct.hwndOwner = return_to__Window;
			color_struct.lStructSize = sizeof(color_struct);
			color_struct.lpCustColors = cust_colors;
			color_struct.Flags = CC_RGBINIT | CC_FULLOPEN;

			color_struct.rgbResult = working_color;

			if (ChooseColor(&color_struct)) // Run Windows color picker
			{
				*currentcolor = B_G_R ? color_struct.rgbResult : bgr2rgb(color_struct.rgbResult);
				return true;
			}
			return false;
		}
		else
		{
			// Add alpha color for correct FS Color Picker target/comparing colors view (Since we send a color without an alpha chanel)

			u32 working_color = (B_G_R ? rgb2bgr(*currentcolor) : *currentcolor);

			if (no_alpha) // Add alpha color for correct FS Color Picker target/comparing colors view (Since we send a color without an alpha chanel)
				working_color = color_rgba(color_get_R(working_color), color_get_G(working_color), color_get_B(working_color), 255);

			u32 comparing_color = working_color;
#ifdef _WIN64
			u32 resultt = 0;
#else
			u32 resultt = FSColorPickerDoModal(&working_color, false, &comparing_color, false, 0); // Run FS Color Picker
#endif
			if (resultt == 1)
			{
				*currentcolor = (B_G_R ? bgr2rgb(working_color) : working_color); // revert order, if needed

				if (no_alpha)
					*currentcolor = color_rgba(color_get_R(*currentcolor), color_get_G(*currentcolor), color_get_B(*currentcolor), 0); // remove alpha if needed

				return true;
			}

			return false;
		}
	}
}