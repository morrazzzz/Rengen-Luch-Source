#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"
#include "x_ray.h"
#include "xr_ioconsole.h"
#include "xr_ioc_cmd.h"
#include "cameramanager.h"
#include "environment.h"
#include "xr_input.h"
#include "CustomHUD.h"
#include "../Include/xrRender/RenderDeviceRender.h"
#include "xr_object.h"
#include "threadtest.h"

xr_token*							vid_quality_token = NULL;

ENGINE_API BOOL mt_LTracking_ = FALSE;
ENGINE_API BOOL mt_dynTextureLoad_ = TRUE;


xr_token							vid_bpp_token							[ ]={
	{ "16",							16											},
	{ "32",							32											},
	{ 0,							0											}
};

void IConsole_Command::add_to_LRU(shared_str const& arg)
{
	if (arg.size() == 0 || bEmptyArgsHandled)
	{
		return;
	}
	
	bool dup = (std::find( m_LRU.begin(), m_LRU.end(), arg ) != m_LRU.end());

	if (!dup)
	{
		m_LRU.push_back(arg);

		if (m_LRU.size() > LRU_MAX_COUNT)
		{
			m_LRU.erase(m_LRU.begin());
		}
	}
}

void  IConsole_Command::add_LRU_to_tips(vecTips& tips)
{
	vecLRU::reverse_iterator	it_rb = m_LRU.rbegin();
	vecLRU::reverse_iterator	it_re = m_LRU.rend();

	for (; it_rb != it_re; ++it_rb)
	{
		tips.push_back((*it_rb));
	}
}

class CCC_Quit : public IConsole_Command
{
public:
	CCC_Quit(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {

		string256	S;
		S[0] = 0;
		sscanf(args, "%s", S);
		if (!xr_strlen(S))
		{
			Msg("!Fuck You! Give quit reason");
			return;
		}

//		TerminateProcess(GetCurrentProcess(),0);
		Console->Hide();
		string4096 disconect_reason = "Console : Quit";

		Engine.Event.Defer("KERNEL:disconnect", u64(xr_strdup(disconect_reason)), 0);
		Engine.Event.Defer("KERNEL:quit", u64(xr_strdup(S)), 0);
	}
};

#ifdef DEBUG_MEMORY_MANAGER
class CCC_MemStat : public IConsole_Command
{
public:
	CCC_MemStat(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		string_path fn;
		if (args&&args[0])	xr_sprintf	(fn,sizeof(fn),"%s.dump",args);
		else				strcpy_s_s	(fn,sizeof(fn),"x:\\$memory$.dump");
		Memory.mem_statistic				(fn);
//		g_pStringContainer->dump			();
//		g_pSharedMemoryContainer->dump		();
	}
};
#endif

#ifdef DEBUG_MEMORY_MANAGER
class CCC_DbgMemCheck : public IConsole_Command
{
public:
	CCC_DbgMemCheck(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { if (Memory.debug_mode){ Memory.dbg_check();}else{Msg("~ Run with -mem_debug options.");} }
};
#endif

class CCC_DbgStrCheck : public IConsole_Command
{
public:
	CCC_DbgStrCheck(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->verify(); }
};

class CCC_DbgStrDump : public IConsole_Command
{
public:
	CCC_DbgStrDump(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->dump();}
};

class CCC_MotionsStat : public IConsole_Command
{
public:
	CCC_MotionsStat(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		//g_pMotionsContainer->dump();
		//	TODO: move this console commant into renderer
	}
};
class CCC_TexturesStat : public IConsole_Command
{
public:
	CCC_TexturesStat(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		//Device.Resources->_DumpMemoryUsage();
		//	TODO: move this console commant into renderer
		//VERIFY(0);
	}
};

class CCC_E_Dump : public IConsole_Command
{
public:
	CCC_E_Dump(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		Engine.Event.Dump();
	}
};
class CCC_E_Signal : public IConsole_Command
{
public:
	CCC_E_Signal(LPCSTR N) : IConsole_Command(N)  { };
	virtual void Execute(LPCSTR args) {
		char	Event[128],Param[128];
		Event[0]=0; Param[0]=0;
		sscanf	(args,"%[^,],%s",Event,Param);
		Engine.Event.Signal	(Event,(u64)Param);
	}
};

class CCC_Help : public IConsole_Command
{
public:
	CCC_Help(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		Log("- --- Command listing: start ---");
		CConsole::vecCMD_IT it;
		for (it=Console->Commands.begin(); it!=Console->Commands.end(); it++)
		{
			IConsole_Command &C = *(it->second);
			TStatus _S; C.Status(_S);
			TInfo	_I;	C.Info	(_I);
			
			Msg("%-20s (%-10s) --- %s",	C.Name(), _S, _I);
		}
		Log("Key: Ctrl + A         === Select all ");
		Log("Key: Ctrl + C         === Copy to clipboard ");
		Log("Key: Ctrl + V         === Paste from clipboard ");
		Log("Key: Ctrl + X         === Cut to clipboard ");
		Log("Key: Ctrl + Z         === Undo ");
		Log("Key: Ctrl + Insert    === Copy to clipboard ");
		Log("Key: Shift + Insert   === Paste from clipboard ");
		Log("Key: Shift + Delete   === Cut to clipboard ");
		Log("Key: Insert           === Toggle mode <Insert> ");
		Log("Key: Back / Delete          === Delete symbol left / right ");

		Log("Key: Up   / Down            === Prev / Next command in tips list ");
		Log("Key: Ctrl + Up / Ctrl + Down === Prev / Next executing command ");
		Log("Key: Left, Right, Home, End {+Shift/+Ctrl}       === Navigation in text ");
		Log("Key: PageUp / PageDown      === Scrolling history ");
		Log("Key: Tab  / Shift + Tab     === Next / Prev possible command from list");
		Log("Key: Enter  / NumEnter      === Execute current command ");
		
		Log("- --- Command listing: end ----");
	}
};

void CrashThread(void*)
{
	R_ASSERT2(false, "Crash Thread");
}

class CCC_Crash : public IConsole_Command
{
public:
	CCC_Crash(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Msg("Testing crash from secondary thread");
		FlushLog();

		thread_spawn(CrashThread, "CrashThreadTest", 0, 0);
	}
};

class CCC_FlushLog : public IConsole_Command {
public:
	CCC_FlushLog(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR /**args/**/) {
		FlushLog();
		Msg("* Log file has been saved successfully");
	}
};

class CCC_ClearLog : public IConsole_Command {
public:
	CCC_ClearLog(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR) {
		LogFile->clear_not_free();
		FlushLog();
		Msg("* Log file has been cleaned successfully!");
	}
};

struct CCC_ReloadSystemLtx : public IConsole_Command {
	CCC_ReloadSystemLtx(LPCSTR N) : IConsole_Command(N) {
		bEmptyArgsHandled = true;
	};

