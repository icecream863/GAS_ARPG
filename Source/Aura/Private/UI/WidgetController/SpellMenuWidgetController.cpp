// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SpellMenuWidgetController.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Player/AuraPlayerState.h"

void USpellMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();
	
	SpellPointsChanged.Broadcast(GetAuraPS()->GetSpellPoints());
}

// 技能在学习的时候 会广播， 在 状态改变时 会广播
void USpellMenuWidgetController::BindCallbacksToDependencies()
{
	//~ 技能状态刷新
	GetAuraASC()->AbilityStatusChanged.AddLambda(
	[this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 AbilityLevel)
	{
		if (AbilityTag == SelectedAbility.Ability)
		{
			SelectedAbility.Status = StatusTag;// 更新技能状态
			
			bool bEnabledSpendPoints = false;
			bool bEquipEnabled = false;
			ShouldEnabledButtons(SelectedAbility.Status, CurrentSpellPoints, bEnabledSpendPoints, bEquipEnabled);
			FString  Description;
			FString NextLevelDescription;
			if (GetAuraASC())
			{
				GetAuraASC()->GetDescriptionByAbilityTag(AbilityTag, Description, NextLevelDescription);
			}
			SpellGlobeSelectedDelegate.Broadcast(bEnabledSpendPoints, bEquipEnabled, Description, NextLevelDescription);
		}
		
		if (AbilityInfo)
		{
			FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
			Info.StatusTag = StatusTag;
			AbilityInfoDelegate.Broadcast(Info);
		}
	});

	GetAuraASC()->AbilityEquipped.AddUObject(this, &USpellMenuWidgetController::OnAbilityEquipped);

	//~	SpellPoints刷新, 
	GetAuraPS()->OnSpellPointsChangedDelegate.AddLambda(
	[this](int32 SpellPoints)
	{
		SpellPointsChanged.Broadcast(SpellPoints);
		
		CurrentSpellPoints = SpellPoints;//更新技能点
		bool bEnabledSpendPoints = false;
		bool bEquipEnabled = false;
		ShouldEnabledButtons(SelectedAbility.Status, CurrentSpellPoints, bEnabledSpendPoints, bEquipEnabled);
		FString  Description;
		FString NextLevelDescription;
		if (GetAuraASC())
		{
			GetAuraASC()->GetDescriptionByAbilityTag(SelectedAbility.Ability, Description, NextLevelDescription);
		}
		
	SpellGlobeSelectedDelegate.Broadcast(bEnabledSpendPoints, bEquipEnabled, Description, NextLevelDescription);
	} );
	
}

void USpellMenuWidgetController::SpellGlobeSelected(const FGameplayTag& AbilityTag)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const int32 SpellPoints = GetAuraPS()->GetSpellPoints();
	FGameplayTag AbilityStatus;

	// 切换选择时取消当前装备等待，避免旧的槽位高亮继续播放。
	if (bWaitingForEquipSelection && AbilityInfo)
	{
		const FGameplayTag AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
		StopWaitingForEquipDelegate.Broadcast(AbilityType);
		bWaitingForEquipSelection = false;
	}
	
	const bool bTagValid = AbilityTag.IsValid();
	const bool bTagNone = AbilityTag == GameplayTags.Abilities_None;
	FGameplayAbilitySpec* AbilitySpec = GetAuraASC()->GetAbilitySpecFromTag(AbilityTag);
	const bool bAbilitySpecValid = AbilitySpec != nullptr;
	
	if (!bTagValid || bTagNone || !bAbilitySpecValid)
	{
		AbilityStatus = GameplayTags.Abilities_Status_Locked;
	}
	else
	{
		AbilityStatus = GetAuraASC()->GetStatusFromSpec(*AbilitySpec);
	}
	
	SelectedAbility.Ability = AbilityTag;
	SelectedAbility.Status = AbilityStatus;
	
	bool bEnabledSpendPoints = false;
	bool bEquipEnabled = false;
	ShouldEnabledButtons(AbilityStatus, SpellPoints, bEnabledSpendPoints, bEquipEnabled);
	
	FString  Description;
	FString NextLevelDescription;
	if (GetAuraASC())
	{
		GetAuraASC()->GetDescriptionByAbilityTag(AbilityTag, Description, NextLevelDescription);
	}
	SpellGlobeSelectedDelegate.Broadcast(bEnabledSpendPoints, bEquipEnabled, Description, NextLevelDescription);
}

