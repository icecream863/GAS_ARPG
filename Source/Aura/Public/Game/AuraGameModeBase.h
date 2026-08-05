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
class ACharacter;
class ULootTiers;
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

	/** 根据 GameInstance 当前槽位读取运行中的存档对象。 */
	ULoadScreenSaveGame* RetrieveInGameSaveData() const;

	/** 将运行中的存档对象写回当前 GameInstance 槽位，并同步出生点标签。 */
	void SaveInGameProgressData(ULoadScreenSaveGame* SaveObject) const;

	/**
	 * 保存当前世界状态：遍历所有实现了 USaveInterface 的 Actor，
	 * 序列化它们带 SaveGame 说明符的成员变量，按地图资产名写进存档。
	 * DestinationMapAssetName 非空时（地图传送），把目标地图资产名与显示名一并写入存档。
	 */
	void SaveWorldState(UWorld* World, const FString& DestinationMapAssetName = TEXT("")) const;

	/** 根据地图资产名反查 Maps 里的用户可见地图名；找不到返回空串。 */
	FString GetMapNameFromMapAssetName(const FString& MapAssetName) const;

	/**
	 * 加载当前世界状态：遍历实现了 USaveInterface 的 Actor，
	 * 按 ActorName 匹配存档数据，反序列化变量并调用 LoadActor 恢复。
	 */
	void LoadWorldState(UWorld* World) const;

	FString GetDefaultMapName() const { return DefaultMapName; }

	/** 新游戏默认进入地图的资产名（用于死亡重生/读档旅行）。 */
	FString GetDefaultMapAssetName() const { return DefaultMap.ToSoftObjectPath().GetAssetName(); }

	/** 玩家死亡后的重生入口：读取当前槽位存档，回到存档记录的地图。 */
	void PlayerDied(ACharacter* DeadCharacter);

	// 新建角色首次进入默认地图时使用的 PlayerStart 标签；新建槽时写入 GameInstance。
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	FName DefaultPlayerStartTag;

	// 【存档/读档】重写 ChoosePlayerStart，按 PlayerStartTag 选择出生点。
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Character Class Defaults")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;

	/** 敌人死亡掉落用的战利品档次数据资产（蓝图里设置）。 */
	UPROPERTY(EditDefaultsOnly, Category = "Loot Tiers")
	TObjectPtr<ULootTiers> LootTiers;

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
