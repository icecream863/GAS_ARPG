// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraPassiveAbility.generated.h"

class UAuraAbilitySystemComponent;

/**
 * 被动法术的公共基类。
 * 激活后持续存在，直到 Aura ASC 按 AbilityTag 广播停用请求。
 */
UCLASS()
class AURA_API UAuraPassiveAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UAuraPassiveAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	/** 收到 ASC 的停用请求时，只结束 AssetTags 中拥有相同 AbilityTag 的被动技能。 */
	void ReceiveDeactivate(const FGameplayTag& AbilityTag);

	/** 记录实际绑定的 ASC，EndAbility 时用它解除委托。 */
	TWeakObjectPtr<UAuraAbilitySystemComponent> BoundAuraASC;
};