void USpellMenuWidgetController::GlobeDeselect()
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (bWaitingForEquipSelection && AbilityInfo)
	{
		const FGameplayTag AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
		StopWaitingForEquipDelegate.Broadcast(AbilityType);
		bWaitingForEquipSelection = false;
	}

	SelectedAbility.Ability = GameplayTags.Abilities_None;
	SelectedAbility.Status = GameplayTags.Abilities_Status_Locked;
	SpellGlobeSelectedDelegate.Broadcast(false, false, FString(), FString());
}

void USpellMenuWidgetController::SpendPointButtonPressed()
{
	if (GetAuraASC())
	{
		GetAuraASC()->ServerSpendSpellPoint(SelectedAbility.Ability);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AuraAbilitySystemComponent is null in SpendPointButtonPressed"));
	}
}

void USpellMenuWidgetController::EquipButtonPressed()
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const bool bAbilityCanBeEquipped =
		SelectedAbility.Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked) ||
		SelectedAbility.Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped);

	if (!bAbilityCanBeEquipped || !AbilityInfo)
	{
		return;
	}

	// 已装备技能重新分配槽位时，需要记住它原来的输入槽。
	const FGameplayTag SelectedStatus = GetAuraASC()->GetStatusFromAbilityTag(SelectedAbility.Ability);
	if (SelectedStatus.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
	{
		SelectedSlot = GetAuraASC()->GetSlotFromAbilityTag(SelectedAbility.Ability);
	}

	FGameplayTag AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
	if (!AbilityType.IsValid())
	{
		AbilityType = GameplayTags.Abilities_Type_None;
	}

	// The equipped row uses this type to highlight offensive or passive slots.
	bWaitingForEquipSelection = true;
	WaitForEquipDelegate.Broadcast(AbilityType);
}

void USpellMenuWidgetController::SpellRowGlobePressed(const FGameplayTag& SlotTag, const FGameplayTag& AbilityType)
{
	if (!bWaitingForEquipSelection || !AbilityInfo)
	{
		return;
	}

	// Prevent assigning offensive abilities to passive slots, and vice versa.
	const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
	if (!SelectedAbilityType.MatchesTagExact(AbilityType))
	{
		return;
	}

	// 槽位分配会修改 AbilitySpec，必须交给服务端执行。
	GetAuraASC()->ServerEquipAbility(SelectedAbility.Ability, SlotTag);
}

void USpellMenuWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status,
	const FGameplayTag& Slot, const FGameplayTag& PreviousSlot)
{
	if (!AbilityInfo)
	{
		return;
	}

	bWaitingForEquipSelection = false;

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (PreviousSlot.IsValid())
	{
		// 旧槽位广播空信息，让对应的装备球清空显示。
		FAuraAbilityInfo LastSlotInfo;
		LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;
		LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
		LastSlotInfo.InputTag = PreviousSlot;
		LastSlotInfo.CooldownTag = FGameplayTag();
		AbilityInfoDelegate.Broadcast(LastSlotInfo);
	}

	FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
	Info.StatusTag = Status;
	Info.InputTag = Slot;
	AbilityInfoDelegate.Broadcast(Info);
	// 装备完成后停止“等待选择槽位”的高亮动画。
	StopWaitingForEquipDelegate.Broadcast(Info.AbilityType);
	SpellGlobeReassignedDelegate.Broadcast(AbilityTag);
	GlobeDeselect();
}

void USpellMenuWidgetController::ShouldEnabledButtons(const FGameplayTag& AbilityStatus,int32 SpellPoints, bool& bSpellPointsButtonEnabled,
                                                      bool& bEquipButtonEnabled)
{
	// 默认都禁用 ，什么时候能 给技能加点和装备
	bSpellPointsButtonEnabled = false;
	bEquipButtonEnabled = false;
	
	if (AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
	{
		bEquipButtonEnabled = true;
		if (SpellPoints > 0)
		{
			bSpellPointsButtonEnabled = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Eligible))
	{
		if (SpellPoints > 0)
		{
			bSpellPointsButtonEnabled = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Unlocked))
	{
		bEquipButtonEnabled = true;
		if (SpellPoints > 0)
		{
			bSpellPointsButtonEnabled = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Locked))
	{
		
	}
}
