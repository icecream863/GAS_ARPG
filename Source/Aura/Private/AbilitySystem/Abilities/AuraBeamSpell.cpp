// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/AuraBeamSpell.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueFunctionLibrary.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

FString UAuraBeamSpell::GetDescription(int32 Level)
{
	const int32 ScaledDamage = FMath::RoundToInt(Damage.GetValueAtLevel(Level));
	const float ManaCost = GetManaCost(Level);
	const float Cooldown = GetCooldown(Level);
	// 总目标数包含首目标；等级 1 命中 1 个，之后随等级增加，但不超过技能上限。
	const int32 NumTargets = FMath::Clamp(Level, 1, MaxNumShockTargets);

	return FString::Printf(
		TEXT("<Title>ELECTROCUTE</>\n\n")
		TEXT("<Small>Level: </><Level>%d</>\n")
		TEXT("<Small>ManaCost: </><ManaCost>%.1f</>\n")
		TEXT("<Small>Cooldown: </><Cooldown>%.1f</>\n\n")
		TEXT("<Default>Channels lightning through up to </><Level>%d</>")
		TEXT("<Default> target%s, dealing </><Damage>%d</>")
		TEXT("<Default> lightning damage per tick. Releasing the spell applies a final hit with a chance to stun.</>"),
		Level,
		ManaCost,
		Cooldown,
		NumTargets,
		NumTargets == 1 ? TEXT("") : TEXT("s"),
		ScaledDamage);
}

FString UAuraBeamSpell::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = FMath::RoundToInt(Damage.GetValueAtLevel(Level));
	const float ManaCost = GetManaCost(Level);
	const float Cooldown = GetCooldown(Level);
	// 调用方传入要预览的下一等级，因此所有数值都必须使用 Level 重新求值。
	const int32 NumTargets = FMath::Clamp(Level, 1, MaxNumShockTargets);

	return FString::Printf(
		TEXT("<Title>NEXT LEVEL:</>\n\n")
		TEXT("<Small>Level: </><Level>%d</>\n")
		TEXT("<Small>ManaCost: </><ManaCost>%.1f</>\n")
		TEXT("<Small>Cooldown: </><Cooldown>%.1f</>\n\n")
		TEXT("<Default>Channels lightning through up to </><Level>%d</>")
		TEXT("<Default> targets, dealing </><Damage>%d</>")
		TEXT("<Default> lightning damage per tick. Releasing the spell applies a final hit with a chance to stun.</>"),
		Level,
		ManaCost,
		Cooldown,
		NumTargets,
		ScaledDamage);
}

void UAuraBeamSpell::StoreMouseDataInfo(const FHitResult& HitResult)
{
	if (HitResult.bBlockingHit)
	{
		// ImpactPoint 是射线真正触碰到表面的世界坐标；后续 Beam 的终点必须使用它，
		// 而不是鼠标射线方向或 ImpactNormal。
		MouseHitLocation = HitResult.ImpactPoint;
		MouseHitActor = HitResult.GetActor();
		return;
	}

	// TargetDataUnderMouse 在没有碰撞命中时仍可能回调。此时继续执行会让后续逻辑
	// 使用默认 FVector(0,0,0)，所以取消当前预测技能并把取消同步给服务端。
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void UAuraBeamSpell::StoreOwnerVariables()
{
	// CurrentActorInfo 由 GAS 在本次 Ability 激活时填充；不要从世界中重新查找控制器，
	// 否则分屏或多人环境可能取得错误的本地玩家。
	OwnerPlayerController = CurrentActorInfo ? CurrentActorInfo->PlayerController.Get() : nullptr;

	// AvatarActor 并不保证一定是 Character（例如某些 AI 或特殊 Pawn），所以用 Cast 并允许为空。
	// Electrocute 的蓝图在取 CharacterMovement 前应保留 IsValid 保护。
	OwnerCharacter = CurrentActorInfo ? Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
}

void UAuraBeamSpell::StoreOwnerPlayerController()
{
	// 旧蓝图节点继续可用，但实际执行新函数，避免漏掉 OwnerCharacter 的初始化。
	StoreOwnerVariables();
}

void UAuraBeamSpell::TraceFirstTarget(const FVector& BeamTargetLocation)
{
	if (!IsValid(OwnerCharacter) || !OwnerCharacter->Implements<UCombatInterface>())
	{
		return;
	}

	USkeletalMeshComponent* Weapon = ICombatInterface::Execute_GetWeapon(OwnerCharacter);
	if (!IsValid(Weapon) || !Weapon->DoesSocketExist(TEXT("TipSocket")))
	{
		return;
	}

	const FVector TraceStart = Weapon->GetSocketLocation(TEXT("TipSocket"));
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	FHitResult HitResult;
	const bool bBlockingHit = UKismetSystemLibrary::SphereTraceSingle(
		OwnerCharacter,
		TraceStart,
		BeamTargetLocation,
		10.f,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResult,
		true);

	if (bBlockingHit)
	{
		// 只在路径中确有阻挡物时改写鼠标目标，避免把未命中结果误写回默认位置。
		MouseHitLocation = HitResult.ImpactPoint;
		MouseHitActor = HitResult.GetActor();
		BindToPrimaryTargetDeath(MouseHitActor);
	}
}

void UAuraBeamSpell::StoreAdditionalTargets(TArray<AActor*>& OutAdditionalTargets) 
{
	OutAdditionalTargets.Reset();
	if (!IsValid(MouseHitActor) || !MouseHitActor->Implements<UCombatInterface>())
	{
		return;
	}

	TArray<AActor*> ActorsToIgnore;
	if (CurrentActorInfo && IsValid(CurrentActorInfo->AvatarActor.Get()))
	{
		ActorsToIgnore.Add(CurrentActorInfo->AvatarActor.Get());
	}
	ActorsToIgnore.Add(MouseHitActor);

	TArray<AActor*> OverlappingActors;
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr,
		OverlappingActors,
		ActorsToIgnore,
		ShockTargetSearchRadius,
		MouseHitActor->GetActorLocation());

	// 总目标数包含首个命中者，因此额外目标数需要减一；等级 1 时不会产生跳链。
	const int32 NumAdditionalTargets = FMath::Min(GetAbilityLevel() - 1, MaxNumShockTargets - 1);
	UAuraAbilitySystemLibrary::GetClosestTargets(
		NumAdditionalTargets,
		OverlappingActors,
		OutAdditionalTargets,
		MouseHitActor->GetActorLocation());

	for (AActor* AdditionalTarget : OutAdditionalTargets)
	{
		BindToAdditionalTargetDeath(AdditionalTarget);
	}
}

