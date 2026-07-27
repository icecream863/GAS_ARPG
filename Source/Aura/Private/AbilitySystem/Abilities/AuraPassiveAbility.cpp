// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/AuraPassiveAbility.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"

UAuraPassiveAbility::UAuraPassiveAbility()
{
	// 被动技能需要保存每个角色各自的 ASC 绑定，不能让所有角色共用 NonInstanced 的 CDO。
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 【优化】课程在三个 GA 中重复配置；放到被动 C++ 基类可统一保证 ServerInitiated。
	// 装备判定发生在服务器；服务器激活后 GAS 会把该持续 Ability 同步到所属客户端。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UAuraPassiveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(
		GetAbilitySystemComponentFromActorInfo());
	if (!IsValid(AuraASC))
	{
		// 没有项目 ASC 就无法接收停用广播，继续保持激活会形成无法正常卸载的被动技能。
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 防御性去重：即使外部错误地重复激活，也只保留一份回调。
	AuraASC->DeactivatePassiveAbility.RemoveAll(this);
	AuraASC->DeactivatePassiveAbility.AddUObject(this, &UAuraPassiveAbility::ReceiveDeactivate);
	BoundAuraASC = AuraASC;
}

void UAuraPassiveAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Ability 实例可能继续存在并被再次装备，结束时必须移除本轮激活留下的绑定。
	if (BoundAuraASC.IsValid())
	{
		BoundAuraASC->DeactivatePassiveAbility.RemoveAll(this);
		BoundAuraASC.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraPassiveAbility::ReceiveDeactivate(const FGameplayTag& AbilityTag)
{
	if (!AbilityTag.IsValid() || !GetAssetTags().HasTagExact(AbilityTag))
	{
		return;
	}

	// 停用由 ASC 发起；复制结束可让拥有该 Ability 实例的网络端同步结束状态。
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
