// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "Interaction/CombatInterface.h"

UPassiveNiagaraComponent::UPassiveNiagaraComponent()
{
	bAutoActivate = false;
}

void UPassiveNiagaraComponent::BeginPlay()
{
	Super::BeginPlay();

	ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetOwner());
	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner())))
	{
		RegisterWithASC(AuraASC);
	}
	else if (CombatInterface)
	{
		// PlayerState 上的 ASC 可能晚于组件 BeginPlay 才完成 ActorInfo 初始化。
		CombatInterface->GetOnASCRegisteredDelegate().AddWeakLambda(
			this,
			[this](UAbilitySystemComponent* RegisteredASC)
			{
				UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(RegisteredASC);
				RegisterWithASC(AuraASC);
				// ASC 注册较晚时，可能错过了 AbilityGiven 广播，按状态补一次激活。
				ActivateIfEquipped(AuraASC);
			});
	}

	if (CombatInterface)
	{
		CombatInterface->GetOnDeathDelegate().AddDynamic(
			this, &UPassiveNiagaraComponent::OnOwnerDeath);
	}
}

void UPassiveNiagaraComponent::RegisterWithASC(UAuraAbilitySystemComponent* AuraASC)
{
	if (!IsValid(AuraASC) || !PassiveSpellTag.IsValid())
	{
		return;
	}

	if (BoundAuraASC.IsValid())
	{
		BoundAuraASC->ActivatePassiveEffect.RemoveAll(this);
		BoundAuraASC->AbilityGivenDelegate.RemoveAll(this);
	}

	BoundAuraASC = AuraASC;
	AuraASC->ActivatePassiveEffect.AddUObject(
		this, &UPassiveNiagaraComponent::OnPassiveActivate);
	// 客户端的组件可能先于 ActivatableAbilities 完成注册；等 AbilitySpec 复制完成后，
	// 再通过 SyncWithASC 读取对应被动技能的 IsActive()，补齐当前 Niagara 显示状态。
	AuraASC->AbilityGivenDelegate.AddUObject(
		this, &UPassiveNiagaraComponent::SyncWithASC);

	// 【优化】课程只监听后续广播；这里注册时立即同步，避免 ASC/组件初始化较晚导致已激活特效不显示。
	SyncWithASC();
	ActivateIfEquipped(AuraASC);
}

void UPassiveNiagaraComponent::ActivateIfEquipped(UAuraAbilitySystemComponent* AuraASC)
{
	if (!AuraASC || !AuraASC->bStartupAbilitiesGiven || !PassiveSpellTag.IsValid())
	{
		return;
	}

	// 读档恢复被动技能时走的是 GiveAbilityAndActivateOnce，不会经过
	// MulticastActivatePassiveEffect 广播；这里直接按“状态是否 Equipped”补一次激活。
	if (AuraASC->GetStatusFromAbilityTag(PassiveSpellTag)
			.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
	{
		Activate();
	}
}

void UPassiveNiagaraComponent::SyncWithASC()
{
	if (!BoundAuraASC.IsValid())
	{
		return;
	}

	const FGameplayAbilitySpec* Spec = BoundAuraASC->GetAbilitySpecFromTag(PassiveSpellTag);
	OnPassiveActivate(PassiveSpellTag, Spec && Spec->IsActive());
}

void UPassiveNiagaraComponent::OnPassiveActivate(const FGameplayTag& AbilityTag, bool bActivate)
{
	if (!AbilityTag.MatchesTagExact(PassiveSpellTag))
	{
		return;
	}

	const bool bOwnerAlive = IsValid(GetOwner()) &&
		GetOwner()->Implements<UCombatInterface>() &&
		!ICombatInterface::Execute_IsDead(GetOwner());

	if (bActivate && bOwnerAlive)
	{
		if (!IsActive())
		{
			Activate(true);
		}
	}
	else
	{
		Deactivate();
	}
}

void UPassiveNiagaraComponent::OnOwnerDeath(AActor* DeadActor)
{
	Deactivate();
}