	virtual void Execute(LPCSTR args) {
		string_path fname;
		FS.update_path(fname, "$game_config$", "system.ltx");
		CInifile::Destroy(pSettings);
		pSettings = xr_new <CInifile>(fname, TRUE);
		CHECK_OR_EXIT(0 != pSettings->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
		Msg("system.ltx was reloaded.");
	}
};

XRCORE_API void _dump_open_files(int mode);
class CCC_DumpOpenFiles : public IConsole_Command
{
public:
	CCC_DumpOpenFiles(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = FALSE; };
	virtual void Execute(LPCSTR args) {
		int _mode			= atoi(args);
		_dump_open_files	(_mode);
	}
};

extern void GenerateTextureLodList();

class CCC_GenTexLodList : public IConsole_Command
{
public:
	CCC_GenTexLodList(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		GenerateTextureLodList();
	}
};

class CCC_SaveCFG : public IConsole_Command
{
public:
	CCC_SaveCFG(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) 
	{
		string_path			cfg_full_name;
		xr_strcpy			(cfg_full_name, (xr_strlen(args)>0)?args:Console->ConfigFile);

		bool b_abs_name = xr_strlen(cfg_full_name)>2 && cfg_full_name[1]==':';

		if(!b_abs_name)
			FS.update_path	(cfg_full_name, "$app_data_root$", cfg_full_name);

		if (strext(cfg_full_name))	
			*strext(cfg_full_name) = 0;
		xr_strcat			(cfg_full_name,".ltx");
		
		BOOL b_allow = TRUE;
		if ( FS.exist(cfg_full_name) )
			b_allow = SetFileAttributes(cfg_full_name,FILE_ATTRIBUTE_NORMAL);

		if ( b_allow ){
			IWriter* F			= FS.w_open(cfg_full_name);
				CConsole::vecCMD_IT it;
				for (it=Console->Commands.begin(); it!=Console->Commands.end(); it++)
					it->second->Save(F);
				FS.w_close			(F);
				Msg("Config-file [%s] saved successfully",cfg_full_name);
		}else
			Msg("!Cannot store config file [%s]", cfg_full_name);
	}
};

CCC_LoadCFG::CCC_LoadCFG(LPCSTR N) : IConsole_Command(N) 
{};

void CCC_LoadCFG::Execute(LPCSTR args) 
{
		Msg("Executing config-script \"%s\"...",args);
		string_path						cfg_name;

		xr_strcpy							(cfg_name, args);
		if (strext(cfg_name))			*strext(cfg_name) = 0;
		xr_strcat							(cfg_name,".ltx");

		string_path						cfg_full_name;

		FS.update_path					(cfg_full_name, "$app_data_root$", cfg_name);
		
		if( NULL == FS.exist(cfg_full_name) )
			xr_strcpy						(cfg_full_name, cfg_name);
		
		IReader* F						= FS.r_open(cfg_full_name);
		
		string1024						str;
		if (F!=NULL) {
			while (!F->eof()) {
				F->r_string				(str,sizeof(str));
				if(allow(str))
					Console->Execute	(str);
			}
			FS.r_close(F);
			Msg("[%s] successfully loaded.",cfg_full_name);
		} else {
			Msg("! Cannot open script file [%s]",cfg_full_name);			
		}
}

CCC_LoadCFG_custom::CCC_LoadCFG_custom(LPCSTR cmd)
:CCC_LoadCFG(cmd)
{
	xr_strcpy(m_cmd, cmd);
};
bool CCC_LoadCFG_custom::allow(LPCSTR cmd)
{
	return (cmd == strstr(cmd, m_cmd) );
};

class CCC_Start : public IConsole_Command
{
	void	parse		(LPSTR dest, LPCSTR args, LPCSTR name)
	{
		dest[0]	= 0;
		if (strstr(args,name))
			sscanf(strstr(args,name)+xr_strlen(name),"(%[^)])",dest);
	}