void UAuraBeamSpell::AddShockLoopCuesToAdditionalTargets(const TArray<AActor*>& AdditionalTargets)
{
	RemoveAllAdditionalShockLoopCues();

	if (!IsValid(MouseHitActor) || !MouseHitActor->Implements<UCombatInterface>())
	{
		return;
	}

	for (AActor* AdditionalTarget : AdditionalTargets)
	{
		AddShockLoopCueToAdditionalTarget(AdditionalTarget);
	}
}

void UAuraBeamSpell::AddShockLoopCueToAdditionalTarget(AActor* AdditionalTarget)
{
	if (!IsValid(AdditionalTarget) || AdditionalTarget == MouseHitActor)
	{
		return;
	}

	if (!IsValid(MouseHitActor) || !MouseHitActor->Implements<UCombatInterface>())
	{
		// 只有首个命中目标是真正敌人时才允许链式 Cue，防止点击地面时球扫误挂残留特效。
		return;
	}

	if (AdditionalActorsToCueParams.Contains(AdditionalTarget))
	{
		RemoveShockLoopCueFromAdditionalTarget(AdditionalTarget);
	}

	const FGameplayTag CueTag = ShockLoopGameplayCueTag.IsValid()
		? ShockLoopGameplayCueTag
		: FGameplayTag::RequestGameplayTag(FName("GameplayCue.ShockLoop"));

	FGameplayCueParameters CueParameters;
	CueParameters.MatchedTagName = CueTag;
	CueParameters.OriginalTag = CueTag;
	CueParameters.Location = AdditionalTarget->GetActorLocation();
	CueParameters.Instigator = GetAvatarActorFromActorInfo();
	CueParameters.EffectCauser = GetAvatarActorFromActorInfo();
	CueParameters.SourceObject = AdditionalTarget;
	CueParameters.AbilityLevel = GetAbilityLevel();
	CueParameters.GameplayEffectLevel = GetAbilityLevel();
	CueParameters.TargetAttachComponent = MouseHitActor->GetRootComponent();

	UGameplayCueFunctionLibrary::AddGameplayCueOnActor(AdditionalTarget, CueTag, CueParameters);

	AdditionalShockLoopTargets.AddUnique(AdditionalTarget);
	AdditionalActorsToCueParams.Add(AdditionalTarget, CueParameters);
}

void UAuraBeamSpell::RemoveShockLoopCueFromAdditionalTarget(AActor* AdditionalTarget)
{
	if (!IsValid(AdditionalTarget))
	{
		AdditionalShockLoopTargets.Remove(AdditionalTarget);
		AdditionalActorsToCueParams.Remove(AdditionalTarget);
		return;
	}

	const FGameplayTag CueTag = ShockLoopGameplayCueTag.IsValid()
		? ShockLoopGameplayCueTag
		: FGameplayTag::RequestGameplayTag(FName("GameplayCue.ShockLoop"));

	if (const FGameplayCueParameters* CueParameters = AdditionalActorsToCueParams.Find(AdditionalTarget))
	{
		// Remove 必须复用 Add 时的参数；Cue 实例查找会把参数作为匹配条件的一部分。
		UGameplayCueFunctionLibrary::RemoveGameplayCueOnActor(AdditionalTarget, CueTag, *CueParameters);
	}

	AdditionalShockLoopTargets.Remove(AdditionalTarget);
	AdditionalActorsToCueParams.Remove(AdditionalTarget);
}

