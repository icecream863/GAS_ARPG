#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LootTiers.generated.h"

class AActor;

/** 单个战利品项的配置：掉什么、掉率、最多掉几个、是否覆盖等级。 */
USTRUCT(BlueprintType)
struct FLootItem
{
	GENERATED_BODY()

	/** 要掉落的 Actor 类（药水/水晶等拾取物）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot Tiers|Spawning")
	TSubclassOf<AActor> LootClass;

	// 【课程】掉率与最大数量不暴露给蓝图事件图，
	// 由 GetLootItems 统一结算，避免蓝图误用原始配置。
	UPROPERTY(EditAnywhere, Category = "Loot Tiers|Spawning")
	float ChanceToSpawn = 0.f;

	UPROPERTY(EditAnywhere, Category = "Loot Tiers|Spawning")
	int32 MaxNumberToSpawn = 0;

	/** 为 true 时把掉落物等级设为掉落它的敌人等级。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot Tiers|Spawning")
	bool bLootLevelOverride = true;
};

/** 战利品档次数据资产：按配置的掉率/数量结算本次掉落的物品列表。 */
UCLASS()
class AURA_API ULootTiers : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 结算所有战利品项，返回真正要生成的物品（可能重复，数量取决于掉率与最大数量）。 */
	UFUNCTION(BlueprintCallable, Category = "Loot Tiers")
	TArray<FLootItem> GetLootItems();

	UPROPERTY(EditDefaultsOnly, Category = "Loot Tiers|Spawning")
	TArray<FLootItem> LootItems;
};