	void	protect_Name_strlwr( LPSTR str )
	{
 		string4096	out;
		xr_strcpy( out, sizeof(out), str );
		strlwr( str );

		LPCSTR name_str = "name=";
		LPCSTR name1 = strstr( str, name_str );
		if ( !name1 || !xr_strlen( name1 ) )
		{
			return;
		}
		int begin_p = xr_strlen( str ) - xr_strlen( name1 ) + xr_strlen( name_str );
		if ( begin_p < 1 )
		{
			return;
		}

		LPCSTR name2 = strchr( name1, '/' );
		int end_p = xr_strlen( str ) - ((name2)? xr_strlen(name2) : 0);
		if ( begin_p >= end_p )
		{
			return;
		}
		for ( int i = begin_p; i < end_p;++i )
		{
			str[i] = out[i];
		}
	}
public:
	CCC_Start(LPCSTR N) : IConsole_Command(N)	{ 	  bLowerCaseArgs = false; };
	virtual void Execute(LPCSTR args)
	{
		/*		if (g_pGameLevel)	{
					Log		("! Please disconnect/unload first");
					return;
					}
					*/
		string4096	op_server, op_client, op_demo;
		op_server[0] = 0;
		op_client[0] = 0;

		parse(op_server, args, "server");	// 1. server
		parse(op_client, args, "client");	// 2. client
		parse(op_demo, args, "demo");	// 3. demo

		strlwr(op_server);
		protect_Name_strlwr(op_client);

		if (!op_client[0] && strstr(op_server, "single"))
			xr_strcpy(op_client, "localhost");

		if ((0 == xr_strlen(op_client)) && (0 == xr_strlen(op_demo)))
		{
			Log("! Can't start game without client. Arguments: '%s'.", args);
			return;
		}
		if (g_pGameLevel){
			string4096	disconect_reason = "Console : Start";
			Engine.Event.Defer("KERNEL:disconnect", u64(xr_strdup(disconect_reason)), 0);
		}
		
		if (xr_strlen(op_demo))
		{
			Engine.Event.Defer	("KERNEL:start_mp_demo",u64(xr_strdup(op_demo)),0);
		} else
		{
			Engine.Event.Defer	("KERNEL:start",u64(xr_strlen(op_server)?xr_strdup(op_server):0),u64(xr_strdup(op_client)));
		}
	}
};

class CCC_Disconnect : public IConsole_Command
{
public:
	CCC_Disconnect(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		string256	S;
		S[0] = 0;
		sscanf(args, "%s", S);
		if (!xr_strlen(S))
		{
			Msg("!Fuck You! Give disconnect reason");
			return;
		}

		Engine.Event.Defer("KERNEL:disconnect", u64(xr_strdup(S)), 0);
	}
};

class CCC_VID_Reset : public IConsole_Command
{
public:
	CCC_VID_Reset(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		if (Device.b_is_Ready) {
			Device.Reset	();
		}
	}
};
class CCC_VidMode : public CCC_Token
{
	u32		_dummy;
public :
					CCC_VidMode(LPCSTR N) : CCC_Token(N, &_dummy, NULL) { bEmptyArgsHandled = FALSE; };
	virtual void	Execute(LPCSTR args){
		u32 _w, _h;
		int cnt = sscanf		(args,"%dx%d",&_w,&_h);
		if(cnt==2){
			psCurrentVidMode[0] = _w;
			psCurrentVidMode[1] = _h;
		}else{
			Msg("! Wrong video mode [%s]", args);
			return;
		}
	}
	virtual void	Status	(TStatus& S)	
	{ 
		xr_sprintf(S,sizeof(S),"%dx%d",psCurrentVidMode[0],psCurrentVidMode[1]); 
	}
	virtual xr_token* GetToken()				{return vid_mode_token;}
	virtual void	Info	(TInfo& I)
	{	
		xr_strcpy(I,sizeof(I),"change screen resolution WxH");
	}