void UAuraBeamSpell::RemoveAllAdditionalShockLoopCues()
{
	TArray<TObjectPtr<AActor>> TargetsToRemove = AdditionalShockLoopTargets;
	for (AActor* Target : TargetsToRemove)
	{
		RemoveShockLoopCueFromAdditionalTarget(Target);
	}

	AdditionalShockLoopTargets.Reset();
	AdditionalActorsToCueParams.Reset();
}

void UAuraBeamSpell::ApplyPeriodicDamage(const TArray<AActor*>& AdditionalTargets)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	if (IsValid(MouseHitActor) && MouseHitActor->Implements<UCombatInterface>())
	{
		MarkTargetBeingShocked(MouseHitActor);
		CauseDamageWithoutDebuff(MouseHitActor);
	}

	// GameplayEffect 可能同步杀死目标，并在死亡回调中修改蓝图的 AdditionalTargets。
	// 遍历快照可避免原数组 Remove Item 后使当前 range-for 的迭代器失效。
	const TArray<AActor*> TargetsSnapshot = AdditionalTargets;
	for (AActor* AdditionalTarget : TargetsSnapshot)
	{
		if (IsValid(AdditionalTarget) && AdditionalTarget != MouseHitActor)
		{
			MarkTargetBeingShocked(AdditionalTarget);
			CauseDamageWithoutDebuff(AdditionalTarget); 
		}
	}
}

void UAuraBeamSpell::ApplyFinalDamageToTargets(const TArray<AActor*>& AdditionalTargets)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority())
	{
		return;
	}

	// 完整伤害可能同步触发死亡回调，因此在第一次 Apply 前就固定本轮目标快照。
	const TArray<AActor*> TargetsSnapshot = AdditionalTargets;

	auto ApplyToLiveTarget = [this](AActor* Target)
	{
		if (!IsValid(Target) || !Target->Implements<UCombatInterface>() ||
			ICombatInterface::Execute_IsDead(Target))
		{
			return;
		}
		CauseDamage(Target); // CauseDamage 里调用了 Apply Damage Effect
	};

	ApplyToLiveTarget(MouseHitActor);
	for (AActor* AdditionalTarget : TargetsSnapshot)
	{
		if (AdditionalTarget != MouseHitActor)
		{
			ApplyToLiveTarget(AdditionalTarget);
		}
	}
}

void UAuraBeamSpell::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearBeingShockedTargets();
	// InstancedPerActor 会复用同一个 Ability UObject；若不解绑，下一次施法前旧目标死亡也会回调到这里。
	UnbindDeathDelegates();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraBeamSpell::MarkTargetBeingShocked(AActor* Target)
{
	if (!IsValid(Target) || !Target->Implements<UCombatInterface>())
	{
		return;
	}

	ICombatInterface::Execute_SetIsBeingShocked(Target, true);
	BeingShockedTargets.Add(Target);
}

void UAuraBeamSpell::ClearBeingShockedTargets()
{
	for (const TWeakObjectPtr<AActor>& Target : BeingShockedTargets)
	{
		if (Target.IsValid() && Target->Implements<UCombatInterface>())
		{
			ICombatInterface::Execute_SetIsBeingShocked(Target.Get(), false);
		}
	}
	BeingShockedTargets.Reset();
}

void UAuraBeamSpell::BindToPrimaryTargetDeath(AActor* Target)
{
	if (BoundPrimaryTarget.Get() != Target)
	{
		UnbindDeathDelegates();
	}

	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Target);
	if (!CombatInterface)
	{
		return;
	}

	FOnDeath& OnDeath = CombatInterface->GetOnDeathDelegate();
	if (!OnDeath.IsAlreadyBound(this, &UAuraBeamSpell::PrimaryTargetDied))
	{
		OnDeath.AddDynamic(this, &UAuraBeamSpell::PrimaryTargetDied);
	}
	BoundPrimaryTarget = Target;
}

void UAuraBeamSpell::BindToAdditionalTargetDeath(AActor* Target)
{
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Target);
	if (!CombatInterface)
	{
		return;
	}

	FOnDeath& OnDeath = CombatInterface->GetOnDeathDelegate();
	if (!OnDeath.IsAlreadyBound(this, &UAuraBeamSpell::AdditionalTargetDied))
	{
		OnDeath.AddDynamic(this, &UAuraBeamSpell::AdditionalTargetDied);
	}
	BoundAdditionalTargets.AddUnique(Target);
}

void UAuraBeamSpell::UnbindDeathDelegates()
{
	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(BoundPrimaryTarget.Get()))
	{
		CombatInterface->GetOnDeathDelegate().RemoveDynamic(this, &UAuraBeamSpell::PrimaryTargetDied);
	}
	BoundPrimaryTarget.Reset();

	for (const TWeakObjectPtr<AActor>& Target : BoundAdditionalTargets)
	{
		if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(Target.Get()))
		{
			CombatInterface->GetOnDeathDelegate().RemoveDynamic(this, &UAuraBeamSpell::AdditionalTargetDied);
		}
	}
	BoundAdditionalTargets.Reset();
}
