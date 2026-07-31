// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class UAbilityInfo;
class UCharacterClassInfo;
class ULoadScreenSaveGame;
class UMVVMLoadSlot;
class USaveGame;
class UWorld;
/**
 * 
 */
UCLASS()
class AURA_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	// 【优化】SlotIndex 已经存入 UMVVMLoadSlot，保存时不再额外传 Slot 参数，避免两份索引不一致。
	void SaveSlotData(UMVVMLoadSlot* LoadSlot) const;

	// 【优化】删除存档也由 GameMode 统一管理，避免 ViewModel 直接碰磁盘。
	void DeleteSlot(const FString& SlotName, int32 SlotIndex) const;

	// 【优化】旅行逻辑也由 GameMode 统一管理，ViewModel 只负责发出“Play”语义。
	void TravelToMap(UMVVMLoadSlot* LoadSlot) const;

	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;

	FString GetDefaultMapName() const { return DefaultMapName; }

	UPROPERTY(EditDefaultsOnly, Category = "Character Class Defaults")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;

	/** 新建存档槽时显示给玩家看的默认地图名。 */
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	FString DefaultMapName;

	/** 新游戏默认进入的地图资产。SoftObjectPtr 不会在菜单里强制加载地图。 */
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TSoftObjectPtr<UWorld> DefaultMap;

	/** 用玩家可见地图名查找实际地图资产；后续点击 Play 时会用它旅行到对应地图。 */
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;
};