	virtual void	fill_tips(vecTips& tips, u32 mode)
	{
		TStatus  str, cur;
		Status( cur );

		bool res = false;
		xr_token* tok = GetToken();
		while ( tok->name && !res )
		{
			if ( !xr_strcmp( tok->name, cur ) )
			{
				xr_sprintf( str, sizeof(str), "%s  (current)", tok->name );
				tips.push_back( str );
				res = true;
			}
			tok++;
		}
		if ( !res )
		{
			tips.push_back( "---  (current)" );
		}
		tok = GetToken();
		while ( tok->name )
		{
			tips.push_back( tok->name );
			tok++;
		}
	}

};

class CCC_SND_Restart : public IConsole_Command
{
public:
	CCC_SND_Restart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		Sound->_restart();
	}
};

float	ps_gamma=1.f,ps_brightness=1.f,ps_contrast=1.f;
class CCC_Gamma : public CCC_Float
{
public:
	CCC_Gamma	(LPCSTR N, float* V) : CCC_Float(N,V,0.5f,3.0f)	{}

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute		(args);
		Device.m_pRender->setGamma(ps_gamma);
		Device.m_pRender->setBrightness(ps_brightness);
		Device.m_pRender->setContrast(ps_contrast);
	}
};


ENGINE_API BOOL r2_sun_static	= FALSE;
ENGINE_API BOOL r2_advanced_pp	= TRUE;	//	advanced post process and effects

u32	renderer_value	= 2;

class CCC_r2 : public CCC_Token
{
	typedef CCC_Token inherited;
public:
	CCC_r2(LPCSTR N) :inherited(N, &renderer_value, NULL){renderer_value=2;};
	virtual			~CCC_r2	()
	{

	}
	virtual void	Execute	(LPCSTR args)
	{
		//	vid_quality_token must be already created!
		tokens					= vid_quality_token;

		inherited::Execute(args);

		if (!xr_strcmp(args, "renderer_r4"))
		{
			psDeviceFlags.set(rsR4, true);

			return;
		}
	}

	virtual void	Save	(IWriter *F)	
	{
		//fill_render_mode_list	();
		tokens					= vid_quality_token;
		if( !strstr(Core.Params, "-r2") && !strstr(Core.Params, "-r3") && !strstr(Core.Params, "-r4")  )
		{
			inherited::Save(F);
		}
	}
	virtual xr_token* GetToken()
	{
		tokens					= vid_quality_token;
		return					inherited::GetToken();
	}

};

class CCC_soundDevice : public CCC_Token
{
	typedef CCC_Token inherited;
public:
	CCC_soundDevice(LPCSTR N) :inherited(N, &snd_device_id, NULL){};
	virtual			~CCC_soundDevice	()
	{}

	virtual void Execute(LPCSTR args)
	{
		GetToken				();
		if(!tokens)				return;
		inherited::Execute		(args);
	}

	virtual void	Status	(TStatus& S)
	{
		GetToken				();
		if(!tokens)				return;
		inherited::Status		(S);
	}

	virtual xr_token* GetToken()
	{
		tokens					= snd_devices_token;
		return inherited::GetToken();
	}

	virtual void Save(IWriter *F)	
	{
		GetToken				();
		if(!tokens)				return;
		inherited::Save			(F);
	}
};

class CCC_ExclusiveMouse : public IConsole_Command {
private:
	typedef IConsole_Command inherited;

public:
	CCC_ExclusiveMouse(LPCSTR N) : inherited(N)
	{
	}

	virtual void	Execute				(LPCSTR args)
	{
		bool		value = false;
		if (!xr_strcmp(args,"1"))
			value	= true;
		else if (!xr_strcmp(args,"0"))
			value	= false;
		else InvalidSyntax();
		
		pInput->exclusive_mouse(value);
	}
};

class CCC_ExclusiveKeyboard : public IConsole_Command {
private:
	typedef IConsole_Command inherited;

public:
	CCC_ExclusiveKeyboard(LPCSTR N) : inherited(N)
	{
	}

	virtual void	Execute(LPCSTR args)
	{
		bool		value = false;
		if (!xr_strcmp(args, "1"))
			value = true;
		else if (!xr_strcmp(args, "0"))
			value = false;
		else InvalidSyntax();

		pInput->exclusive_keyboard(value);
	}
};

class ENGINE_API CCC_HideConsole : public IConsole_Command
{
public		:
	CCC_HideConsole(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled	= true;
	}

