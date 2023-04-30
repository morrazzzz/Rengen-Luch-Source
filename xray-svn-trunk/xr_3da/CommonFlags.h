#pragma once

enum {
	CF_AiUseTorchDynamicLights = (1 << 0),
	CF_Prefetch_UI = (1 << 2),
	CF_NoInvulnarable = (1 << 3)
};

ENGINE_API extern Flags32		g_uCommonFlags;