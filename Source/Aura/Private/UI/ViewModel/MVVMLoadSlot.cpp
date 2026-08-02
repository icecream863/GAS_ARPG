#include "UI/ViewModel/MVVMLoadSlot.h"

void UMVVMLoadSlot::InitializeSlot()
{
	// SlotStatus 的枚举顺序必须和 WidgetSwitcher 页签顺序保持一致。
	const int32 WidgetSwitcherIndex = SlotStatus.GetValue();
	SetWidgetSwitcherIndex.Broadcast(WidgetSwitcherIndex);
}

void UMVVMLoadSlot::SetLoadSlotName(const FString& InLoadSlotName)
{
	UE_MVVM_SET_PROPERTY_VALUE(LoadSlotName, InLoadSlotName);
}

void UMVVMLoadSlot::SetSlotIndex(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

void UMVVMLoadSlot::SetPlayerName(const FString& InPlayerName)
{
	UE_MVVM_SET_PROPERTY_VALUE(PlayerName, InPlayerName);
}

void UMVVMLoadSlot::SetMapName(const FString& InMapName)
{
	UE_MVVM_SET_PROPERTY_VALUE(MapName, InMapName);
}

void UMVVMLoadSlot::SetPlayerLevel(const int32 InPlayerLevel)
{
	UE_MVVM_SET_PROPERTY_VALUE(PlayerLevel, InPlayerLevel);
}

void UMVVMLoadSlot::SetSlotStatus(const ESaveSlotStatus InSlotStatus)
{
	SlotStatus = InSlotStatus;
}

void UMVVMLoadSlot::SetPlayerStartTag(const FName& InPlayerStartTag)
{
	PlayerStartTag = InPlayerStartTag;
}