	virtual void	Execute	(LPCSTR args)
	{
		Console->Hide	();
	}
	virtual void	Status	(TStatus& S)
	{
		S[0]			= 0;
	}
	virtual void	Info	(TInfo& I)
	{	
		xr_sprintf		(I,sizeof(I),"hide console");
	}
};

struct CF_ThreadTest : public IConsole_Command {
	CF_ThreadTest(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args) {
		RunSingleThreadTest();
	}
};


struct CCC_SwitchAllMT : public IConsole_Command {
	CCC_SwitchAllMT(LPCSTR N) : IConsole_Command(N) {};

	virtual void Execute(LPCSTR args)
	{
		string256 value;
		sscanf(args, "%s", value);

		int res = std::stoi(value);

		if (res == 0 || res == 1)
		{
			Msg("^ Switching All MT Calculations to %i state", res);

			if (res == 0)
			{
				psDeviceFlags.set(mtSound, false);
				psDeviceFlags.set(mtPhysics, false);
				psDeviceFlags.set(mtNetwork, false);
				psDeviceFlags.set(mtParticles, false);

				mt_LTracking_ = 0;
				mt_dynTextureLoad_ = 0;

				Console->Execute("mt_xrgame 0");
				Console->Execute("mt_xrrender 0");
			}
			else
			{
				psDeviceFlags.set(mtSound, true);
				psDeviceFlags.set(mtPhysics, true);
				psDeviceFlags.set(mtNetwork, true);
				psDeviceFlags.set(mtParticles, true);

				mt_LTracking_ = 1;
				mt_dynTextureLoad_ = 1;

				Console->Execute("mt_xrgame 1");
				Console->Execute("mt_xrrender 1");
			}
		}
		else
			Msg("^ Valid arguments are 0 or 1");
	}
};

class CCC_ConsoleColors : public IConsole_Command
{
public:
	CCC_ConsoleColors(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		Msg("~~ Console color of this simbol");
		Msg("!! Console color of this simbol");
		Msg("@@ Console color of this simbol");
		Msg("## Console color of this simbol");
		Msg("$$ Console color of this simbol");
		Msg("%% Console color of this simbol");
		Msg("^^ Console color of this simbol");
		Msg("&& Console color of this simbol");
		Msg("** Console color of this simbol");
		Msg("-- Console color of this simbol");
		Msg("++ Console color of this simbol");
		Msg("== Console color of this simbol");
		Msg("// Console color of this simbol");
	}
};

class CCC_DumpObjects : public IConsole_Command {
public:
	CCC_DumpObjects(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR)
	{
		if(g_pGameLevel)
			g_pGameLevel->Objects.dump_all_objects();
	}
};

class CCC_MemStats : public IConsole_Command
{
public:
	CCC_MemStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) {
		/*
				Memory.mem_compact		();
				u32		_crt_heap		= mem_usage_impl((HANDLE)_get_heap_handle(),0,0);
				u32		_process_heap	= mem_usage_impl(GetProcessHeap(),0,0);
		#ifdef SEVERAL_ALLOCATORS
				u32		_game_lua		= game_lua_memory_usage();
				u32		_render			= ::Render->memory_usage();
		#endif
				int		_eco_strings	= (int)g_pStringContainer->stat_economy			();
				int		_eco_smem		= (int)g_pSharedMemoryContainer->stat_economy	();
				u32		m_base=0,c_base=0,m_lmaps=0,c_lmaps=0;

				if (Device.m_pRender) Device.m_pRender->ResourcesGetMemoryUsage(m_base,c_base,m_lmaps,c_lmaps);

				*/

		Msg(LINE_SPACER);

#pragma todo("Fix it, otherwise no need in this random numbers")
		/*
		log_vminfo();

		Msg		("* [ D3D ]: textures[%d K]", (m_base+m_lmaps)/1024);

#ifndef SEVERAL_ALLOCATORS
		Msg		("* [x-ray]: crt heap[%d K], process heap[%d K]",_crt_heap/1024,_process_heap/1024);
#else
		Msg		("* [x-ray]: crt heap[%d K], process heap[%d K], game lua[%d K], render[%d K]",_crt_heap/1024,_process_heap/1024,_game_lua/1024,_render/1024);
#endif

		Msg		("* [x-ray]: economy: strings[%d K], smem[%d K]",_eco_strings/1024,_eco_smem);

#ifdef FS_DEBUG
		Msg		("* [x-ray]: file mapping: memory[%d K], count[%d]",g_file_mapped_memory/1024,g_file_mapped_count);
		dump_file_mappings	();
#endif
		*/

		Msg("* Total Process RAM Usage = [%u KB]\n", Device.Statistic->GetTotalRAMConsumption() / 1024);
	}
};


