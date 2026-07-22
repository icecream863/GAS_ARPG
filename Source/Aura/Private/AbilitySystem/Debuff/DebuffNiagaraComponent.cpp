// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Interaction/CombatInterface.h"

UDebuffNiagaraComponent::UDebuffNiagaraComponent()
{
	bAutoActivate = false;
}

void UDebuffNiagaraComponent::BeginPlay()
{
	Super::BeginPlay();

	ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetOwner());
	if (UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		RegisterWithASC(ASC);
	}
	else if (CombatInterface) // ASC 尚未注册时，等待角色完成 ASC 注册。
	{
		CombatInterface->GetOnASCRegisteredDelegate().AddWeakLambda(
			this,
			[this](UAbilitySystemComponent* RegisteredASC)
			{
				RegisterWithASC(RegisteredASC);
			});
	}

	if (CombatInterface)
	{
		CombatInterface->GetOnDeathDelegate().AddDynamic(
			this, &UDebuffNiagaraComponent::OnOwnerDeath);
	}
}

void UDebuffNiagaraComponent::RegisterWithASC(UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC) || !DebuffTag.IsValid())
	{
		return;
	}

	ASC->RegisterGameplayTagEvent(DebuffTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UDebuffNiagaraComponent::DebuffTagChanged);

	// Registration can happen after the tag was granted, so synchronize immediately.
	// 注册可能发生在标签被授予之后，因此立即同步。
	DebuffTagChanged(DebuffTag, ASC->GetTagCount(DebuffTag));
}

void UDebuffNiagaraComponent::DebuffTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	const bool bOwnerValid = IsValid(GetOwner());
	const bool bOwnerAlive = bOwnerValid &&
		GetOwner()->Implements<UCombatInterface>() &&
		!ICombatInterface::Execute_IsDead(GetOwner());

	if (NewCount > 0 && bOwnerAlive)
	{
		Activate();
	}
	else
	{
		Deactivate();
	}
}

void UDebuffNiagaraComponent::OnOwnerDeath(AActor* DeadActor)
{
	Deactivate();
}
