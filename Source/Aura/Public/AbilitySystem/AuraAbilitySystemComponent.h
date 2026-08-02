// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilitySystemComponent.generated.h"

class UAuraAbilitySystemComponent;
class ULoadScreenSaveGame;
DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, FGameplayTagContainer& /*AssetTagContainer*/);
DECLARE_MULTICAST_DELEGATE(FAbilityGiven);
DECLARE_DELEGATE_OneParam(FForEachAbility, FGameplayAbilitySpec&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FAbilityStatusChanged,const FGameplayTag&/* Ability Tag */,const FGameplayTag& /* Status Tag */, int32 /* Level */);
DECLARE_MULTICAST_DELEGATE_FourParams(FAbilityEquipped, const FGameplayTag& /* Ability Tag */, const FGameplayTag& /* Status Tag */, const FGameplayTag& /* Slot */, const FGameplayTag& /* Previous Slot */);
DECLARE_MULTICAST_DELEGATE_OneParam(FDeactivatePassiveAbility, const FGameplayTag& /* Ability Tag */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FActivatePassiveEffect, const FGameplayTag& /* Ability Tag */, bool /* bActivate */);

/**
 *
 */


UCLASS()
class AURA_API UAuraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

protected:

	virtual void OnRep_ActivateAbilities() override;
	//当 ActivatableAbilities （可激活能力列表）发生 网络复制 时自动调用

public:
	/** 初始化 ASC 的 ActorInfo，并注册 GameplayEffect 应用回调（用于后续在客户端广播效果标签等）。 */
	void AbilityActorInfoSet();

	/**
	 * 当有 GameplayEffect 应用到自身时，把该效果的 Asset Tags 通过此委托广播出去。
	 * 常用于 UI提示音效等监听“发生了带哪些标签的效果”。
	 */
	FEffectAssetTags EffectAssetTagsDelegate;
	FAbilityGiven AbilityGivenDelegate; // ~-当角色被授予能力时广播，常用于 UI 监听角色能力变化。
	FAbilityStatusChanged AbilityStatusChanged;
	FAbilityEquipped AbilityEquipped;
	/** 广播需要停用的被动技能标签；对应的被动 Ability 自己判断并结束。 */
	FDeactivatePassiveAbility DeactivatePassiveAbility;
	/** 每个网络端本地广播，被动 Niagara 组件按 AbilityTag 选择性响应。 */
	FActivatePassiveEffect ActivatePassiveEffect;

	bool bStartupAbilitiesGiven = false; // 避免先广播再绑定。

	/**
	 * 输入标签被按住时触发：遍历可激活能力，找到匹配该输入标签的能力并通知 GAS 输入按下，
	 * 必要时尝试激活该能力
	 */
	void AbilityInputTagHeld(const FGameplayTag& InputTag);

	/**
	 * 输入标签首次按下时触发：只负责把一次性的 Pressed 事件转发给正在运行的 AbilityTask。
	 */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/**
	 * 输入标签被松开时触发：遍历可激活能力，找到匹配该输入标签的能力并通知 GAS 输入松开。
	 */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/**
	 * 授予角色起始能力：把 StartupAbilities 里的能力创建为 Spec 并 GiveAbility，
	 * 同时把能力的输入标签写入 DynamicAbilityTags，便于通过输入标签驱动释放。
	 */
	void AddCharacterAbility(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);

	void AddPassiveCharacterAbility(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities);

	/**
	 * 读档时按存档数据恢复能力：遍历 SavedAbilities，为每个技能重建 AbilitySpec
	 * （还原等级、状态、槽位），被动技能只有存档状态为 Equipped 时才激活。
	 */
	void AddCharacterAbilitiesFromSaveData(ULoadScreenSaveGame* SaveData);

	void UpgradeAttribute(const FGameplayTag& AttributeTag);

	UFUNCTION(Server, Reliable)
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	/** 只在 Held 逻辑里调用 TryActivateAbility 确实是核心逻辑所在
	* 1. 为什么要放在 Held 而不是 Pressed？
	在 GAS 处理输入时，将激活逻辑放在 Held 主要是为了支持自动施法/连发或者蓄力技能。

	按下即触发： 当你按下按键的第一帧，Held 事件就会触发。由于此时技能尚未激活，!AbilitySpec.IsActive() 为真，技能会立即启动。

	按住连续触发： 对于那种“按住就一直放”的技能（比如法师的平 A 或持续施法），
	如果技能结束了，但你还没松手，Held 会在下一帧检测到 !IsActive()，从而再次自动调用 TryActivateAbility。
	*/


	void ForEachAbility(const FForEachAbility& Delegate);

	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	FGameplayAbilitySpec* GetAbilitySpecFromTag(const FGameplayTag& Tag);
	FGameplayTag GetStatusFromAbilityTag(const FGameplayTag& AbilityTag);
	FGameplayTag GetSlotFromAbilityTag(const FGameplayTag& AbilityTag);

	bool GetDescriptionByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription,  FString& OutNextLevelDescription);

	void UpdateAbilityStatuses(int32 Level);



	/* Client RPC 客户端主动找服务端要数据 */
	/* Server RPC 服务端主动发消息给客户端 */
	UFUNCTION(Client, Reliable)
	void ClientEffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle) const;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAbilityStatus(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 AbilityLevel);

	UFUNCTION(Server, Reliable)
	void ServerSpendSpellPoint(const FGameplayTag& AbilityTag);

	UFUNCTION(Server, Reliable)
	void ServerEquipAbility(const FGameplayTag& AbilityTag, const FGameplayTag& Slot);

	UFUNCTION(Client, Reliable)
	void ClientEquipAbility(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PreviousSlot);

	// 【优化】课程使用 Unreliable；持续特效一旦丢失关闭消息就会永久残留，装备操作频率低，因此改用 Reliable。
	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivatePassiveEffect(const FGameplayTag& AbilityTag, bool bActivate);

private:
	static bool AbilityHasSlot(const FGameplayAbilitySpec& Spec, const FGameplayTag& Slot);
	static bool AbilityHasAnySlot(const FGameplayAbilitySpec& Spec);
	bool SlotIsEmpty(const FGameplayTag& Slot);
	FGameplayAbilitySpec* GetSpecWithSlot(const FGameplayTag& Slot);
	static bool IsPassiveAbility(const FGameplayAbilitySpec& Spec);
	static void ClearSlot(FGameplayAbilitySpec& Spec);
	static void AssignSlotToAbility(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot);

};
