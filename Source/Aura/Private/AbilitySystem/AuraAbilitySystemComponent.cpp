// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AbilitySystem/Abilities/AuraPassiveAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Aura/AuraLogChannels.h"
#include "Interaction/PlayerInterface.h"

namespace
{
	/**
	 * 获取当前正在运行的 Ability 实例所使用的 PredictionKey。
	 * AbilityTask 会用 (SpecHandle, PredictionKey) 作为复制事件的索引；两者只要有一个不一致，
	 * WaitInputPress/Release 即使收到了输入，也找不到自己注册的 Delegate。
	 */
	FPredictionKey GetActiveAbilityPredictionKey(const FGameplayAbilitySpec& AbilitySpec)
	{
		const TArray<UGameplayAbility*> AbilityInstances = AbilitySpec.GetAbilityInstances();
		if (!AbilityInstances.IsEmpty())
		{
			// InstancedPerActor 通常只有一个实例；InstancedPerExecution 可能有多个，
			// 输入事件应发送给最后创建、也就是当前这一次激活的实例。
			if (const UGameplayAbility* ActiveInstance = AbilityInstances.Last())
			{
				return ActiveInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
			}
		}

		// 仅为旧的 NonInstanced Ability 保留回退。UE 5.8 已弃用 Spec 上的 ActivationInfo，
		// 新的实例化 Ability 必须走上面的实例级 ActivationInfo。
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return AbilitySpec.ActivationInfo.GetActivationPredictionKey();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}


// ~-在AbilityActorInfoSet函数中，我们将EffectApplied函数绑定到OnGameplayEffectAppliedDelegateToSelf委托上，这样每当一个GameplayEffect被应用到这个AbilitySystemComponent时，EffectApplied函数就会被调用。
// ~-在EffectApplied函数中，我们从EffectSpec中获取所有的AssetTags，并通过EffectAssetTags委托广播这些标签，以便其他系统可以响应这些标签的变化。
void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent* AbilitySystemComponent,
                                                const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	EffectAssetTagsDelegate.Broadcast(TagContainer);
}

void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
	/* 当可激活能力列表（Activatable/Replicated ActivatableAbilities）从服务器复制到客户端时会被调用。重写它可以在客户端处理“被授予能力”或能力列表变化的初始化工作。*/
	Super::OnRep_ActivateAbilities();

	if (!bStartupAbilitiesGiven)
	{
		bStartupAbilitiesGiven = true;
		AbilityGivenDelegate.Broadcast();
	}//只需要在第一次 添加技能时广播， 因为广播的是 ASC，
}


void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			// 通知 Spec 键位被按下
			AbilitySpecInputPressed(AbilitySpec);
			// 如果技能还没激活，尝试激活它
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);

			// WaitInputPress/Release 监听的是 Ability 的 replicated event，不是普通输入回调。
			if (AbilitySpec.IsActive())
			{
				InvokeReplicatedEvent(
					EAbilityGenericReplicatedEvent::InputPressed,
					AbilitySpec.Handle,
					GetActiveAbilityPredictionKey(AbilitySpec));
			}
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{

	if (!InputTag.IsValid()) return;
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
				AbilitySpecInputReleased(AbilitySpec);
				if (AbilitySpec.IsActive())
				{
					InvokeReplicatedEvent(
						EAbilityGenericReplicatedEvent::InputReleased,
						AbilitySpec.Handle,
						GetActiveAbilityPredictionKey(AbilitySpec));
				}
		}
	}
}

void UAuraAbilitySystemComponent::AddCharacterAbility(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{

	for (const TSubclassOf<UGameplayAbility> StartupAbility : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(StartupAbility, 1);

		if (const UAuraGameplayAbility* AuraGameplayAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraGameplayAbility->StartupInputTag);
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);// 学习 就是 装备了
			GiveAbility(AbilitySpec);
		}
	}

	bStartupAbilitiesGiven = true;
	AbilityGivenDelegate.Broadcast();//只发生在服务端
}

void UAuraAbilitySystemComponent::AddPassiveCharacterAbility(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{

	for (const TSubclassOf<UGameplayAbility> StartupAbility : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(StartupAbility, 1);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}

}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
		else
		{
			UE_LOG(LogAura, Warning, TEXT("UpgradeAttribute: Not enough attribute points to upgrade %s"), *AttributeTag.ToString());
		}
	}
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventTag = AttributeTag;
	Payload.EventMagnitude = 1.0f;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
	}

}


