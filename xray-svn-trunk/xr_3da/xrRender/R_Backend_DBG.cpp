#include "stdafx.h"
#pragma hdrstop

void CBackend::dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
	RCache.set_Geometry(geom);
	RCache.BackendRender(pt, vBase, pc);
}

void CBackend::dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
	RCache.set_Geometry(geom);
	RCache.BackendRender(pt, baseV, startV, countV, startI, PC);
}

#ifdef DRENDER

void CBackend::dbg_Draw(D3DPRIMITIVETYPE t, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt)
{
	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");
}

void CBackend::dbg_Draw(D3DPRIMITIVETYPE t, FVF::L* pVerts, int pcnt)
{
	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");
}


void CBackend::dbg_DrawOBB(Fmatrix& t, Fvector& half_dim, u32 c)
{
	Fmatrix mL2W_Transform,mScaleTransform;

	mScaleTransform.scale(half_dim);
	mL2W_Transform.mul_43(t,mScaleTransform);

	FVF::L  aabb[8];
	aabb[0].set( -1, -1, -1, c ); // 0
	aabb[1].set( -1, +1, -1, c ); // 1
	aabb[2].set( +1, +1, -1, c ); // 2
	aabb[3].set( +1, -1, -1, c ); // 3
	aabb[4].set( -1, -1, +1, c ); // 4
	aabb[5].set( -1, +1, +1, c ); // 5
	aabb[6].set( +1, +1, +1, c ); // 6
	aabb[7].set( +1, -1, +1, c ); // 7

	u16 aabb_id[12*2] = {
		0,1,  1,2,  2,3,  3,0,  4,5,  5,6,  6,7,  7,4,  1,5,  2,6,  3,7,  0,4
	};

	set_xform_world	(mL2W_Transform);
	dbg_Draw(D3DPT_LINELIST, aabb, 8, aabb_id, 12);
}

