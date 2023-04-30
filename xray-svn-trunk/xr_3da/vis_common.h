
#pragma once

struct vis_data
{
	vis_data()
	{
		clear_vis_data();
	}

	virtual ~vis_data()
	{

	}

	Fsphere					sphere;
	Fbox					box;
	xr_map<size_t, u32>		vis_marker; // for different sub-renders. Can overflow if many short life sources spam its mark and dont erase it at destruction
	
	// is Atomic, since placing critical sections is to costly for perfomance. It ill have bugs, but itll not have undefined values in MT
	std::atomic<u32>		nextHomTestframe;
	std::atomic<u32>		lastHomTestFrame;
	std::atomic<bool>		isVisible;

	IC void		clear_vis_data()
	{
		sphere.P.set	(0, 0, 0);
		sphere.R		= 0;
		box.invalidate	();

		nextHomTestframe	= 0;
		lastHomTestFrame	= 0;
		isVisible			= false;
	}

	IC void CopySelf(vis_data& where) // copy only needed info
	{
		u32 nf = nextHomTestframe;
		u32 lf = lastHomTestFrame;
		bool res = isVisible;

		where.nextHomTestframe = nf;
		where.lastHomTestFrame = lf;
		where.isVisible = res;
	};
};
