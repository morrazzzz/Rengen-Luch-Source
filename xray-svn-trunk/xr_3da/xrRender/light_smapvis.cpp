#include "StdAfx.h"
#include "../xrRender/light.h"
#include "../xrRender/FBasicVisual.h"

// This algorithm supports only non-moving and non-changing lights. Otherwise it'll just slow down rendering, because of GPU occ is constantly being buisy

smapvis::smapvis()
{
	invalidate();
	frame_sleep = 0;

	testQ_id = u32(-1);

	occFlushed_ = true;
	needInvalidate = false;
}

smapvis::~smapvis ()
{
	flushoccq();
	invalidate();
}

void smapvis::invalidate()
{
	state		= state_counting;
	testQ_V		= 0;

	frame_sleep = CurrentFrame() + ps_r__LightSleepFrames;

	invisible.clear	();

	needInvalidate = false;
}

void smapvis::need_invalidate()
{
	needInvalidate = true;
}

void smapvis::smvisbegin(DsGraphBuffer& render_buffer)
{
	render_buffer.ResetStaticVisualCount();

	switch	(state)
	{
	case state_counting:	
		// do nothing -> we just prepare for testing process
		break;
	case state_working:
		// mark already known to be invisible visuals, set breakpoint

		testQ_V	= 0;
		testQ_id = 0;

		smmark(render_buffer);

		render_buffer.indexOfVisualWeLookFor_ = indexOfVisualWeWantToTest_;
		render_buffer.testMeCallBack_ = this;

		break;
	case state_usingTC:
		// just mark
		smmark(render_buffer);

		break;
	}
}

void smapvis::smvisend(DsGraphBuffer& render_buffer)
{
	// Gather stats
	u32	ts;

	ts = render_buffer.GetStaticVisualCount();
	RImplementation.stats.ic_total += ts;

	render_buffer.indexOfVisualWeLookFor_ = 0;
	render_buffer.testMeCallBack_ = nullptr;

	switch(state)
	{
	case state_counting:
		// switch to 'working'
		if (sleep())
		{
			countOfVisualsToTest_ = ts;
			indexOfVisualWeWantToTest_ = 0;
			state = state_working;
		}
		break;
	case state_working:
		// feedback should be called at this time -> clear feedback
		// issue query
		if (testQ_V)
		{
			//R_ASSERT(occFlushed_); // This happens sometimes. Debug it

			RImplementation.occq_begin(testQ_id);

			render_buffer.visMarker_++;

			RImplementation.r_dsgraph_insert_static(testQ_V, render_buffer);
			RImplementation.r_dsgraph_render_graph(0, render_buffer);

			RImplementation.occq_end(testQ_id);

			testQ_frame = CurrentFrame() + 1;	// get result on next frame

			occFlushed_ = false;
		}
		break;
	case state_usingTC:
		// nothing to do
		break;
	}

	if (needInvalidate)
		invalidate();
}

void smapvis::flushoccq()
{
#pragma todo("Need separate occ for secondary viewports?")
	// the tough part
	if (testQ_frame != CurrentFrame())
		return;

	if ((state != state_working) || (!testQ_V))
		return;

	u64	fragments = RImplementation.occq_get(testQ_id);

	occFlushed_ = true;

	if(fragments == 0)
	{
		// this is invisible shadow-caster, register it
		// next time we will not get this caster, so 'indexOfVisualWeWantToTest_' remains the same
		invisible.push_back	(testQ_V);
		countOfVisualsToTest_--;
	}
	else
	{
		// this is visible shadow-caster, advance index
		indexOfVisualWeWantToTest_++;
	}

	testQ_V = 0;

	if (indexOfVisualWeWantToTest_ == countOfVisualsToTest_)
	{
		// we are at the end of list - stop further testing and use what we got into invisibles pool
		if (state == state_working)
			state = state_usingTC;
	}
}

void smapvis::resetoccq()
{
	if (testQ_frame == (CurrentFrame() + 1))
		testQ_frame--;

	flushoccq();
}

void smapvis::smmark(DsGraphBuffer& render_buffer)
{
	RImplementation.stats.ic_culled += invisible.size();

	// Mark the invisible geometry with the same value, as our Next ds buffer calculation mark has. This will disable this ggeometry from rendering
	u32 marker = render_buffer.visMarker_ + 1;

	for (u32 it = 0; it < invisible.size(); it++)
	{
		invisible[it]->protectVisMarker_.Enter();

		auto itter = invisible[it]->vis.vis_marker.find(render_buffer.visMarkerId_); // find our buffer mark

		if (itter != invisible[it]->vis.vis_marker.end())
			itter->second = marker; // this effectively disables rendering

		invisible[it]->protectVisMarker_.Leave();
	}
}

void smapvis::rfeedback_static(dxRender_Visual* V)
{
	testQ_V = V;
}
