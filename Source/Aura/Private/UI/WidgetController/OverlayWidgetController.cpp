// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AuraGameplayTags.h"
#include "Player/AuraPlayerState.h"


void UOverlayWidgetController::BroadcastInitialValues()
{
	// 不需要 继承父类函数


	OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());

	OnManaChanged.Broadcast(GetAuraAS()->GetMana());
	OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());
}

void UOverlayWidgetController::BindCallbacksToDependencies()//依赖是 AuraAttribute底层
{
	// Super::BindCallbacksToDependencies(); 不需要继承父类函数，只需重载
	//绑定 经验条

	GetAuraASC()->AbilityEquipped.AddUObject(this, &UOverlayWidgetController::OnAbilityEquipped);
	GetAuraPS()->OnXPChangedDelegate.AddUObject(this, &UOverlayWidgetController::OnXPChanged);
	GetAuraPS()->OnLevelChangedDelegate.AddLambda(
	[this](int32 NewLevel, bool bLevelUp)
	{
		OnPlayerLevelChangedDelegate.Broadcast(NewLevel, bLevelUp);
	} );




	//监测 AttributeValue 变化， 触发回调
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetHealthAttribute()).AddLambda(
	[this](const FOnAttributeChangeData& Data)
		{
			//~ 监听到属性变化， 就广播给界面
			OnHealthChanged.Broadcast(Data.NewValue);
		}
		);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		}
		);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnManaChanged.Broadcast(Data.NewValue);
		}
		);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(GetAuraAS()->GetMaxManaAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxManaChanged.Broadcast(Data.NewValue);
		}
		);

// ~ Tags 初始化技能图标

		/*
		 * 1.如果广播先触发，后绑定，那么在绑定之前发生的事件将不会被监听到，因为当时没有任何函数绑定到委托上来接收这些事件。
		 * 2.如果绑定先触发，后广播，那么在广播事件时，已经有函数绑定到委托上了，所以这些函数能够正确地接收和处理广播的事件。
		 *  时机触发不确定，要做 保险操作
		 */
		if (GetAuraASC()->bStartupAbilitiesGiven)
		{
			BroadcastAbilityInfo();
			//~ 如果广播已触发，直接回调
		}
		else
		{
			GetAuraASC()->AbilityGivenDelegate.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
			//~ 如果广播未触发，绑定回调，等广播触发时回调被调用
		}


//~  当有 pick up Message 标签的 GameplayEffect 应用时，广播对应 UI 数据给界面
		GetAuraASC()->EffectAssetTagsDelegate.AddLambda(
		[this](FGameplayTagContainer& TagContainer)
		{
			//~ 监听到标签变化， 就去datatable里找对应标签的UI数据，广播给界面
			//~ 筛选标签，找出符合条件的标签（比如说包含 "Message" 标签的标签），然后根据这些标签去 DataTable 里找对应的 UI 数据，最后把这些数据广播给界面。
			const FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));

			for (const FGameplayTag& Tag : TagContainer)
			{
				if (Tag.MatchesTag(MessageTag))//"A.1".MatchesTag("A") will return True, "A".MatchesTag("A.1") will return False
				{
					FUIWidgetRow* Row =  GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);

					MessageWidgetRowDelegate.Broadcast(*Row);
				}
			}
		});
	}







void UOverlayWidgetController::OnXPChanged(int32 NewXP)
{

	const ULevelUpInfo* LevelUpInfo = GetAuraPS()->LevelUpInfo;

	checkf(LevelUpInfo, TEXT("LevelUpInfo is null! Please assign a LevelUpInfo Data Asset to the PlayerState."));

	const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
	const int32 MaxLevel = LevelUpInfo->LevelUpInformation.Num() - 1; // 因为下标0是占位，所以最大等级是数组长度减1

	if (Level <= MaxLevel && Level > 0)
	{
		const int32 XPForCurrentLevel = LevelUpInfo->LevelUpInformation[Level].LevelUpRequirement;
		const int32 XPForPreviousLevel = LevelUpInfo->LevelUpInformation[Level - 1].LevelUpRequirement;

		const float XPPercent = (XPForCurrentLevel - XPForPreviousLevel) > 0 ?
			static_cast<float>(NewXP - XPForPreviousLevel) / (XPForCurrentLevel - XPForPreviousLevel) : 0.0f;

		OnXPPercentChangedDelegate.Broadcast(XPPercent);
	}
	else
	{
		OnXPPercentChangedDelegate.Broadcast(0.0f); // 超出范围，显示满格或空格
	}

}

void UOverlayWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status,
	const FGameplayTag& Slot, const FGameplayTag& PreviousSlot) const
{
	if (!AbilityInfo)
	{
		return;
	}

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (PreviousSlot.IsValid())
	{
		// 旧槽位发空 AbilityInfo，让 HUD 上原来的技能球清空。
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
}