void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
	/*
	*  作用域锁：遍历可激活技能列表期间，禁止 GAS 在回调/重入中修改 AbilitySpec 列表（增删/刷新）
	*  避免遍历过程中容器变化导致的崩溃或不一致；离开作用域自动解锁（RAII）
	*/
	FScopedAbilityListLock ActiveScopeLock(*this);

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!Delegate.ExecuteIfBound(AbilitySpec)) //这里在执行 委托
		{
			UE_LOG(LogAura, Error, TEXT("ForEachAbility: Delegate execution failed for ability %s"), *AbilitySpec.Ability->GetName());
		}
	}
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		for (const FGameplayTag& Tag : AbilitySpec.Ability.Get()->GetAssetTags())
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
			{
				return Tag;
			}
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (const FGameplayTag& Tag : AbilitySpec.GetDynamicSpecSourceTags())//AddCharacterAbility里添加的标签
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (const FGameplayTag& Tag : AbilitySpec.GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetAbilitySpecFromTag(const FGameplayTag& Tag)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTagExact(Tag))
		{
			return &AbilitySpec;
		}
	}
	return nullptr;
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if (const FGameplayAbilitySpec* AbilitySpec = GetAbilitySpecFromTag(AbilityTag))
	{
		return GetStatusFromSpec(*AbilitySpec);
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetSlotFromAbilityTag(const FGameplayTag& AbilityTag)
{
	if (const FGameplayAbilitySpec* AbilitySpec = GetAbilitySpecFromTag(AbilityTag))
	{
		return GetInputTagFromSpec(*AbilitySpec);
	}
	return FGameplayTag();
}

bool UAuraAbilitySystemComponent::GetDescriptionByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription,
	FString& OutNextLevelDescription)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (!AbilityTag.IsValid() || AbilityTag.MatchesTagExact(GameplayTags.Abilities_None))
	{
		OutDescription = FString();
		OutNextLevelDescription = FString();
		return false;
	}

	if (FGameplayAbilitySpec* AbilitySpec = GetAbilitySpecFromTag(AbilityTag))
	{
		if (UAuraGameplayAbility* AuraGameplayAbility = Cast<UAuraGameplayAbility>(AbilitySpec->Ability))
		{
			OutDescription = AuraGameplayAbility->GetDescription(AbilitySpec->Level);
			OutNextLevelDescription = AuraGameplayAbility->GetNextLevelDescription(AbilitySpec->Level + 1);
			return true;
		}


	}

	// 如果技能未解锁，返回锁定描述，locked状态返回 locked 描述
	UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	if (!AbilityInfo)
	{
		OutDescription = FString();
		OutNextLevelDescription = FString();
		return false;
	}
	OutDescription = UAuraGameplayAbility::GetLockedDescription(AbilityInfo->FindAbilityInfoForTag(AbilityTag).LevelRequirement);
	OutNextLevelDescription = FString();
	return false;

}


// 在角色升级时调用
void UAuraAbilitySystemComponent::UpdateAbilityStatuses(int32 Level)
{
	UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());

	for (const FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
	{
		if (Level < Info.LevelRequirement )	 continue;
		if (!Info.AbilityTag.IsValid()) continue;
		if (GetAbilitySpecFromTag(Info.AbilityTag) == nullptr)
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
			GiveAbility(AbilitySpec);
			ClientUpdateAbilityStatus(Info.AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);//填 1是因为才解锁一个技能
			// 服务端将数据 广播给客户端
			MarkAbilitySpecDirty(AbilitySpec);// 立即刷新状态，避免客户端延迟显示
		}

	}


}

// 消耗 技能点
void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag& AbilityTag)
{
	if (FGameplayAbilitySpec* AbilitySpec = GetAbilitySpecFromTag(AbilityTag))
	{
		if (GetAvatarActor()->Implements<UPlayerInterface>() && IPlayerInterface::Execute_GetSpellPoints(GetAvatarActor()) > 0)
		{
			IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
			// 这里可以添加逻辑来处理技能升级或其他相关操作
		}

		const FAuraGameplayTags & GameplayTags = FAuraGameplayTags::Get();
		FGameplayTag CurrentStatus = GetStatusFromSpec(*AbilitySpec);
		if (CurrentStatus == GameplayTags.Abilities_Status_Eligible)
		{
			CurrentStatus = GameplayTags.Abilities_Status_Unlocked;
			AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Eligible);
			AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Unlocked);
		}
		else if (CurrentStatus == GameplayTags.Abilities_Status_Equipped || CurrentStatus == GameplayTags.Abilities_Status_Unlocked)
		{
			AbilitySpec->Level += 1;
		}

		ClientUpdateAbilityStatus(AbilityTag, CurrentStatus, AbilitySpec->Level);// 更新技能状态
		MarkAbilitySpecDirty(*AbilitySpec); // 立即刷新状态，避免客户端延迟显示
	}

}
void UAuraAbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag,
                                                                           const FGameplayTag& StatusTag, int32 AbilityLevel)
{
	AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, AbilityLevel);
}