void CBackend::dbg_DrawTRI	(Fmatrix& t, Fvector& p1, Fvector& p2, Fvector& p3, u32 c)
{
	FVF::L	tri[3];
	tri[0].p = p1; tri[0].color = c;
	tri[1].p = p2; tri[1].color = c;
	tri[2].p = p3; tri[2].color = c;

	set_xform_world	(t);
	dbg_Draw(D3DPT_TRIANGLESTRIP, tri, 1);
}
void CBackend::dbg_DrawLINE(Fmatrix& t, Fvector& p1, Fvector& p2, u32 c)
{
	FVF::L line[2];
	line[0].p = p1; line[0].color = c;
	line[1].p = p2; line[1].color = c;

	set_xform_world	(t);
	dbg_Draw(D3DPT_LINELIST, line, 1);
}
void CBackend::dbg_DrawEllipse(Fmatrix& t, u32 c)
{
	float gVertices[] =
	{
		0.0000f,0.0000f,1.0000f,  0.0000f,0.3827f,0.9239f,  -0.1464f,0.3536f,0.9239f,
			-0.2706f,0.2706f,0.9239f,  -0.3536f,0.1464f,0.9239f,  -0.3827f,0.0000f,0.9239f,
			-0.3536f,-0.1464f,0.9239f,  -0.2706f,-0.2706f,0.9239f,  -0.1464f,-0.3536f,0.9239f,
			0.0000f,-0.3827f,0.9239f,  0.1464f,-0.3536f,0.9239f,  0.2706f,-0.2706f,0.9239f,
			0.3536f,-0.1464f,0.9239f,  0.3827f,0.0000f,0.9239f,  0.3536f,0.1464f,0.9239f,
			0.2706f,0.2706f,0.9239f,  0.1464f,0.3536f,0.9239f,  0.0000f,0.7071f,0.7071f,
			-0.2706f,0.6533f,0.7071f,  -0.5000f,0.5000f,0.7071f,  -0.6533f,0.2706f,0.7071f,
			-0.7071f,0.0000f,0.7071f,  -0.6533f,-0.2706f,0.7071f,  -0.5000f,-0.5000f,0.7071f,
			-0.2706f,-0.6533f,0.7071f,  0.0000f,-0.7071f,0.7071f,  0.2706f,-0.6533f,0.7071f,
			0.5000f,-0.5000f,0.7071f,  0.6533f,-0.2706f,0.7071f,  0.7071f,0.0000f,0.7071f,
			0.6533f,0.2706f,0.7071f,  0.5000f,0.5000f,0.7071f,  0.2706f,0.6533f,0.7071f,
			0.0000f,0.9239f,0.3827f,  -0.3536f,0.8536f,0.3827f,  -0.6533f,0.6533f,0.3827f,
			-0.8536f,0.3536f,0.3827f,  -0.9239f,0.0000f,0.3827f,  -0.8536f,-0.3536f,0.3827f,
			-0.6533f,-0.6533f,0.3827f,  -0.3536f,-0.8536f,0.3827f,  0.0000f,-0.9239f,0.3827f,
			0.3536f,-0.8536f,0.3827f,  0.6533f,-0.6533f,0.3827f,  0.8536f,-0.3536f,0.3827f,
			0.9239f,0.0000f,0.3827f,  0.8536f,0.3536f,0.3827f,  0.6533f,0.6533f,0.3827f,
			0.3536f,0.8536f,0.3827f,  0.0000f,1.0000f,0.0000f,  -0.3827f,0.9239f,0.0000f,
			-0.7071f,0.7071f,0.0000f,  -0.9239f,0.3827f,0.0000f,  -1.0000f,0.0000f,0.0000f,
			-0.9239f,-0.3827f,0.0000f,  -0.7071f,-0.7071f,0.0000f,  -0.3827f,-0.9239f,0.0000f,
			0.0000f,-1.0000f,0.0000f,  0.3827f,-0.9239f,0.0000f,  0.7071f,-0.7071f,0.0000f,
			0.9239f,-0.3827f,0.0000f,  1.0000f,0.0000f,0.0000f,  0.9239f,0.3827f,0.0000f,
			0.7071f,0.7071f,0.0000f,  0.3827f,0.9239f,0.0000f,  0.0000f,0.9239f,-0.3827f,
			-0.3536f,0.8536f,-0.3827f,  -0.6533f,0.6533f,-0.3827f,  -0.8536f,0.3536f,-0.3827f,
			-0.9239f,0.0000f,-0.3827f,  -0.8536f,-0.3536f,-0.3827f,  -0.6533f,-0.6533f,-0.3827f,
			-0.3536f,-0.8536f,-0.3827f,  0.0000f,-0.9239f,-0.3827f,  0.3536f,-0.8536f,-0.3827f,
			0.6533f,-0.6533f,-0.3827f,  0.8536f,-0.3536f,-0.3827f,  0.9239f,0.0000f,-0.3827f,
			0.8536f,0.3536f,-0.3827f,  0.6533f,0.6533f,-0.3827f,  0.3536f,0.8536f,-0.3827f,
			0.0000f,0.7071f,-0.7071f,  -0.2706f,0.6533f,-0.7071f,  -0.5000f,0.5000f,-0.7071f,
			-0.6533f,0.2706f,-0.7071f,  -0.7071f,0.0000f,-0.7071f,  -0.6533f,-0.2706f,-0.7071f,
			-0.5000f,-0.5000f,-0.7071f,  -0.2706f,-0.6533f,-0.7071f,  0.0000f,-0.7071f,-0.7071f,
			0.2706f,-0.6533f,-0.7071f,  0.5000f,-0.5000f,-0.7071f,  0.6533f,-0.2706f,-0.7071f,
			0.7071f,0.0000f,-0.7071f,  0.6533f,0.2706f,-0.7071f,  0.5000f,0.5000f,-0.7071f,
			0.2706f,0.6533f,-0.7071f,  0.0000f,0.3827f,-0.9239f,  -0.1464f,0.3536f,-0.9239f,
			-0.2706f,0.2706f,-0.9239f,  -0.3536f,0.1464f,-0.9239f,  -0.3827f,0.0000f,-0.9239f,
			-0.3536f,-0.1464f,-0.9239f,  -0.2706f,-0.2706f,-0.9239f,  -0.1464f,-0.3536f,-0.9239f,
			0.0000f,-0.3827f,-0.9239f,  0.1464f,-0.3536f,-0.9239f,  0.2706f,-0.2706f,-0.9239f,
			0.3536f,-0.1464f,-0.9239f,  0.3827f,0.0000f,-0.9239f,  0.3536f,0.1464f,-0.9239f,
			0.2706f,0.2706f,-0.9239f,  0.1464f,0.3536f,-0.9239f,  0.0000f,0.0000f,-1.0000f
	};

	const int vcnt = sizeof(gVertices)/(sizeof(float)*3);
	FVF::L  verts[vcnt];

	for (int i = 0; i < vcnt; i++)
	{
		int k = i * 3;

		verts[i].set(gVertices[k], gVertices[k + 1], gVertices[k + 2], c);
	}

	set_xform_world(t);

	//	TODO: DX10: implement
	//VERIFY(!"CBackend::dbg_Draw not implemented.");
	//dbg_Draw(D3DPT_TRIANGLELIST,verts,vcnt,gFaces,224);
}

#endif
