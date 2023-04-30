#include "StdAfx.h"
#include ".\r__occlusion.h"
#include "QueryHelper.h"

R_occlusion::R_occlusion(void)
{
	enabled = strstr(Core.Params, "-no_occq") ? FALSE : TRUE;
}


R_occlusion::~R_occlusion(void)
{
	occq_destroy();
}


void R_occlusion::occq_create(u32 limit)
{
	pool.reserve(limit);
	used.reserve(limit);
	fids.reserve(limit);

	for (u32 it = 0; it<limit; it++)
	{
		_Q	q;	q.order = it;

		if (FAILED(CreateQuery(&q.Q, D3DQUERYTYPE_OCCLUSION))) 
			break;

		pool.push_back(q);
	}

	std::reverse(pool.begin(), pool.end());
}


void R_occlusion::occq_destroy()
{
	while (!used.empty())
	{
		_RELEASE(used.back().Q);

		used.pop_back();
	}
	while (!pool.empty())
	{
		_RELEASE(pool.back().Q);

		pool.pop_back();
	}

	used.clear();
	pool.clear();
	fids.clear();
}


u32	R_occlusion::occq_begin(u32& ID)
{
	if (!enabled) return 0;

	RDEVICE.Statistic->occ_Begin_calls++;

	//	Igor: prevent release crash if we issue too many queries
	if (pool.empty())
	{
		if ((CurrentFrame() % 40) == 0)
			Msg("!Occlusion: Too many occlusion queries were issued > %u", occq_size);

		ID = iInvalidHandle;

		return 0;
	}

	RImplementation.stats.o_queries++;

	if (!fids.empty())	
	{
		ID = fids.back();
		fids.pop_back();

		R_ASSERT(pool.size());

		used[ID] = pool.back();
	}
	else 
	{
		ID = used.size();

		R_ASSERT(pool.size());

		used.push_back(pool.back());
	}
	if (used[ID].status == 2)
	{
		occq_result	fragments = 0;
		GetData(used[ID].Q, &fragments, sizeof(fragments));
		used[ID].status = 0;
	}
	R_ASSERT(used[ID].status == 0);
	pool.pop_back();
	//ÑHK_DX					(used[ID].Q->Issue	(D3DISSUE_BEGIN));

	CHK_DX(BeginQuery(used[ID].Q));
#ifdef DEBUG
	 //Msg				("begin: [%2d] - %d", used[ID].order, ID);
#endif
	 used[ID].status = 1;
	return used[ID].order;
}


void R_occlusion::occq_end(u32&	ID)
{
	if (!enabled)
		return;

	RDEVICE.Statistic->occ_End_calls++;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle)
		return;

	R_ASSERT(used[ID].status == 1);

#ifdef DEBUG
	// Msg				("end  : [%2d] - %d", used[ID].order, ID);
#endif
	//CHK_DX			(used[ID].Q->Issue	(D3DISSUE_END));
	CHK_DX(EndQuery(used[ID].Q));
	used[ID].status = 2;
}

extern float maxWaitForOccCalc_;

R_occlusion::occq_result R_occlusion::occq_get(u32&	ID)
{
	if (!enabled)
	{
		return 0xffffffff;
	}

	RDEVICE.Statistic->occ_Get_calls++;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle) 
		return 0xFFFFFFFF;

	if (ID >= used.size())
	{
		Msg("!Occlusion: Wrong data sequence");
		return 0xffffffff;
	}

	if (!used[ID].Q)
	{
		Msg("!Occlusion: Null queue skipped");
		return 0xffffffff;
	}

	occq_result	fragments = 0;
	HRESULT hr;
	R_ASSERT(used[ID].status == 2);
	// CHK_DX		(used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH));
//	 Msg			("get  : [%2d] - %d => %d", used[ID].order, ID, fragments);

	CTimer T;
	T.Start();

	Device.Statistic->RenderDUMP_Wait.Begin();

	//while	((hr=used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH))==S_FALSE) {

	R_ASSERT2(ID<used.size(), make_string("_Pos = %d, size() = %d ", ID, used.size()));

	while ((hr = GetData(used[ID].Q, &fragments, sizeof(fragments))) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r2_wait_sleep);

		if (T.GetElapsed_sec() * 1000.f > maxWaitForOccCalc_)
		{
			fragments = (occq_result)-1;//0xffffffff;
			break;
		}
	}

	RImplementation.vpStats->GPU_Occ_Wait += T.GetElapsed_sec() * 1000.f;
	Device.Statistic->RenderDUMP_Wait.End();

	if (hr == D3DERR_DEVICELOST)	fragments = 0xffffffff;

	if (0 == fragments)	RImplementation.stats.o_culled++;

	// insert into pool (sorting in decreasing order)
	_Q& Q = used[ID];

	if (pool.empty()) 
		pool.push_back(Q);
	else	
	{
		int	it = int(pool.size()) - 1;

		while ((it >= 0) && (pool[it].order < Q.order))
			it--;

		pool.insert(pool.begin() + it + 1, Q);
	}

	// remove from used and shrink as necessary
	used[ID].Q = 0;
	used[ID].status = 0;
	fids.push_back(ID);
	ID = 0;

	return fragments;
}
