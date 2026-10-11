#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/** 10_Rendering 에셋 빌더가 만드는 에셋 경로와, 빌더·런타임이 같이 쓰는 배치 데이터 */
namespace RenderingAssets
{
	inline const TCHAR* Folder = TEXT("/Game/Glossary/10_Rendering/");

	/** 1003: 약 33만 삼각형 바위, Nanite 켬 */
	inline const TCHAR* NaniteRock = TEXT("/Game/Glossary/10_Rendering/SM_NaniteRock");
	/** 1003 비교용: 같은 바위, Nanite 끔 */
	inline const TCHAR* NaniteRockOff = TEXT("/Game/Glossary/10_Rendering/SM_NaniteRock_NoNanite");
	/** 1010: 비 Nanite 바위 + LOD 3단계 자동 생성 (50/25/10%) */
	inline const TCHAR* LODRock = TEXT("/Game/Glossary/10_Rendering/SM_LODRock");
	/** 1012·1013·1018: 줄기 + 잎 두 머티리얼 슬롯을 가진 간단한 나무 */
	inline const TCHAR* Tree = TEXT("/Game/Glossary/10_Rendering/SM_Tree");
	/** 1011: 마을 건물 25채를 한 메시로 합친 HLOD 프록시 */
	inline const TCHAR* TownProxy = TEXT("/Game/Glossary/10_Rendering/SM_HLODProxy_Town");
	/** 1008: 깜빡이는 Light Function 머티리얼 */
	inline const TCHAR* LightFunction = TEXT("/Game/Glossary/10_Rendering/M_LightFunction_Flicker");
	/** 1018: 나무 뿌리기 PCG 그래프 */
	inline const TCHAR* ForestGraph = TEXT("/Game/Glossary/10_Rendering/PCG_ForestScatter");
	inline const TCHAR* RockMaterial = TEXT("/Game/Glossary/10_Rendering/MI_Rock");

	/** 1011 마을 건물 하나 (바닥 중심 기준) */
	struct FTownBuilding
	{
		FVector Base;
		FVector Size;
		FLinearColor Color;
	};

	/** 1011 마을 배치: 5×5 건물. 빌더(합친 프록시 메시)와 데모(개별 건물)가 같은 값을 쓴다 */
	inline TArray<FTownBuilding> GetTownBuildings()
	{
		TArray<FTownBuilding> Out;
		FRandomStream Rand(1011);
		for (int32 X = 0; X < 5; ++X)
		{
			for (int32 Y = 0; Y < 5; ++Y)
			{
				FTownBuilding B;
				B.Base = FVector((X - 2) * 700.f, (Y - 2) * 700.f, 0.f);
				B.Size = FVector(Rand.FRandRange(300.f, 500.f), Rand.FRandRange(300.f, 500.f), Rand.FRandRange(300.f, 1200.f));
				const float Hue = Rand.FRandRange(0.f, 360.f);
				B.Color = FLinearColor::MakeFromHSV8(uint8(Hue / 360.f * 255.f), 110, 200);
				Out.Add(B);
			}
		}
		return Out;
	}
}
