#include "UI/ViewModel/MVVMLoadScreen.h"

#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ViewModel/MVVMLoadSlot.h"

void UMVVMLoadScreen::InitializeLoadSlots()
{
	checkf(LoadSlotViewModelClass, TEXT("BP_LoadScreenViewModel 未设置 LoadSlotViewModelClass"));

	LoadSlots.Empty(NumLoadSlots);
	for (int32 Index = 0; Index < NumLoadSlots; ++Index)
	{
		UMVVMLoadSlot* LoadSlot = NewObject<UMVVMLoadSlot>(this, LoadSlotViewModelClass);
		LoadSlot->SetLoadSlotName(FString::Printf(TEXT("LoadSlot_%d"), Index));
		LoadSlot->SetSlotIndex(Index);
		LoadSlots.Add(Index, LoadSlot);
	}
}

void UMVVMLoadScreen::LoadData()
{
	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!AuraGameMode)
	{
		return;
	}

	for (const TTuple<int32, TObjectPtr<UMVVMLoadSlot>>& LoadSlotPair : LoadSlots)
	{
		UMVVMLoadSlot* LoadSlot = LoadSlotPair.Value;
		if (!LoadSlot)
		{
			continue;
		}

		ULoadScreenSaveGame* SaveObject = AuraGameMode->GetSaveSlotData(LoadSlot->GetLoadSlotName(), LoadSlot->GetSlotIndex());
		LoadSlot->SetPlayerName(SaveObject->PlayerName);
		FString MapName = SaveObject->MapName;
		if (MapName.IsEmpty() && !SaveObject->MapAssetName.IsEmpty())
		{
			// 兼容旧存档/异常存档：显示名被写空时，按资产名反查回显示名。
			MapName = AuraGameMode->GetMapNameFromMapAssetName(SaveObject->MapAssetName);
		}
		LoadSlot->SetMapName(MapName);
		LoadSlot->SetPlayerLevel(SaveObject->PlayerLevel);
		LoadSlot->SetPlayerStartTag(SaveObject->PlayerStartTag);
		LoadSlot->SetSlotStatus(SaveObject->SaveSlotStatus);
		LoadSlot->InitializeSlot();
	}
}

UMVVMLoadSlot* UMVVMLoadScreen::GetLoadSlotViewModelByIndex(const int32 Index) const
{
	const TObjectPtr<UMVVMLoadSlot>* FoundSlot = LoadSlots.Find(Index);
	ensureMsgf(FoundSlot, TEXT("请求了不存在的存档槽索引：%d"), Index);
	return FoundSlot ? FoundSlot->Get() : nullptr;
}

void UMVVMLoadScreen::SelectSlotButtonPressed(const int32 Slot)
{
	SelectedSlot = GetLoadSlotViewModelByIndex(Slot);
	if (!SelectedSlot)
	{
		return;
	}

	// 只要能点击 Taken 槽位的 Select，就说明已经有有效槽位被选中。
	SlotSelected.Broadcast();

	for (const TTuple<int32, TObjectPtr<UMVVMLoadSlot>>& LoadSlotPair : LoadSlots)
	{
		if (!LoadSlotPair.Value)
		{
			continue;
		}

		// 被选中的槽位禁用按钮，其它 Taken 槽位重新启用，形成清晰的选中态。
		const bool bEnableButton = LoadSlotPair.Key != Slot;
		LoadSlotPair.Value->EnableSelectSlotButton.Broadcast(bEnableButton);
	}
}

void UMVVMLoadScreen::NewGameButtonPressed(const int32 Slot)
{
	// 【优化】复用带边界检查的查询函数，避免课程中 Map 下标访问在索引错误时插入空元素。
	if (UMVVMLoadSlot* LoadSlot = GetLoadSlotViewModelByIndex(Slot))
	{
		LoadSlot->SetSlotStatus(EnterName);
		LoadSlot->InitializeSlot();
	}
}

void UMVVMLoadScreen::NewSlotButtonPressed(const int32 Slot, const FString& EnteredName)
{
	// 【课程】客户端（Listen Server 的非主机）拿不到有效 GameMode，禁止创建新槽，
	// 并提示用户切回单人模式，避免单人存档在多人环境下被误操作。
	if (!IsValid(Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this))))
	{
		GEngine->AddOnScreenDebugMessage(1, 15.f, FColor::Magenta, TEXT("Please switch to single player"));
		return;
	}

	UMVVMLoadSlot* LoadSlot = GetLoadSlotViewModelByIndex(Slot);
	if (!LoadSlot)
	{
		return;
	}

	LoadSlot->SetPlayerName(EnteredName);
	// 新建槽固定从 1 级开始，和 SaveGame 的默认值保持一致。
	LoadSlot->SetPlayerLevel(1);
	LoadSlot->SetSlotStatus(Taken);

	if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		LoadSlot->SetMapName(AuraGameMode->GetDefaultMapName());
		// 新建槽必须同时写入地图资产名，死亡重生才能回到起始地图。
		LoadSlot->SetMapAssetName(AuraGameMode->GetDefaultMapAssetName());
		LoadSlot->SetPlayerStartTag(AuraGameMode->DefaultPlayerStartTag);
		AuraGameMode->SaveSlotData(LoadSlot);

		// 新建槽即写入跨关卡持久数据：读档用的 Slot 名/索引，以及默认出生点标签。
		if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			AuraGameInstance->LoadSlotName = LoadSlot->GetLoadSlotName();
			AuraGameInstance->LoadSlotIndex = LoadSlot->GetSlotIndex();
			AuraGameInstance->PlayerStartTag = AuraGameMode->DefaultPlayerStartTag;
		}
	}

	LoadSlot->InitializeSlot();
}

void UMVVMLoadScreen::PlayButtonPressed()
{
	if (!SelectedSlot)
	{
		return;
	}

	if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		// 旅行前把选中槽的出生点标签写进 GameInstance，无论新建槽还是已存旧槽 Play 都能拿到正确标签。
		if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			AuraGameInstance->LoadSlotName = SelectedSlot->GetLoadSlotName();
			AuraGameInstance->LoadSlotIndex = SelectedSlot->GetSlotIndex();
			AuraGameInstance->PlayerStartTag = SelectedSlot->GetPlayerStartTag();
		}
		AuraGameMode->TravelToMap(SelectedSlot);
	}
}

void UMVVMLoadScreen::DeleteButtonPressed()
{
	if (!SelectedSlot)
	{
		return;
	}

	AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (!AuraGameMode)
	{
		return;
	}

	AuraGameMode->DeleteSlot(SelectedSlot->GetLoadSlotName(), SelectedSlot->GetSlotIndex());

	// 删除后把当前槽恢复为 Vacant，下一次创建新槽时再按正常流程变成 Taken。
	SelectedSlot->SetPlayerName(TEXT("Default Name"));
	SelectedSlot->SetMapName(FString());
	SelectedSlot->SetSlotStatus(Vacant);
	SelectedSlot->InitializeSlot();

	// 【优化】提前把该槽的 Select 按钮恢复为可点击，避免再次创建 Taken 后沿用旧的禁用状态。
	SelectedSlot->EnableSelectSlotButton.Broadcast(true);
	SelectedSlot = nullptr;
}

void UMVVMLoadScreen::SetNumLoadSlots(const int32 InNumLoadSlots)
{
	UE_MVVM_SET_PROPERTY_VALUE(NumLoadSlots, FMath::Max(0, InNumLoadSlots));
}
