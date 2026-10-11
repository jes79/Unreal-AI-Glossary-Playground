#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * 08 분야가 쓰는 게임플레이 태그를 C++ 네이티브 태그로 등록한다.
 * (DefaultGame.ini 의 [/Script/GameplayTags.GameplayTagsSettings] 목록은 실제로는 DefaultGameplayTags.ini 에서 읽히므로
 *  로드되지 않는다 — 모듈이 직접 등록하면 ini 와 상관없이 항상 존재한다)
 */
namespace D08Tags
{
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Burning);
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Poisoned);
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fireball);
	GLOSSARYSYSTEMS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Cooldown_Fireball);
}
