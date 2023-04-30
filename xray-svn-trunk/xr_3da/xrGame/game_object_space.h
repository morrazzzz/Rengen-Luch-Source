#pragma once

namespace GameObject {
	enum ECallbackType {
		eTradeStart = u32(0),
		eTradeStop,
		eTradeSellBuyItem,
		eTradePerformTradeOperation,

		eZoneEnter,
		eZoneExit,
		eExitLevelBorder,
		eEnterLevelBorder,
		eDeath,

		ePatrolPathInPoint,

		eInventoryPda,
		eInventoryInfo,
		eArticleInfo,
		eHudAnimStarted,
		eHudAnimCompleted,
		eTaskStateChange,
		eMapLocationAdded,
		eMapSpecialLocationDiscovered,

		eUseObject,

		eHit,

		eSound,

		eActionTypeMovement,
		eActionTypeWatch,
		eActionTypeRemoved,
		eActionTypeAnimation,
		eActionTypeSound,
		eActionTypeParticle,
		eActionTypeObject,
		eActionTypeWeaponFire,

		eActorSleep,

		eHelicopterOnPoint,
		eHelicopterOnHit,

		eOnItemTake,
		eOnItemDrop,

		//lost alpha
		eOnButtonPress,
		eOnButtonRelease,
		eOnButtonHold,
		eOnMoveToSlot,
		eOnMoveToBelt,
		eOnMoveToArtBelt,
		eOnMoveToRuck,
	//	eOnSoundPlayed,
		//

		eScriptAnimation,
		
		eTraderGlobalAnimationRequest,
		eTraderHeadAnimationRequest,
		eTraderSoundEnd,

		eInvBoxItemTake,
		eWeaponNoAmmoAvailable,

		eDummy = u32(-1),
	};
};

