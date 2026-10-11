#include "Demo1101_UPropertyUFunction.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GlossaryKit.h"
#include "UObject/UnrealType.h"

using namespace GlossaryKit;

// ================================================================ AD1101_Cannon

AD1101_Cannon::AD1101_Cannon()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	InitShapeComponent(Body, EShape::Cube, FVector(160.f, 160.f, 120.f));
	Body->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(Root);
	InitShapeComponent(Barrel, EShape::Cylinder, FVector(50.f, 50.f, 180.f));
	// 원기둥은 Z 축으로 서 있으므로 눕혀서 +X 를 향하게
	Barrel->SetRelativeLocationAndRotation(FVector(110.f, 0.f, 120.f), FRotator(-90.f, 0.f, 0.f));
}

void AD1101_Cannon::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	SetColor(Body, Colors::Accent);
	SetColor(Barrel, Colors::Dark, 0.3f, 0.8f);
}

void AD1101_Cannon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Speed 는 음수가 될 수도 있다(C++ 에서 직접 넣으면 ClampMin 이 막지 않으므로) → 핑퐁 계산이 음수도 처리
	Phase += Speed * DeltaSeconds;
	const float Span = TravelWidth * 2.f;
	float P = FMath::Fmod(Phase, Span);
	if (P < 0.f)
	{
		P += Span;
	}
	const float Offset = (P < TravelWidth ? P : Span - P) - TravelWidth * 0.5f;
	SetActorLocation(StartLocation + FVector(0.f, Offset, 0.f));

	if (bAutoFire)
	{
		FireTimer += DeltaSeconds;
		if (FireTimer >= FireInterval)
		{
			FireTimer = 0.f;
			Fire();
		}
	}

	Heat = FMath::Max(0.f, Heat - DeltaSeconds * 0.4f);
	if (FlashTimer > 0.f)
	{
		FlashTimer -= DeltaSeconds;
		if (FlashTimer <= 0.f)
		{
			SetColor(Body, Colors::Accent);
		}
	}
}

void AD1101_Cannon::Fire()
{
	++ShotsFired;
	Heat += 0.25f;

	// 포탄: 물리 구체를 앞으로 쏜다(2초 뒤 사라짐)
	const FVector Muzzle = Barrel->GetComponentLocation() + GetActorForwardVector() * 110.f;
	if (AStaticMeshActor* Ball = SpawnShape(GetWorld(), EShape::Sphere, Muzzle, FVector(40.f), Colors::Item, FRotator::ZeroRotator, true))
	{
		Ball->GetStaticMeshComponent()->SetPhysicsLinearVelocity(GetActorForwardVector() * 1400.f + FVector(0.f, 0.f, 350.f));
		Ball->SetLifeSpan(2.f);
	}

	// BlueprintNativeEvent 는 이름 그대로 부른다. 생성된 썽크가 BP 덮어쓰기가 있으면 그것을, 없으면 _Implementation 을 실행
	OnFired(ShotsFired);

	if (Heat > 1.f)
	{
		Heat = 0.f;
		// BlueprintImplementableEvent: BP 자식이 구현하지 않았으면 아무 일도 일어나지 않는다(크래시도 없음)
		OnOverheated();
	}
}

float AD1101_Cannon::GetShotsPerMinute() const
{
	return bAutoFire && FireInterval > 0.f ? 60.f / FireInterval : 0.f;
}

void AD1101_Cannon::OnFired_Implementation(int32 ShotIndex)
{
	// C++ 기본 동작: 몸통을 잠깐 노랗게
	++NativeEventCalls;
	SetColor(Body, Colors::Item, 0.4f, 0.f, 2.f);
	FlashTimer = 0.08f;
}

void AD1101_Cannon::ResetCounter()
{
	ShotsFired = 0;
	NativeEventCalls = 0;
}

// ================================================================ ADemo1101

ADemo1101_UPropertyUFunction::ADemo1101_UPropertyUFunction()
{
	DemoId = TEXT("1101");
	Controls = TEXT("1 / 2 : Speed -150 / +150 (ClampMin 0 적용 = 디테일 패널처럼)   3 : C++ 로 Speed = -300 직접 대입\n")
		TEXT("F : Fire 를 블루프린트처럼 ProcessEvent 로 호출   A : bAutoFire 켜기/끄기\n")
		TEXT("관찰: 오른쪽 패널은 리플렉션이 기록한 실제 플래그. ClampMin 은 에디터 입력만 막고 C++ 대입은 못 막는다");
	Showcase = TEXT("Speed·Fire 를 노출한 대포, 실행 중 값 조절 + 리플렉션으로 읽은 지정자 플래그 목록");
	PlayerStartTransform = FTransform(FRotator::ZeroRotator, FVector(-700.f, 0.f, 100.f));
}

