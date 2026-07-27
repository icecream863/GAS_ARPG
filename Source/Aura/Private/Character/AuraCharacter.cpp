// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacter.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UObject/ConstructorHelpers.h"

AAuraCharacter::AAuraCharacter()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetUsingAbsoluteRotation(true);	
	CameraBoom->bDoCollisionTest = false;
	
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>("TopDownCamera");
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	
	
	LevelUpNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("LevelUpNiagaraComponent");
	LevelUpNiagaraComponent->SetupAttachment(GetRootComponent());
	LevelUpNiagaraComponent->bAutoActivate = false;

	PassiveEffectAttachComponent = CreateDefaultSubobject<USceneComponent>("PassiveEffectAttachComponent");
	PassiveEffectAttachComponent->SetupAttachment(GetRootComponent());
	// 【优化】使用绝对旋转代替课程中的每帧 SetWorldRotation，避免仅为三个特效永久开启角色 Tick。
	PassiveEffectAttachComponent->SetUsingAbsoluteRotation(true);

	HaloOfProtectionNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>(
		"HaloOfProtectionNiagaraComponent");
	HaloOfProtectionNiagaraComponent->SetupAttachment(PassiveEffectAttachComponent);
	HaloOfProtectionNiagaraComponent->PassiveSpellTag =
		FAuraGameplayTags::Get().Abilities_Passive_HaloOfProtection;

	LifeSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>(
		"LifeSiphonNiagaraComponent");
	LifeSiphonNiagaraComponent->SetupAttachment(PassiveEffectAttachComponent);
	LifeSiphonNiagaraComponent->PassiveSpellTag =
		FAuraGameplayTags::Get().Abilities_Passive_LifeSiphon;

	ManaSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>(
		"ManaSiphonNiagaraComponent");
	ManaSiphonNiagaraComponent->SetupAttachment(PassiveEffectAttachComponent);
	ManaSiphonNiagaraComponent->PassiveSpellTag =
		FAuraGameplayTags::Get().Abilities_Passive_ManaSiphon;

	// 【优化】标签和固定资产在 C++ 统一配置，三个角色蓝图组件无需再分别填写 Class Defaults。
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HaloSystem(
		TEXT("/Game/Assets/Effects/Stun/NS_Halo.NS_Halo"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LifeSiphonSystem(
		TEXT("/Game/Assets/Effects/Stun/NS_LifeSiphon.NS_LifeSiphon"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ManaSiphonSystem(
		TEXT("/Game/Assets/Effects/Stun/NS_ManaSiphon.NS_ManaSiphon"));

	if (HaloSystem.Succeeded())
	{
		HaloOfProtectionNiagaraComponent->SetAsset(HaloSystem.Object);
	}
	if (LifeSiphonSystem.Succeeded())
	{
		LifeSiphonNiagaraComponent->SetAsset(LifeSiphonSystem.Object);
	}
	if (ManaSiphonSystem.Succeeded())
	{
		ManaSiphonNiagaraComponent->SetAsset(ManaSiphonSystem.Object);
	}
	
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bConstrainToPlane = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	CharacterClass = ECharacterClass::Elementalist;
}

void AAuraCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AAuraCharacter::InitAbilityActorInfo()
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	AuraPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(AuraPlayerState,this);
	Cast<UAuraAbilitySystemComponent>(AuraPlayerState->GetAbilitySystemComponent())->AbilityActorInfoSet();
	
	AbilitySystemComponent = AuraPlayerState->GetAbilitySystemComponent();
	AttributeSet = AuraPlayerState->GetAttributeSet();
	OnASCRegistered.Broadcast(AbilitySystemComponent);
	RegisterDebuffTagEvents();
	if (!HasAuthority())
	{
		// RepNotify 可能早于 ASC 初始化；ActorInfo 就绪后再同步一次本地输入标签。
		OnRep_Stunned();
	}
	
	if (AAuraPlayerController* AuraPlayerController = Cast<AAuraPlayerController>(GetController()) )
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(AuraPlayerController->GetHUD()) )
		{
			AuraHUD->InitOverlay(AuraPlayerState, AuraPlayerController, AbilitySystemComponent, AttributeSet);//参数在上面获取
		}
	}
	
	InitialDefaultAttributes();//初始化默认属性
}

void AAuraCharacter::OnRep_Stunned()
{
	Super::OnRep_Stunned();

	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	FGameplayTagContainer BlockedInputTags;
	BlockedInputTags.AddTag(GameplayTags.Player_Block_CursorTrace);
	BlockedInputTags.AddTag(GameplayTags.Player_Block_InputPressed);
	BlockedInputTags.AddTag(GameplayTags.Player_Block_InputHeld);
	BlockedInputTags.AddTag(GameplayTags.Player_Block_InputReleased);

	if (bIsStunned)
	{
		AbilitySystemComponent->AddLooseGameplayTags(BlockedInputTags);
	}
	else
	{
		AbilitySystemComponent->RemoveLooseGameplayTags(BlockedInputTags);
	}
}


void AAuraCharacter::LevelUp_Implementation()
{
	MulticastLevelUpParticles();
}

void AAuraCharacter::MulticastLevelUpParticles_Implementation()
{
	if (LevelUpNiagaraComponent)
	{
		const FVector CameraLocation = TopDownCameraComponent->GetComponentLocation();
		const FVector NiagaraSystemLocation = TopDownCameraComponent->GetComponentLocation();
		const FRotator ToCameraRotation = (CameraLocation - NiagaraSystemLocation).Rotation();
	
		LevelUpNiagaraComponent->SetWorldRotation(ToCameraRotation);
		LevelUpNiagaraComponent->Activate(true);
	}
}


void AAuraCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// Init Ability Actor Info for Server
	InitAbilityActorInfo();
	
	AddCharacterAbilities();
}

void AAuraCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// Init Ability Actor Info for Client
	InitAbilityActorInfo();
}

void AAuraCharacter::AddToXP_Implementation(int32 InXP)
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	AuraPlayerState->AddToXP(InXP);
}


void AAuraCharacter::AddToPlayerLevel_Implementation(int32 InPlayerLevel)
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	AuraPlayerState->AddToLevel(InPlayerLevel); // 广播等级
	
	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()) )
	{
		AuraASC->UpdateAbilityStatuses(AuraPlayerState->GetPlayerLevel()); // 更新技能状态(广播)
	}
	
}

void AAuraCharacter::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	AuraPlayerState->AddToAttributePoints(InAttributePoints);
}

void AAuraCharacter::AddToSpellPoints_Implementation(int32 InSpellPoints)
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	AuraPlayerState->AddToSpellPoints(InSpellPoints);
}

int32 AAuraCharacter::FindLevelForXP_Implementation(int32 InXP) const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->LevelUpInfo->FindLevelForXP(InXP);
}

int32 AAuraCharacter::GetXP_Implementation() const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->GetPlayerXP();
}

int32 AAuraCharacter::GetSpellPoints_Implementation() const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->GetSpellPoints();
}

int32 AAuraCharacter::GetAttributePoints_Implementation() const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->GetAttributePoints();
}

int32 AAuraCharacter::GetAttributePointsReward_Implementation(int Level) const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].AttributePointAward;
}

int32 AAuraCharacter::GetSpellPointsReward_Implementation(int Level) const
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].SpellPointAward;
}

int32 AAuraCharacter::GetPlayerLevel_Implementation()
{
	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);
	return AuraPlayerState->GetPlayerLevel();
	
}
