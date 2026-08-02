// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AuraGameplayTags.h"
#include "NiagaraComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
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
	LoadProgress();

	// 玩家进度（属性/能力）恢复后，再恢复世界状态（检查点发光等）。
	if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		AuraGameMode->LoadWorldState(GetWorld());
	}
}

void AAuraCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	// Init Ability Actor Info for Client
	InitAbilityActorInfo();
}

void AAuraCharacter::LoadProgress()
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!AuraGameMode)
	{
		// 【优化】非存档模式下仍保证角色拥有默认属性和能力。
		InitialDefaultAttributes();
		AddCharacterAbilities();
		return;
	}

	ULoadScreenSaveGame* SaveData = AuraGameMode->RetrieveInGameSaveData();
	if (!SaveData)
	{
		// 【优化】存档读取失败时回退到新角色初始化，避免生成无属性角色。
		InitialDefaultAttributes();
		AddCharacterAbilities();
		return;
	}

	if (SaveData->bFirstTimeLoadIn)
	{
		// 【优化】首进这个槽位时仍然走课程原始默认属性链，避免把空存档误当成读档数据。
		InitialDefaultAttributes();
		AddCharacterAbilities();
		return;
	}

	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	check(AuraPlayerState);

	AuraPlayerState->SetLevel(SaveData->PlayerLevel);
	AuraPlayerState->SetXP(SaveData->XP);
	AuraPlayerState->SetSpellPoints(SaveData->SpellPoints);
	AuraPlayerState->SetAttributePoints(SaveData->AttributePoints);

	UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromSaveData(this, AbilitySystemComponent, SaveData);

	// 恢复能力：按存档重建 AbilitySpec（等级/状态/槽位），被动只有 Equipped 才激活。
	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
	{
		AuraASC->AddCharacterAbilitiesFromSaveData(SaveData);
	}
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

void AAuraCharacter::SaveProgress_Implementation(const FName& CheckpointTag)
{
	// 【优化】课程只在保存技能前检查权限；这里提前到函数开头，客户端重叠触发
	// 检查点时直接返回，避免把客户端不完整/未授权数据写盘。
	if (!HasAuthority())
	{
		return;
	}

	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!AuraGameMode)
	{
		return;
	}

	// 先读取当前槽位已有数据，后续属性、技能和经验存档可以继续写入同一个对象。
	ULoadScreenSaveGame* SaveData = AuraGameMode->RetrieveInGameSaveData();
	if (!SaveData)
	{
		return;
	}

	AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
	if (!AuraPlayerState)
	{
		return;
	}

	UAuraAttributeSet* AuraAttributeSet = Cast<UAuraAttributeSet>(GetAttributeSet());
	if (!AuraAttributeSet)
	{
		return;
	}

	SaveData->PlayerStartTag = CheckpointTag;
	SaveData->PlayerLevel = AuraPlayerState->GetPlayerLevel();
	SaveData->XP = AuraPlayerState->GetPlayerXP();
	SaveData->SpellPoints = AuraPlayerState->GetSpellPoints();
	SaveData->AttributePoints = AuraPlayerState->GetAttributePoints();
	SaveData->Strength = AuraAttributeSet->GetStrength();
	SaveData->Intelligence = AuraAttributeSet->GetIntelligence();
	SaveData->Resilience = AuraAttributeSet->GetResilience();
	SaveData->Vigor = AuraAttributeSet->GetVigor();
	SaveData->bFirstTimeLoadIn = false;

	// ---- 保存能力：把 ActivatableAbilities 里每个 Spec 转成 FSavedAbility 存档 ----
	UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this);
	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent);
	if (!AbilityInfo || !AuraASC)
	{
		return;
	}

	// 先清空再填充，避免旧存档里已删除/已更换的能力残留。
	SaveData->SavedAbilities.Empty();

	FForEachAbility SaveAbilityDelegate;
	SaveAbilityDelegate.BindLambda(
		[SaveData, AbilityInfo, AuraASC](FGameplayAbilitySpec& AbilitySpec)
		{
			// 先从 Spec 的动态标签里取出能力标签（GetAbilityTagFromSpec 是静态函数，用实例调用也可）。
			const FGameplayTag AbilityTag = AuraASC->GetAbilityTagFromSpec(AbilitySpec);
			// 再按标签到 AbilityInfo 数据资产里反查能力类、能力类型等配置。
			const FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);

			FSavedAbility SavedAbility;
			SavedAbility.GameplayAbility = Info.Ability;
			SavedAbility.AbilityTag = AbilityTag;
			SavedAbility.AbilityStatus = AuraASC->GetStatusFromAbilityTag(AbilityTag);
			SavedAbility.AbilitySlot = AuraASC->GetSlotFromAbilityTag(AbilityTag);
			SavedAbility.AbilityType = Info.AbilityType;
			SavedAbility.AbilityLevel = AbilitySpec.Level;

			// AddUnique 依赖 FSavedAbility 的 operator==（按 AbilityTag 比较），
			// 防止同一技能被重复写入存档。
			SaveData->SavedAbilities.AddUnique(SavedAbility);
		});
	AuraASC->ForEachAbility(SaveAbilityDelegate);

	AuraGameMode->SaveInGameProgressData(SaveData);
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
