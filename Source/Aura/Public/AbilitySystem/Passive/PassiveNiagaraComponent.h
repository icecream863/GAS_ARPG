// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NiagaraComponent.h"
#include "PassiveNiagaraComponent.generated.h"

class UAuraAbilitySystemComponent;

/**
 * 被动技能的常驻 Niagara 组件。
 * ASC 在所有网络端广播被动技能状态后，只有标签完全匹配的组件会响应。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AURA_API UPassiveNiagaraComponent : public UNiagaraComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	UPassiveNiagaraComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Ability")
	FGameplayTag PassiveSpellTag;

protected:


private:
	void RegisterWithASC(UAuraAbilitySystemComponent* AuraASC);
	/** 若该被动技能状态为 Equipped 且起始能力已授予，直接激活 Niagara。 */
	void ActivateIfEquipped(UAuraAbilitySystemComponent* AuraASC);
	void SyncWithASC();
	void OnPassiveActivate(const FGameplayTag& AbilityTag, bool bActivate);

	UFUNCTION()
	void OnOwnerDeath(AActor* DeadActor);

	TWeakObjectPtr<UAuraAbilitySystemComponent> BoundAuraASC;
};