void ADemo1101_UPropertyUFunction::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Cannon = World->SpawnActor<AD1101_Cannon>(FVector(400.f, 0.f, 0.f), FRotator::ZeroRotator, Params);

	// 이동 범위 표시
	SpawnBlock(World, FVector(400.f, 0.f, 0.f), FVector(220.f, 1100.f, 4.f), Colors::Platform);
	SpawnLabel(World, TEXT("AD1101_Cannon  (Speed · Fire 노출)"), FVector(400.f, 0.f, 330.f), Colors::Item, 20.f);

	Panel = UD11_SidePanel::Create(GetPC(), TEXT("리플렉션이 본 AD1101_Cannon"), FVector2D(1.f, 0.f), 12.f);
	Log.Add(TEXT("시작: Speed 300, FireInterval 0.5 (생성자 기본값)"));
}

void ADemo1101_UPropertyUFunction::SetSpeedLikeDetailsPanel(float NewValue)
{
	// GET_MEMBER_NAME_CHECKED: 이름 오타를 컴파일 타임에 잡는다
	FFloatProperty* SpeedProp = FindFProperty<FFloatProperty>(AD1101_Cannon::StaticClass(), GET_MEMBER_NAME_CHECKED(AD1101_Cannon, Speed));
	if (!SpeedProp || !Cannon)
	{
		return;
	}
#if WITH_METADATA
	// ClampMin/ClampMax 는 메타데이터일 뿐이다. 디테일 패널·BP 핀이 이 값을 읽어 입력을 막는다 — 여기서도 똑같이 적용
	if (SpeedProp->HasMetaData(TEXT("ClampMin")))
	{
		NewValue = FMath::Max(NewValue, SpeedProp->GetFloatMetaData(TEXT("ClampMin")));
	}
	if (SpeedProp->HasMetaData(TEXT("ClampMax")))
	{
		NewValue = FMath::Min(NewValue, SpeedProp->GetFloatMetaData(TEXT("ClampMax")));
	}
#endif
	SpeedProp->SetPropertyValue_InContainer(Cannon, NewValue);
	Log.Add(FString::Printf(TEXT("FProperty 로 Speed = %.0f (ClampMin/Max 적용)"), Cannon->Speed));
}

void ADemo1101_UPropertyUFunction::CallFireLikeBlueprint()
{
	// BP 의 'Fire' 노드는 이름으로 UFunction 을 찾아 ProcessEvent 한다. BlueprintCallable 이 아니면 BP 에 노드가 안 생긴다.
	if (UFunction* FireFunc = Cannon ? Cannon->FindFunction(GET_FUNCTION_NAME_CHECKED(AD1101_Cannon, Fire)) : nullptr)
	{
		Cannon->ProcessEvent(FireFunc, nullptr);
		Log.Add(FString::Printf(TEXT("ProcessEvent(Fire) → ShotsFired %d"), Cannon->ShotsFired));
	}
}

