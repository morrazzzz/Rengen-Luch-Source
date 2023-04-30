#include "stdafx.h"
#pragma hdrstop
#include "splash.h"
#include "../ECore/Editor/LogForm.h"
#include "../ECore/Editor/EditMesh.h"
#include "main.h"
#include "scene.h"
#include "UI_LevelMain.h"
#include "UI_LevelTools.h"


USEFORM("BottomBar.cpp", fraBottomBar); /* TFrame: File Type */
USEFORM("main.cpp", frmMain);
USEFORM("TopBar.cpp", fraTopBar); /* TFrame: File Type */
USEFORM("DOOneColor.cpp", frmOneColor);
USEFORM("DOShuffle.cpp", frmDOShuffle);
USEFORM("EditLibrary.cpp", frmEditLibrary);
USEFORM("EditLightAnim.cpp", frmEditLightAnim);
USEFORM("FrameAIMap.cpp", fraAIMap);
USEFORM("FrameDetObj.cpp", fraDetailObject);
USEFORM("FrameGroup.cpp", fraGroup);
USEFORM("FrameLight.cpp", fraLight);
USEFORM("FrameObject.cpp", fraObject);
USEFORM("FramePortal.cpp", fraPortal);
USEFORM("FramePS.cpp", fraPS);
USEFORM("FrameSector.cpp", fraSector);
USEFORM("FrameShape.cpp", fraShape);
USEFORM("FrameSpawn.cpp", fraSpawn);
USEFORM("FrameWayPoint.cpp", fraWayPoint);
USEFORM("LeftBar.cpp", fraLeftBar); /* TFrame: File Type */
USEFORM("ObjectList.cpp", frmObjectList);
USEFORM("previewimage.cpp", frmPreviewImage);
USEFORM("PropertiesEObject.cpp", frmPropertiesEObject);
USEFORM("Splash.cpp", frmSplash);
USEFORM("FrmDBXpacker.cpp", DB_packer);
USEFORM("Edit\AppendObjectInfoForm.cpp", frmAppendObjectInfo);
USEFORM("RightForm.cpp", frmRight);

TfrmSplash* startupSplashForm_;

WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
        if (!Application->Handle)
		{ 
            Application->CreateHandle	(); 
            Application->Icon->Handle 	= LoadIcon(MainInstance, "MAINICON");
			Application->Title 			= "Loading...";
        } 
		
		frmSplash 				= xr_new<TfrmSplash>((TComponent*)0);
		frmSplash->Show			();
		frmSplash->Repaint		();
		startupSplashForm_ = frmSplash;
		
		frmSplash->SetStatus	("Core initializing");

		Core._initialize		("level_la", ELogCallback);

        Application->Initialize	();

		// startup create
		frmSplash->SetStatus	("Creating Edit Modes Environments");

		Tools					= xr_new<CLevelTools>();
		
		frmSplash->SetStatus	("Creating UI handler");
		
        UI						= xr_new<CLevelMain>();
		
        UI->RegisterCommands	();
		
		frmSplash->SetStatus	("Creating Scene Base");
		
		Scene					= xr_new<EScene>();
		
		frmSplash->SetStatus	("Setting up supportive windows");
		
		Application->Title 		= UI->EditorDesc();
		
		frmSplash->SetStatus	("Creating Log");
		
		TfrmLog::CreateLog		();

		frmSplash->SetStatus	("Creating Main Window");
		
		Application->CreateForm(__classid(TfrmMain), &frmMain);
		
		frmSplash->SetStatus	("Creating Right Form Window");
		
		Application->CreateForm(__classid(TfrmRight), &frmRight);
		
		frmMain->SetHInst(hInst);

		if (psDeviceFlags.is(rsAutoCloseLog))
		{
			TfrmLog::ShowLog();
		}
		
		xr_delete(frmSplash);
		startupSplashForm_ = NULL;
		
		// start frames loop
		Application->Run		();


		
		// if frames loop exited - destroy app
		TfrmLog::DestroyLog		();

		UI->ClearCommands		();
		xr_delete				(Scene);
		xr_delete				(Tools);
		xr_delete				(UI);

		Core._destroy			();
		
		TerminateProcess(GetCurrentProcess(), 1); // sometimes editor does not want to completely exit. This resolves the problem

    return 0;
}