ENGINE_API float camFov = 67.5f;
ENGINE_API float psHUD_FOV = 0.45f;

XRCORE_API extern BOOL	log_sending_thread_ID;

ENGINE_API u32 particlesCollision_ = 1; // 1 = only those, that are set in SDK, 2 = forced to all, 0 = off
ENGINE_API float particlesCollisionDistance_ = 100.f;

ENGINE_API BOOL logPerfomanceProblems_ = FALSE; // logs the perfomance spikes in scheduler updates, updatecls, server object updates

extern Flags32		psEnvFlags;

extern float		ps_frames_per_sec;

ENGINE_API float VIEWPORT_NEAR_HUD = 0.2f;
ENGINE_API float viewPortNearK = 0.5f;
ENGINE_API float camAspectK = 1.f;

BOOL keep_necessary_textures = FALSE;
BOOL mt_texture_prefetching = TRUE;
ENGINE_API BOOL mt_texture_loading = 1;

ENGINE_API BOOL mtUseCustomAffinity_ = 0;

BOOL statisticAllowed_ = TRUE;

BOOL show_FPS_only = FALSE;

BOOL show_engine_timers = FALSE;
BOOL show_render_times = FALSE;
BOOL log_render_times = FALSE;
BOOL log_engine_times = FALSE;

BOOL show_UpdateClTimes_ = FALSE;
BOOL show_ScedulerUpdateTimes_ = FALSE;
BOOL show_MTTimes_ = FALSE;
BOOL show_MTRenderDelays_ = FALSE;
BOOL show_OnFrameTimes_ = FALSE;

BOOL display_ram_usage = FALSE;
BOOL display_cpu_usage = FALSE;

BOOL displayFrameGraph = FALSE;

BOOL advSettingsDiclaimerIsShown = FALSE;

ENGINE_API BOOL debugSecondViewPort = FALSE;

ENGINE_API BOOL psSVP1FrustumOptimize = TRUE;
ENGINE_API float psSVP1FrustumFovK = 0.45f;
ENGINE_API float psSVP1FrustumWidthK = 0.55f;
ENGINE_API float psSVP1FrustumHeightK = 0.65f;
ENGINE_API float psSVPImageSizeK = 0.7f;

ENGINE_API float devfloat1 = 0.f;
ENGINE_API float devfloat2 = 0.f;
ENGINE_API float devfloat3 = 0.f;
ENGINE_API float devfloat4 = 0.f;

extern ENGINE_API float g_console_sensitive;

extern int g_Dump_Export_Obj;

