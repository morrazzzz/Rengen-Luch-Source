// dxRender_Visual.cpp: implementation of the dxRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "../../xr_3da/render.h"
#include "fbasicvisual.h"
#include "../../xr_3da/fmesh.h"


IRender_Mesh::~IRender_Mesh()		
{ 
	_RELEASE(p_rm_Vertices); 
	_RELEASE(p_rm_Indices);		
}

dxRender_Visual::dxRender_Visual()
{
	Type				= 0;
	shader				= 0;
	vis.clear_vis_data	();

	parentHierarhyHolder_ = nullptr;

	RImplementation.Models->createdModelsCnt_++;
}

dxRender_Visual::~dxRender_Visual()
{
	VERIFY2(GetCurrentThreadId() == Core.mainThreadID, "Visuals must be destructed only within main thread, to avoid mt bugs");

	RImplementation.Models->deletedModelsCnt_++;
}

void dxRender_Visual::Release()
{
}

void dxRender_Visual::Load(const char* N, IReader *data, u32 )
{
#ifdef DEBUG
	dbg_name	= N;
#endif

	// header
	VERIFY(data);

	ogf_header hdr;

	if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
	{
		R_ASSERT2(hdr.format_version == xrOGF_FormatVersion, "Invalid visual version");

		Type = hdr.type;

		if (hdr.shader_id)
			shader = ::RImplementation.getShader(hdr.shader_id);

		vis.box.set(hdr.bb.min, hdr.bb.max);
		vis.sphere.set(hdr.bs.c, hdr.bs.r);
	}
	else
	{
		FATAL("Invalid visual");
	}

	// Shader
	if (data->find_chunk(OGF_TEXTURE))
	{
		string256 fnT, fnS;

		data->r_stringZ(fnT, sizeof(fnT));
		data->r_stringZ(fnS, sizeof(fnS));

		shader.create(fnS, fnT);
	}
}

#define PCOPY(a) a = pFrom->a

void dxRender_Visual::Copy(dxRender_Visual *pFrom)
{
	PCOPY(Type);
	PCOPY(shader);

	vis.box = pFrom->vis.box;
	vis.sphere = pFrom->vis.sphere;
	pFrom->vis.CopySelf(vis);

#ifdef DEBUG
	PCOPY(dbg_name);
#endif
}
