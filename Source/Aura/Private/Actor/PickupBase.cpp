#include "Actor/PickupBase.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AAuraPickupBase::AAuraPickupBase()
{
	// 【修复】bCanEverTick 必须为 true，SetActorTickEnabled(true) 才能真正启用 Tick
	// （UE 的 SetTickFunctionEnable 在 bCanEverTick=false 时什么都不做）。
	// 用 bStartWithTickEnabled=false 实现"按需 Tick"：只有调用 Start*/出生动画才开启。
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 【优化】课程分别在 BP_Pickup_Base / BP_HealthPotion 里手动指定音效；
	// 这里统一给默认药水音效（晶体也复用），蓝图仍可逐个覆盖。
	static ConstructorHelpers::FObjectFinder<USoundBase> SpawnSoundAsset(
		TEXT("/Game/Assets/Sounds/Potions/Spawn/sfx_Potion_Spawn.sfx_Potion_Spawn"));
	if (SpawnSoundAsset.Succeeded())
	{
		SpawnSound = SpawnSoundAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> GroundImpactSoundAsset(
		TEXT("/Game/Assets/Sounds/Potions/HitGround/sfx_Potion_HitGround.sfx_Potion_HitGround"));
	if (GroundImpactSoundAsset.Succeeded())
	{
		GroundImpactSound = GroundImpactSoundAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ConsumeSoundAsset(
		TEXT("/Game/Assets/Sounds/Potions/Consume/sfx_Potion_Consume.sfx_Potion_Consume"));
	if (ConsumeSoundAsset.Succeeded())
	{
		ConsumeSound = ConsumeSoundAsset.Object;
	}
}

void AAuraPickupBase::BeginPlay()
{
	Super::BeginPlay();

	// 记录出生位置/朝向作为浮动与旋转的基准。
	InitialLocation = GetActorLocation();
	CalculatedLocation = InitialLocation;
	CalculatedRotation = GetActorRotation();

	// 出生音效：生成瞬间播放一次。
	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
	}

	if (bStartWithPickupMovement)
	{
		// 【优化】课程在 BeginPlay 直接播放 Timeline，结束后才启动旋转/浮动。
		// 这里先播放出生动画（弹跳+缩放），动画结束再由 Tick 切换到旋转/浮动。
		bSpawnAnimationActive = true;
		SpawnTime = 0.f;
		SetActorTickEnabled(true);
	}
}

void AAuraPickupBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 出生动画阶段：先弹跳+缩放，完成后自动启动旋转/正弦浮动。
	if (bSpawnAnimationActive)
	{
		SpawnTime += DeltaTime;
		const float T = SpawnTime / SpawnDuration;

		if (T >= 1.f)
		{
			bSpawnAnimationActive = false;
			// 【优化】允许按需微调最终停留高度（例如贴地），浮动阶段以该位置为基准。
			SetActorLocation(InitialLocation + FVector(0.f, 0.f, GroundRestingZOffset));
			SetActorScale3D(FVector::OneVector);
			StartRotation();
			StartSinusoidalMovement();
			return;
		}

		// 高度曲线：前半段正弦抛物线飞起再落下，末段轻微负向反弹（undershoot）。
		float Elevation = 0.f;
		if (T <= 0.8f)
		{
			Elevation = SpawnApex * FMath::Sin(PI * T / 0.8f);
		}
		else
		{
			Elevation = -SpawnApex * SpawnBounceAmount * FMath::Sin(PI * (T - 0.8f) / 0.2f);

			// 落地音效：进入负向反弹区间后播放一次（对应课程“elevation < -0.1”的时机）。
			if (!bGroundImpactPlayed && GroundImpactSound)
			{
				bGroundImpactPlayed = true;
				UGameplayStatics::PlaySoundAtLocation(this, GroundImpactSound, GetActorLocation());
			}
		}

		// 缩放曲线：出生瞬间快速放大过冲再回落，落地时再轻微起伏。
		float Scale = 1.f;
		if (T <= 0.25f)
		{
			Scale += SpawnScaleOvershoot * FMath::Sin(PI * T / 0.25f);
		}
		else if (T >= 0.8f)
		{
			Scale += 0.1f * FMath::Sin(PI * (T - 0.8f) / 0.2f);
		}

		CalculatedLocation = InitialLocation + FVector(0.f, 0.f, Elevation);
		SetActorLocation(CalculatedLocation);
		SetActorScale3D(FVector(Scale));
		return;
	}

	RunningTime += DeltaTime;

	// 每到正弦一个周期就把计时归零，避免浮点溢出导致跳动。
	const float SinePeriod = 2.f * PI / SinePeriodConstant;
	if (RunningTime > SinePeriod)
	{
		RunningTime = 0.f;
	}

	ItemMovement(DeltaTime);
}

void AAuraPickupBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 拾取音效：被拾取销毁（Destroyed）时播放；切关卡等其他销毁原因不播。
	if (EndPlayReason == EEndPlayReason::Destroyed && ConsumeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ConsumeSound, GetActorLocation());
	}

	Super::EndPlay(EndPlayReason);
}

void AAuraPickupBase::StartRotation()
{
	bRotates = true;
	CalculatedRotation = GetActorRotation();
	CalculatedLocation = InitialLocation;

	// 【优化】C++ 直接接管 Tick，无需在蓝图里启用 Event Tick。
	SetActorTickEnabled(true);
}

void AAuraPickupBase::StartSinusoidalMovement()
{
	bSinusoidalMovement = true;
	InitialLocation = GetActorLocation();
	CalculatedLocation = InitialLocation;
	RunningTime = 0.f;

	SetActorTickEnabled(true);
}

void AAuraPickupBase::ItemMovement(const float DeltaTime)
{
	if (bRotates)
	{
		const FRotator DeltaRotation(0.f, DeltaTime * RotationRate, 0.f);
		CalculatedRotation = CalculatedRotation + DeltaRotation;
	}

	if (bSinusoidalMovement)
	{
		// sin(时间 × 周期常数) × 振幅，仅作用于 Z 轴。
		const float Sine = FMath::Sin(RunningTime * SinePeriodConstant) * SineAmplitude;
		CalculatedLocation = InitialLocation + FVector(0.f, 0.f, Sine);
	}

	// 【优化】课程在蓝图 Event Tick 里 SetActorLocation/Rotation；
	// 这里在 C++ 直接应用，拾取物无需任何蓝图节点即可旋转浮动。
	SetActorLocation(CalculatedLocation);
	SetActorRotation(CalculatedRotation);
}