void CCC_Register()
{
	psDeviceFlags.set(rsSkipTextureLoading, false);
	psMouseSens = 0.12f;

	psSoundOcclusionScale = pSettings->r_float("sound", "occlusion_scale"); clamp(psSoundOcclusionScale, 0.1f, .5f);

	// General
	CMD1(CCC_Help,				"help");
	CMD1(CCC_Quit,				"quit");
	CMD1(CCC_Start,				"start");
	CMD1(CCC_Disconnect,		"disconnect");
	CMD1(CCC_HideConsole,		"hide");
	CMD1(CCC_SaveCFG,			"cfg_save");
	CMD1(CCC_LoadCFG,			"cfg_load");
	CMD1(CCC_FlushLog,			"flush");
	CMD1(CCC_ClearLog,			"clear_log");
	CMD1(CCC_ReloadSystemLtx,	"reload_system_ltx");

	// General video control
	CMD1(CCC_VidMode,			"vid_mode");
	CMD1(CCC_VID_Reset, 		"vid_restart");
	CMD1(CCC_r2,				"renderer");
	CMD2(CCC_Gamma,				"rs_c_gamma", &ps_gamma);
	CMD2(CCC_Gamma,				"rs_c_brightness", &ps_brightness);
	CMD2(CCC_Gamma,				"rs_c_contrast", &ps_contrast);

	// MT Control
	CMD3(CCC_Mask,				"mt_sound",				&psDeviceFlags,			mtSound);
	CMD3(CCC_Mask,				"mt_physics",			&psDeviceFlags,			mtPhysics);
	CMD3(CCC_Mask,				"mt_network",			&psDeviceFlags,			mtNetwork);
	CMD3(CCC_Mask,				"mt_particles",			&psDeviceFlags,			mtParticles);
	CMD4(CCC_Integer,			"mt_light_tracking",	&mt_LTracking_,			0, 1);
	CMD4(CCC_Integer,			"mt_dyn_texture_load",	&mt_dynTextureLoad_,	0, 1);
	CMD4(CCC_Integer,			"mt_texture_prefetching",	&mt_texture_prefetching,			0, 1);
	CMD4(CCC_Integer,			"mt_texture_loading",		&mt_texture_loading,				0, 1);
	CMD4(CCC_Integer,			"mt_use_custom_threads_affinity_mask",&mtUseCustomAffinity_,	0, 1);
	CMD1(CCC_SwitchAllMT,		"mt_global_switch");

	// Render device states
	CMD3(CCC_Mask,				"rs_prefetch_objects",		&psDeviceFlags, rsPrefObjects);

	CMD3(CCC_Mask,				"rs_v_sync",			&psDeviceFlags,		rsVSync);
	CMD3(CCC_Mask,				"rs_fullscreen",		&psDeviceFlags,		rsFullscreen);
	CMD3(CCC_Mask,				"rs_refresh_60hz",		&psDeviceFlags,		rsRefresh60hz);

	CMD4(CCC_Float,				"rs_vis_distance",		&psVisDistance,		0.2f,	4.5f);
	CMD4(CCC_Float,				"rs_fog_distance",		&psFogDistance,		0.2f,	4.5f);

	CMD4(CCC_Float,				"rs_cap_frame_rate",	&ps_frames_per_sec,	10.f,	900.00f);

	if (CApplication::isDeveloperMode)
		CMD3(CCC_Mask,			"rs_detail", &psDeviceFlags, rsDetails);

	// New statistic window stuff
	CMD3(CCC_Mask,				"rs_stats",				&psDeviceFlags,		rsStatistic);

	CMD4(CCC_Integer,			"rs_global_statistics_switch", &statisticAllowed_,				0, 1);

	CMD4(CCC_Integer,			"rs_fps",				&show_FPS_only,						0, 1);
	CMD4(CCC_Integer,			"rs_engine_timers",		&show_engine_timers,				0, 1);
	CMD4(CCC_Integer,			"rs_render_timers",		&show_render_times,					0, 1);
	CMD4(CCC_Integer,			"rs_log_render_timers",	&log_render_times,					0, 1);
	CMD4(CCC_Integer,			"rs_log_engine_timers", &log_engine_times,					0, 1);

	CMD4(CCC_Integer,			"rs_updatecl_times",		&show_UpdateClTimes_,				0, 1);
	CMD4(CCC_Integer,			"rs_sc_update_times",		&show_ScedulerUpdateTimes_,			0, 1);
	CMD4(CCC_Integer,			"rs_log_perfomance_spikes",	&logPerfomanceProblems_,			0, 1);
	CMD4(CCC_Integer,			"rs_mt_work_times",			&show_MTTimes_,						0, 1);
	CMD4(CCC_Integer,			"rs_mt_render_delays_time",	&show_MTRenderDelays_,				0, 1);
	CMD4(CCC_Integer,			"rs_on_frame_times",		&show_OnFrameTimes_,				0, 1);

	CMD4(CCC_Integer,			"rs_display_ram_usage", &display_ram_usage,					0, 1);
	CMD4(CCC_Integer,			"rs_display_cpu_usage", &display_cpu_usage,					0, 1);

	CMD4(CCC_Integer,			"rs_log_sending_thread_ID",	&log_sending_thread_ID,			0, 1);

	CMD4(CCC_Integer,			"rs_frame_time_graph",	&displayFrameGraph,			0, 1);

	// Camera
	CMD4(CCC_Float,				"cam_inert",			&psCamInert, 	0.f,	0.5f);
	CMD4(CCC_Float,				"cam_slide_inert",		&psCamSlideInert, 0.f,	0.5f);
	
	CMD3(CCC_Mask,				"rs_cam_pos",			&psDeviceFlags,		rsCameraPos);

	CMD4(CCC_Float,				"rs_view_port_near",	&VIEWPORT_NEAR,				0.001, 5.0);
	CMD4(CCC_Float,				"rs_view_port_near_hud",&VIEWPORT_NEAR_HUD,			0.001, 5.0);
	CMD4(CCC_Float,				"rs_view_port_near_k",	&viewPortNearK,				0.01, 5.0);

	CMD4(CCC_Float,				"rs_cam_aspect_k",		&camAspectK,				0.01, 5.0);

	CMD4(CCC_Float,				"rs_fov",				&camFov, 45.0f, 120.0f);
	CMD4(CCC_Float,				"hud_fov",				&psHUD_FOV, 0.1f, 0.8f);

	// SPV1 Frustum correction, for better geometry cutoff
	CMD4(CCC_Integer,			"svp_frustum_optimize",	&psSVP1FrustumOptimize, FALSE, TRUE);
	CMD4(CCC_Float,				"svp_frustum_fov_k",	&psSVP1FrustumFovK, 0.f, 2.f);
	CMD4(CCC_Float,				"svp_frustum_width_k",	&psSVP1FrustumWidthK, 0.f, 2.f);
	CMD4(CCC_Float,				"svp_frustum_height_k",	&psSVP1FrustumHeightK, 0.f, 2.f);

	CMD4(CCC_Float,				"svp_image_size_k",		&psSVPImageSizeK, 0.1f, 2.f);

	// Texture manager
	CMD4(CCC_Integer,			"texture_lod",	&psTextureLOD, 0, 4);
	CMD3(CCC_Mask,				"g_skip_texture_load", &psDeviceFlags, rsSkipTextureLoading); //useful for faster r2 3 4 debugging
	CMD1(CCC_GenTexLodList,		"generate_tex_lod_list");
	CMD4(CCC_Integer,			"keep_textures_in_ram", &keep_necessary_textures, 0, 1);

	// Sound
	CMD2(CCC_Float,				"snd_volume_eff",		&psSoundVEffects);
	CMD2(CCC_Float,				"snd_volume_music",		&psSoundVMusic);
	CMD1(CCC_SND_Restart,		"snd_restart");
	CMD3(CCC_Mask,				"snd_acceleration",			&psSoundFlags,			ss_Hardware);
	CMD3(CCC_Mask,				"snd_efx",					&psSoundFlags,			ss_EAX);
	CMD4(CCC_Integer,			"snd_targets",				&psSoundTargets,		64, 128);
	CMD4(CCC_Integer,			"snd_cache_size",			&psSoundCacheSizeMB,	32, 64);
	CMD4(CCC_Integer,			"snd_use_distance_delay",	&psUseDistDelay,		0, 1);
	CMD4(CCC_Float,				"snd_sound_speed",			&psSoundSpeed,			1.f, 1000.f);
	CMD1(CCC_soundDevice,		"snd_device");

	// Input
	CMD3(CCC_Mask,				"mouse_invert",			&psMouseInvert, 1);
	CMD4(CCC_Float,				"mouse_sens",			&psMouseSens, 0.05f, 0.6f);

	CMD1(CCC_ExclusiveMouse,	"input_exclusive_mouse");
	CMD1(CCC_ExclusiveKeyboard,	"input_exclusive_keyboard");

	// Other
	CMD4(CCC_Float,				"developer_float_1", &devfloat1, -100000.f, 100000.f);
	CMD4(CCC_Float,				"developer_float_2", &devfloat2, -100000.f, 100000.f);
	CMD4(CCC_Float,				"developer_float_3", &devfloat3, -100000.f, 100000.f);
	CMD4(CCC_Float,				"developer_float_4", &devfloat4, -100000.f, 100000.f);

	CMD4(CCC_Integer,			"adv_settings_diclaimer_is_shown",	&advSettingsDiclaimerIsShown, 0, 1);
	CMD1(CF_ThreadTest,			"run_single_thread_test");
	CMD1(CCC_ConsoleColors,		"console_colors");

	CMD1(CCC_Crash,				"crash");

	CMD4(CCC_Float,				"con_sensitive", &g_console_sensitive, 0.01f, 1.0f);

	// Debug
	CMD4(CCC_Integer,			"rs_debug_view_port_2", &debugSecondViewPort, FALSE, TRUE);

	CMD1(CCC_MemStats,			"stat_memory");
	CMD4(CCC_Integer,			"net_dbg_dump_export_obj",	&g_Dump_Export_Obj, 0, 1);
	CMD1(CCC_DumpObjects,		"dump_all_objects");

#ifdef DEBUG
	extern BOOL debug_destroy;
	CMD4(CCC_Integer,		"debug_destroy", &debug_destroy, FALSE, TRUE );

	CMD1(CCC_DumpOpenFiles,	"dump_open_files");

	CMD3(CCC_Mask,			"rs_occ_draw",			&psDeviceFlags,		rsOcclusionDraw);
	CMD3(CCC_Mask,			"rs_occ_stats",			&psDeviceFlags,		rsOcclusionStats);

	CMD1(CCC_DbgStrCheck,	"dbg_str_check");
	CMD1(CCC_DbgStrDump,	"dbg_str_dump");

	// Events
	CMD1(CCC_E_Dump,		"e_list");
	CMD1(CCC_E_Signal,		"e_signal");

	CMD3(CCC_Mask,			"rs_clear_bb",			&psDeviceFlags,		rsClearBB);
	CMD3(CCC_Mask,			"rs_constant_fps",		&psDeviceFlags,		rsConstantFPS);

	CMD1(CCC_MotionsStat,	"stat_motions");
	CMD1(CCC_TexturesStat,	"stat_textures");

#ifdef DEBUG_MEMORY_MANAGER
	CMD1(CCC_MemStat,		"dbg_mem_dump");
	CMD1(CCC_DbgMemCheck,	"dbg_mem_check");
#endif
#endif
};
 