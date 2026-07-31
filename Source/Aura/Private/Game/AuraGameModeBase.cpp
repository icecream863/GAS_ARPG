// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameModeBase.h"

#include "Game/LoadScreenSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ViewModel/MVVMLoadSlot.h"

void AAuraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 确保默认地图一定被注册到地图表里，避免蓝图漏填导致 Play 时找不到起始地图。
	if (!DefaultMapName.IsEmpty() && !DefaultMap.IsNull())
	{
		Maps.Add(DefaultMapName, DefaultMap);
	}
}

void AAuraGameModeBase::SaveSlotData(UMVVMLoadSlot* LoadSlot) const
{
	if (!LoadSlot)
	{
		return;
	}

	checkf(LoadScreenSaveGameClass, TEXT("GameMode 未设置 LoadScreenSaveGameClass"));

	const FString SlotName = LoadSlot->GetLoadSlotName();
	const int32 SlotIndex = LoadSlot->GetSlotIndex();

	// 覆盖保存前先删除旧文件，保证这个槽位只保留当前创建的数据。
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
	}

	USaveGame* SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	checkf(LoadScreenSaveGame, TEXT("LoadScreenSaveGameClass 必须继承 ULoadScreenSaveGame"));

	LoadScreenSaveGame->SlotName = SlotName;
	LoadScreenSaveGame->SlotIndex = SlotIndex;
	LoadScreenSaveGame->PlayerName = LoadSlot->GetPlayerName();
	LoadScreenSaveGame->MapName = LoadSlot->GetMapName();
	LoadScreenSaveGame->SaveSlotStatus = LoadSlot->GetSlotStatus();

	UGameplayStatics::SaveGameToSlot(LoadScreenSaveGame, SlotName, SlotIndex);
}

void AAuraGameModeBase::DeleteSlot(const FString& SlotName, const int32 SlotIndex) const
{
	// 【优化】统一由 GameMode 处理磁盘删除，方便以后扩展统一校验或平台差异逻辑。
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
	}
}

void AAuraGameModeBase::TravelToMap(UMVVMLoadSlot* LoadSlot) const
{
	if (!LoadSlot)
	{
		return;
	}

	const FString& MapName = LoadSlot->GetMapName();
	const TSoftObjectPtr<UWorld>* FoundMap = Maps.Find(MapName);
	if (!FoundMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("TravelToMap: 找不到地图名 %s 对应的关卡"), *MapName);
		return;
	}

	// 【优化】使用 GameMode 作为 WorldContext，避免依赖 ViewModel/UWidget 的世界上下文可用性。
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, *FoundMap);
}

ULoadScreenSaveGame* AAuraGameModeBase::GetSaveSlotData(const FString& SlotName, const int32 SlotIndex) const
{
	checkf(LoadScreenSaveGameClass, TEXT("GameMode 未设置 LoadScreenSaveGameClass"));

	USaveGame* SaveGameObject = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex);
	}
	else
	{
		// 没有磁盘文件时创建一个默认对象；默认状态是 Vacant，不会立刻写盘。
		SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	}

	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	checkf(LoadScreenSaveGame, TEXT("LoadScreenSaveGameClass 必须继承 ULoadScreenSaveGame"));
	LoadScreenSaveGame->SlotName = SlotName;
	LoadScreenSaveGame->SlotIndex = SlotIndex;
	return LoadScreenSaveGame;
}
