// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/AuraMainMenuDisplay.h"

#include "Animation/AnimSequence.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Sound/SoundBase.h"
#include "UI/Widget/AuraMainMenuWidget.h"

AAuraMainMenuDisplay::AAuraMainMenuDisplay()
{
	// 【优化】课程在蓝图 Event Tick 中更新两个循环动画；集中到 C++ 后，
	// 蓝图只保留美术配置，避免主菜单展示逻辑散落在较长的事件图中。
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AuraMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AuraMesh"));
	AuraMesh->SetupAttachment(SceneRoot);
	AuraMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StaffMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("StaffMesh"));
	StaffMesh->SetupAttachment(AuraMesh, TEXT("WeaponHandSocket"));
	StaffMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FireballComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireballComponent"));
	FireballComponent->SetupAttachment(SceneRoot);

	MagicCircleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("MagicCircleDecal"));
	MagicCircleDecal->SetupAttachment(SceneRoot);

	TitleWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TitleWidgetComponent"));
	TitleWidgetComponent->SetupAttachment(SceneRoot);
	TitleWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	TitleWidgetComponent->SetDrawAtDesiredSize(true);

	// 【优化】该类只服务于固定的 Aura 主菜单展示，将课程中的蓝图 Class Defaults
	// 收口到 C++，避免子蓝图漏配资产；蓝图仍可覆盖组件 Transform 完成构图。
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> AuraMeshAsset(
		TEXT("/Game/Assets/Characters/Aura/SKM_Aura.SKM_Aura"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AuraPoseAsset(
		TEXT("/Game/Assets/Characters/Aura/Animations/AuraPose.AuraPose"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> StaffMeshAsset(
		TEXT("/Game/Assets/Characters/Aura/Staff/SKM_Staff.SKM_Staff"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireballSystem(
		TEXT("/Game/Assets/Effects/Projectiles/FireBolt/NS_Fire_4.NS_Fire_4"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MagicCircleMaterial(
		TEXT("/Game/Assets/MagicCircles/M_MagicCircle_1.M_MagicCircle_1"));
	static ConstructorHelpers::FClassFinder<UUserWidget> TitleWidgetClass(
		TEXT("/Game/BluePrints/MainMenu/WBP_Title"));
	static ConstructorHelpers::FObjectFinder<USoundBase> MainMenuMusicAsset(
		TEXT("/Game/Assets/Sounds/Music/Music_MainMenu.Music_MainMenu"));

	if (AuraMeshAsset.Succeeded())
	{
		AuraMesh->SetSkeletalMesh(AuraMeshAsset.Object);
	}
	if (AuraPoseAsset.Succeeded())
	{
		// PlayAnimation 会配置 Single Node 模式，并按第二个参数循环播放该姿势动画。
		AuraMesh->PlayAnimation(AuraPoseAsset.Object, true);
	}
	if (StaffMeshAsset.Succeeded())
	{
		StaffMesh->SetSkeletalMesh(StaffMeshAsset.Object);
	}
	if (FireballSystem.Succeeded())
	{
		FireballComponent->SetAsset(FireballSystem.Object);
	}
	if (MagicCircleMaterial.Succeeded())
	{
		MagicCircleDecal->SetDecalMaterial(MagicCircleMaterial.Object);
	}
	if (TitleWidgetClass.Succeeded())
	{
		TitleWidgetComponent->SetWidgetClass(TitleWidgetClass.Class);
	}
	if (MainMenuMusicAsset.Succeeded())
	{
		MainMenuMusic = MainMenuMusicAsset.Object;
	}

	// 【优化】原生类是始终可用的后备界面；WBP_MainMenu 只负责提供可视化外壳。
	MainMenuWidgetClass = UAuraMainMenuWidget::StaticClass();
}

void AAuraMainMenuDisplay::BeginPlay()
{
	Super::BeginPlay();

	// 【优化】使用相对位置而不是课程中的世界位置。
	// 这样在关卡中移动整个展示 Actor 后，火球仍会围绕蓝图里配置的局部位置浮动。
	FireballInitialRelativeLocation = FireballComponent->GetRelativeLocation();

	// 【优化】课程把音乐、Create Widget 和输入模式串在蓝图 BeginPlay 中；
	// 这些稳定的生命周期逻辑放入 C++，避免展示蓝图承担流程控制。
	StartMainMenuMusic();
	CreateMainMenuWidget();
}

void AAuraMainMenuDisplay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAnimationTime(DeltaSeconds);
	UpdateFireballLocation();
	UpdateMagicCircleRotation(DeltaSeconds);
}

void AAuraMainMenuDisplay::UpdateAnimationTime(float DeltaSeconds)
{
	// 正弦函数以 2PI 为一个完整周期；取余后数值始终保持在一个周期内。
	AnimationTime = FMath::Fmod(AnimationTime + DeltaSeconds, UE_TWO_PI);
}

void AAuraMainMenuDisplay::UpdateFireballLocation() const
{
	const float HeightOffset = FMath::Sin(AnimationTime) * FireballAmplitude;
	FireballComponent->SetRelativeLocation(
		FireballInitialRelativeLocation + FVector(0.f, 0.f, HeightOffset));
}

void AAuraMainMenuDisplay::UpdateMagicCircleRotation(float DeltaSeconds) const
{
	const float RollThisFrame = MagicCircleRotationRate * DeltaSeconds;
	MagicCircleDecal->AddLocalRotation(FRotator(0.f, 0.f, RollThisFrame));
}

void AAuraMainMenuDisplay::StartMainMenuMusic()
{
	if (MainMenuMusic)
	{
		MainMenuMusicComponent = UGameplayStatics::SpawnSound2D(this, MainMenuMusic);
	}
}

void AAuraMainMenuDisplay::CreateMainMenuWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	// 【优化】蓝图外壳是可选项。延迟加载避免它尚未创建时在 CDO 构造阶段产生硬引用错误。
	TSubclassOf<UAuraMainMenuWidget> WidgetClassToCreate = MainMenuWidgetClass;
	const FSoftClassPath MainMenuWidgetPath(TEXT("/Game/BluePrints/MainMenu/WBP_MainMenu.WBP_MainMenu_C"));
	if (UClass* LoadedWidgetClass = MainMenuWidgetPath.TryLoadClass<UAuraMainMenuWidget>())
	{
		WidgetClassToCreate = LoadedWidgetClass;
	}

	MainMenuWidget = CreateWidget<UAuraMainMenuWidget>(PlayerController, WidgetClassToCreate);
	if (!MainMenuWidget)
	{
		return;
	}

	MainMenuWidget->AddToViewport();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
