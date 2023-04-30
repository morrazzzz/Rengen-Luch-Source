////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_debug.h
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Debug functions
////////////////////////////////////////////////////////////////////////////

#pragma once

enum AI_FLAGS
{
	aiDebug						=(1<<0),
	aiBrain						=(1<<1),
	aiMotion					=(1<<2),
	aiFrustum					=(1<<3),
	aiFuncs						=(1<<4),
	aiALife						=(1<<5),
	aiGOAP						=(1<<7),
	aiCover						=(1<<8),
	aiAnimation					=(1<<9),
	aiVision					=(1<<10),
	aiMonsterDebug				=(1<<11),
	aiStats						=(1<<12),
	aiDestroy					=(1<<13),
	aiSerialize					=(1<<14),
	aiDialogs					=(1<<15),
	aiInfoPortion				=(1<<16),
	aiGOAPScript				=(1<<17),
	aiGOAPObject				=(1<<18),
	aiStalker					=(1<<19),
	aiDrawGameGraph				=(1<<20),
	aiDrawGameGraphStalkers		=(1<<21),
	aiDrawGameGraphObjects		=(1<<22),
	aiNilObjectAccess			=(1<<23),
	aiDebugOnFrameAllocs		=(1<<25),
	aiDrawVisibilityRays		=(1<<26),
	aiAnimationStats			=(1<<27),
	aiDrawGameGraphRealPos		=(1<<28),

	aiObstaclesAvoiding			=(1<<24),
	aiObstaclesAvoidingStatic	=(1<<29),
	aiUseSmartCovers			=(1<<30),
	aiUseSmartCoversAnimationSlot=(1<<31)
};

extern Flags32 psAI_Flags;

enum AI_FLAGS_2
{
	aiDebugMsg					=(1<<0),
	aiTestCorrectness			=(1<<1),
	aiPath						=(1<<2)
};

extern Flags32 psAI_Flags2;