FString ADemo1101_UPropertyUFunction::DescribeReflection() const
{
	if (!Cannon)
	{
		return FString();
	}
	TArray<FString> Lines;
	Lines.Add(TEXT("[UPROPERTY]  이름 = 현재 값   ← 플래그"));
	for (TFieldIterator<FProperty> It(AD1101_Cannon::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Prop = *It;
		if (Prop->IsA<FObjectPropertyBase>())
		{
			continue; // 컴포넌트 포인터(VisibleAnywhere)는 생략
		}
		TArray<FString> Flags;
		if (Prop->HasAnyPropertyFlags(CPF_Edit))
		{
			const TCHAR* Verb = Prop->HasAnyPropertyFlags(CPF_EditConst) ? TEXT("Visible") : TEXT("Edit");
			const TCHAR* Where = Prop->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ? TEXT("DefaultsOnly")
				: Prop->HasAnyPropertyFlags(CPF_DisableEditOnTemplate) ? TEXT("InstanceOnly") : TEXT("Anywhere");
			Flags.Add(FString(Verb) + Where);
		}
		if (Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
		{
			Flags.Add(Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly) ? TEXT("BlueprintReadOnly") : TEXT("BlueprintReadWrite"));
		}
		if (Prop->HasAnyPropertyFlags(CPF_Transient))
		{
			Flags.Add(TEXT("Transient"));
		}
#if WITH_METADATA
		if (Prop->HasMetaData(TEXT("Category")))
		{
			Flags.Add(TEXT("Category=") + Prop->GetMetaData(TEXT("Category")));
		}
		for (const TCHAR* Key : { TEXT("ClampMin"), TEXT("ClampMax"), TEXT("EditCondition") })
		{
			if (Prop->HasMetaData(Key))
			{
				Flags.Add(FString::Printf(TEXT("%s=%s"), Key, *Prop->GetMetaData(Key)));
			}
		}
#endif
		// 값도 리플렉션으로 읽는다(블루프린트 Get 노드·디테일 패널이 하는 일)
		FString Value;
		if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
		{
			Value = FString::Printf(TEXT("%.2f"), FloatProp->GetPropertyValue_InContainer(Cannon));
		}
		else
		{
			Prop->ExportTextItem_Direct(Value, Prop->ContainerPtrToValuePtr<void>(Cannon), nullptr, Cannon, PPF_None);
		}
		Lines.Add(FString::Printf(TEXT("  %s = %s   ← %s"), *Prop->GetName(), *Value, *FString::Join(Flags, TEXT(" · "))));
	}

	Lines.Add(TEXT("[UFUNCTION]"));
	for (TFieldIterator<UFunction> It(AD1101_Cannon::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const UFunction* Func = *It;
		TArray<FString> Flags;
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintEvent))
		{
			// NativeEvent 는 C++ 구현이 있으므로 FUNC_Native 도 붙는다
			Flags.Add(Func->HasAnyFunctionFlags(FUNC_Native) ? TEXT("BlueprintNativeEvent (C++ 은 _Implementation)") : TEXT("BlueprintImplementableEvent (BP 만 구현)"));
		}
		else if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
		{
			Flags.Add(TEXT("BlueprintPure (실행 핀 없음, const)"));
		}
		else if (Func->HasAnyFunctionFlags(FUNC_BlueprintCallable))
		{
			Flags.Add(TEXT("BlueprintCallable (실행 핀 있음)"));
		}
#if WITH_METADATA
		if (Func->HasMetaData(TEXT("CallInEditor")))
		{
			Flags.Add(TEXT("CallInEditor (디테일 패널 버튼)"));
		}
#endif
		Lines.Add(FString::Printf(TEXT("  %s()   ← %s"), *Func->GetName(), *FString::Join(Flags, TEXT(" · "))));
	}
	return FString::Join(Lines, TEXT("\n"));
}

void ADemo1101_UPropertyUFunction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Cannon)
	{
		return;
	}

	if (WasKeyPressed(EKeys::One)) { SetSpeedLikeDetailsPanel(Cannon->Speed - SpeedStep); }
	if (WasKeyPressed(EKeys::Two)) { SetSpeedLikeDetailsPanel(Cannon->Speed + SpeedStep); }
	if (WasKeyPressed(EKeys::Three))
	{
		// C++ 대입은 메타데이터를 거치지 않는다 → ClampMin 0 이어도 음수가 들어간다(대포가 거꾸로 움직임)
		Cannon->Speed = -300.f;
		Log.Add(TEXT("C++ 직접 대입 Speed = -300 → ClampMin 무시됨!"));
	}
	if (WasKeyPressed(EKeys::F)) { CallFireLikeBlueprint(); }
	if (WasKeyPressed(EKeys::A))
	{
		Cannon->bAutoFire = !Cannon->bAutoFire;
		Log.Add(FString::Printf(TEXT("bAutoFire = %s (꺼지면 FireInterval 은 EditCondition 으로 회색)"), Cannon->bAutoFire ? TEXT("true") : TEXT("false")));
	}

	if (Panel)
	{
		Panel->SetBody(DescribeReflection() + TEXT("\n[최근 동작]\n") + Log.Join());
	}
	SetStatus(FString::Printf(TEXT("Speed %.0f cm/s · ShotsFired %d · GetShotsPerMinute() %.0f\nOnFired_Implementation 호출 %d회 (BlueprintNativeEvent 의 C++ 기본 구현)"),
		Cannon->Speed, Cannon->ShotsFired, Cannon->GetShotsPerMinute(), Cannon->NativeEventCalls));
}
