////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_script_lua_extension.h
//	Created 	: 19.09.2003
//  Modified 	: 22.09.2003
//	Author		: Dmitriy Iassenev
//	Description : XRay Script extensions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_script_space.h"
//struct CLuaVirtualMachine;

namespace Script {
	bool				bfPrintOutput				(CLuaVirtualMachine *tpLuaVM, LPCSTR	caScriptName, int iErorCode = 0);
	LPCSTR				cafEventToString			(int				iEventCode);
	void				vfPrintError				(CLuaVirtualMachine *tpLuaVM, int		iErrorCode);
	bool				bfListLevelVars				(CLuaVirtualMachine *tpLuaVM, int		iStackLevel);
	bool				bfLoadBuffer				(CLuaVirtualMachine *tpLuaVM, LPCSTR	caBuffer,		size_t	tSize,				LPCSTR	caScriptName, LPCSTR caNameSpaceName = 0);
	bool				bfLoadFileIntoNamespace		(CLuaVirtualMachine *tpLuaVM, LPCSTR	caScriptName,	LPCSTR	caNamespaceName,	bool	bCall);
	bool				bfGetNamespaceTable			(CLuaVirtualMachine *tpLuaVM, LPCSTR	caName);
	CLuaVirtualMachine	*get_namespace_table		(CLuaVirtualMachine *tpLuaVM, LPCSTR	caName);
	bool				bfIsObjectPresent			(CLuaVirtualMachine *tpLuaVM, LPCSTR	caIdentifier,	int type);
	bool				bfIsObjectPresent			(CLuaVirtualMachine *tpLuaVM, LPCSTR	caNamespaceName, LPCSTR	caIdentifier, int type);
	luabind::object		lua_namespace_table			(CLuaVirtualMachine *tpLuaVM, LPCSTR namespace_name);
};