void UAuraAbilitySystemComponent::ServerEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& Slot)
{
	if (FGameplayAbilitySpec* AbilitySpec = GetAbilitySpecFromTag(AbilityTag))
	{
		const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
		const FGameplayTag PreviousSlot = GetInputTagFromSpec(*AbilitySpec);
		FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);
		const bool bStatusValid =
			Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped) ||
			Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked);

		if (bStatusValid)
		{
			if (!SlotIsEmpty(Slot))
			{
				// 目标槽已有技能：先找到被挤出的 Spec，才能在清槽前判断是否需要结束被动技能。
				if (FGameplayAbilitySpec* SpecWithSlot = GetSpecWithSlot(Slot))
				{
					const FGameplayTag AbilityTagInSlot = GetAbilityTagFromSpec(*SpecWithSlot);
					if (AbilityTag.MatchesTagExact(AbilityTagInSlot))
					{
						// 同一技能放回原槽时无需结束或重新激活，但仍通知客户端结束装备选择状态。
						ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PreviousSlot);
						return;
					}

					if (IsPassiveAbility(*SpecWithSlot))
					{
						// 被动 Ability 自己监听该委托，并在标签匹配时调用 EndAbility。
						DeactivatePassiveAbility.Broadcast(AbilityTagInSlot);
						MulticastActivatePassiveEffect(AbilityTagInSlot, false);
					}

					ClearSlot(*SpecWithSlot);
					MarkAbilitySpecDirty(*SpecWithSlot);
				}
			}

			if (!AbilityHasAnySlot(*AbilitySpec) && IsPassiveAbility(*AbilitySpec))
			{
				// 被动技能第一次进入任意槽位时由服务器激活；换槽时保持原实例持续运行。
				if (TryActivateAbility(AbilitySpec->Handle))
				{
					MulticastActivatePassiveEffect(AbilityTag, true);
				}
			}

			AssignSlotToAbility(*AbilitySpec, Slot);

			if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
			{
				// 第一次装备时，把技能从 Unlocked 推进到 Equipped 状态。
				AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Unlocked);
				AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Equipped);
				Status = GameplayTags.Abilities_Status_Equipped;
			}

			MarkAbilitySpecDirty(*AbilitySpec);
			ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PreviousSlot);
		}
	}
}

void UAuraAbilitySystemComponent::ClientEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& Status,
	const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
{
	AbilityEquipped.Broadcast(AbilityTag, Status, Slot, PreviousSlot);
}

void UAuraAbilitySystemComponent::MulticastActivatePassiveEffect_Implementation(
	const FGameplayTag& AbilityTag, bool bActivate)
{
	ActivatePassiveEffect.Broadcast(AbilityTag, bActivate);
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(const FGameplayAbilitySpec& Spec, const FGameplayTag& Slot)
{
	return Spec.GetDynamicSpecSourceTags().HasTagExact(Slot);
}

bool UAuraAbilitySystemComponent::AbilityHasAnySlot(const FGameplayAbilitySpec& Spec)
{
	const FGameplayTag InputTagRoot = FGameplayTag::RequestGameplayTag(FName("InputTag"));
	return Spec.GetDynamicSpecSourceTags().HasTag(InputTagRoot);
}

bool UAuraAbilitySystemComponent::SlotIsEmpty(const FGameplayTag& Slot)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (AbilityHasSlot(Spec, Slot))
		{
			return false;
		}
	}
	return true;
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecWithSlot(const FGameplayTag& Slot)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (AbilityHasSlot(Spec, Slot))
		{
			return &Spec;
		}
	}
	return nullptr;
}

bool UAuraAbilitySystemComponent::IsPassiveAbility(const FGameplayAbilitySpec& Spec)
{
	// 【优化】课程查询 DA_AbilityInfo.AbilityType；这里直接依据 C++ 基类，避免数据资产误配破坏运行时逻辑。
	return Spec.Ability && Spec.Ability->IsA<UAuraPassiveAbility>();
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec& Spec)
{
	const FGameplayTag Slot = GetInputTagFromSpec(Spec);
	if (Slot.IsValid())
	{
		Spec.GetDynamicSpecSourceTags().RemoveTag(Slot);
	}
}

void UAuraAbilitySystemComponent::AssignSlotToAbility(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot)
{
	// 一个 Spec 最多保留一个 InputTag：先移除旧槽，再加入新槽。
	ClearSlot(Spec);
	Spec.GetDynamicSpecSourceTags().AddTag(Slot);
}
