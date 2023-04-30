#ifndef	RenderVisual_included
#define	RenderVisual_included
#pragma once

class IKinematics;
class IKinematicsAnimated;
class IParticleCustom;
struct vis_data;

class IRenderVisual
{
public:
	IRenderVisual() { ignoreGeometryDrawOptimization_ = false; }

	virtual ~IRenderVisual()
	{
		R_ASSERT2(GetCurrentThreadId() == Core.mainThreadID, "Visuals must be destructed only within main thread");
	}

	virtual vis_data&	_BCL	getVisData() = 0;
	virtual u32					getType() = 0;

	bool						ignoreGeometryDrawOptimization_;

#ifdef DEBUG
	virtual shared_str	_BCL	getDebugName() = 0;
#endif

	virtual	IKinematics*	_BCL	dcast_PKinematics			()				{ return 0;	}
	virtual	IKinematicsAnimated*	dcast_PKinematicsAnimated	()				{ return 0;	}
	virtual IParticleCustom*		dcast_ParticleCustom		()				{ return 0;	}
};

#endif