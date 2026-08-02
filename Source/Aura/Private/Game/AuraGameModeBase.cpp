// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameModeBase.h"

#include "EngineUtils.h"
#include "Aura/AuraLogChannels.h"
#include "Game/AuraGameInstance.h"
#include "Game/LoadScreenSaveGame.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/SaveInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
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
	LoadScreenSaveGame->PlayerStartTag = LoadSlot->GetPlayerStartTag();
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

AActor* AAuraGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// 取得关卡里所有的 PlayerStart。
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Actors);

	// 没有任何 PlayerStart 时返回 nullptr（引擎会在世界原点生成，无可控出生点）。
	if (Actors.Num() == 0)
	{
		return nullptr;
	}

	// 默认使用第一个找到的 PlayerStart。
	AActor* SelectedActor = Actors[0];

	// 出生点标签来自 GameInstance（跨关卡持久），由 LoadScreen 在新建槽时设置。
	const FName DesiredTag = [&]() -> FName
	{
		if (const UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance()))
		{
			return AuraGameInstance->PlayerStartTag;
		}
		return NAME_None;
	}();

	for (AActor* Actor : Actors)
	{
		if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
		{
			if (DesiredTag != NAME_None && PlayerStart->PlayerStartTag == DesiredTag)
			{
				SelectedActor = PlayerStart;
				break; // 找到第一个匹配的即可，避免无意义继续遍历
			}
		}
	}

	return SelectedActor;
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

ULoadScreenSaveGame* AAuraGameModeBase::RetrieveInGameSaveData() const
{
	const UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGameInstance || AuraGameInstance->LoadSlotName.IsEmpty())
	{
		return nullptr;
	}

	return GetSaveSlotData(AuraGameInstance->LoadSlotName, AuraGameInstance->LoadSlotIndex);
}

void AAuraGameModeBase::SaveInGameProgressData(ULoadScreenSaveGame* SaveObject) const
{
	if (!SaveObject)
	{
		return;
	}

	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGameInstance || AuraGameInstance->LoadSlotName.IsEmpty())
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(
		SaveObject,
		AuraGameInstance->LoadSlotName,
		AuraGameInstance->LoadSlotIndex);
	
	AuraGameInstance->PlayerStartTag = SaveObject->PlayerStartTag;
}

void AAuraGameModeBase::SaveWorldState(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	// PIE/流送关卡会在世界名前面加前缀（如 UEDPIE_0_），去掉后才是真实地图资产名。
	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGameInstance || AuraGameInstance->LoadSlotName.IsEmpty())
	{
		return;
	}

	ULoadScreenSaveGame* SaveGame =
		GetSaveSlotData(AuraGameInstance->LoadSlotName, AuraGameInstance->LoadSlotIndex);
	if (!SaveGame)
	{
		return;
	}

	// 第一次保存这个地图时，先往 SavedMaps 里补一条空记录。
	if (!SaveGame->HasMap(WorldName))
	{
		FSavedMap NewSavedMap;
		NewSavedMap.MapAssetName = WorldName;
		SaveGame->SavedMaps.Add(NewSavedMap);
	}

	// 取出当前地图的存档条目并清空旧 Actor 数据，下面重新填充。
	FSavedMap SavedMap = SaveGame->GetSavedMapWithMapName(WorldName);
	SavedMap.SavedActors.Empty();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || !Actor->Implements<USaveInterface>())
		{
			continue;
		}

		FSavedActor SavedActor;
		SavedActor.ActorName = Actor->GetFName();
		SavedActor.Transform = Actor->GetTransform();

		// 把 Actor 上所有带 SaveGame 说明符的成员变量序列化进字节数组。
		FMemoryWriter MemoryWriter(SavedActor.Bytes);
		FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, true);
		Archive.ArIsSaveGame = true;
		Actor->Serialize(Archive);

		SavedMap.SavedActors.AddUnique(SavedActor);
		
	}

	// 用新填充的数据覆盖存档里同名的旧条目（引用直接改写，数组大小不变）。
	for (FSavedMap& MapToReplace : SaveGame->SavedMaps)
	{
		if (MapToReplace.MapAssetName == WorldName)
		{
			MapToReplace = SavedMap;
			break;
		}
	}

	UGameplayStatics::SaveGameToSlot(
		SaveGame,
		AuraGameInstance->LoadSlotName,
		AuraGameInstance->LoadSlotIndex);
}

void AAuraGameModeBase::LoadWorldState(UWorld* World) const
{
	if (!World)
	{
		return;
	}

	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGameInstance || AuraGameInstance->LoadSlotName.IsEmpty())
	{
		return;
	}

	// 没有存档文件就没必要继续。
	if (!UGameplayStatics::DoesSaveGameExist(
			AuraGameInstance->LoadSlotName, AuraGameInstance->LoadSlotIndex))
	{
		return;
	}

	ULoadScreenSaveGame* SaveGame = Cast<ULoadScreenSaveGame>(UGameplayStatics::LoadGameFromSlot(
		AuraGameInstance->LoadSlotName, AuraGameInstance->LoadSlotIndex));
	if (!SaveGame)
	{
		UE_LOG(LogAura, Error, TEXT("LoadWorldState: Failed to load slot %s"),
			*AuraGameInstance->LoadSlotName);
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor->Implements<USaveInterface>())
		{
			continue;
		}

		for (const FSavedActor& SavedActor : SaveGame->GetSavedMapWithMapName(WorldName).SavedActors)
		{
			if (SavedActor.ActorName != Actor->GetFName())
			{
				continue;
			}

			// 需要恢复位置的 Actor（如可移动物体）才设置 Transform。
			if (ISaveInterface::Execute_ShouldLoadTransform(Actor))
			{
				Actor->SetActorTransform(SavedActor.Transform);
			}

			// 把存档字节反序列化回 Actor 上带 SaveGame 说明符的成员变量。
			FMemoryReader MemoryReader(SavedActor.Bytes);
			FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
			Archive.ArIsSaveGame = true;
			Actor->Serialize(Archive);

			// 调用各 Actor 自己的恢复逻辑（检查点重新发光等）。
			ISaveInterface::Execute_LoadActor(Actor);
			break;
		}
	}
}
