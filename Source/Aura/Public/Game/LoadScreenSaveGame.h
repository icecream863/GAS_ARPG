#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "LoadScreenSaveGame.generated.h"

class UGameplayAbility;

/** 存档槽当前应该显示哪一种界面。数值顺序必须匹配 WidgetSwitcher 页签顺序。 */
UENUM(BlueprintType)
enum ESaveSlotStatus : uint8
{
	Vacant UMETA(DisplayName = "Vacant"),
	EnterName UMETA(DisplayName = "Enter Name"),
	Taken UMETA(DisplayName = "Taken")
};

/** 保存一个能力所需的全部信息；读档时用它重新创建 AbilitySpec。 */
USTRUCT(BlueprintType)
struct FSavedAbility
{
	GENERATED_BODY()

	/** 能力类本身，从 AbilityInfo 数据资产里按 AbilityTag 反查得到。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class Defaults")
	TSubclassOf<UGameplayAbility> GameplayAbility;

	/** 能力标签，如 Abilities.Fire.FireBolt，是整套存档/读档体系的身份标识。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilityTag = FGameplayTag::EmptyTag;

	/** 能力状态（Locked/Eligible/Unlocked/Equipped），决定读档后能否装备/升级。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilityStatus = FGameplayTag::EmptyTag;

	/** 输入槽标签，保存该能力当前绑定的按键槽。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilitySlot = FGameplayTag::EmptyTag;

	/** 能力类型（攻击/被动等），用于读档后区分处理。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AbilityType = FGameplayTag::EmptyTag;

	/** 能力等级，保存时来自 AbilitySpec.Level。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 AbilityLevel = 1;
};

/**
 * 两个存档能力是否视为同一个：只比较 AbilityTag。
 * TArray::AddUnique 需要 operator== 才能去重，避免同一技能被重复写入存档。
 */
FORCEINLINE bool operator==(const FSavedAbility& Left, const FSavedAbility& Right)
{
	return Left.AbilityTag.MatchesTagExact(Right.AbilityTag);
}

/** 单个 Actor 的存档数据：用 ActorName 标识，可选 Transform 和 SaveGame 标记变量的序列化字节。 */
USTRUCT(BlueprintType)
struct FSavedActor
{
	GENERATED_BODY()

	/** 持久化 Actor 的名字（如检查点），名字恒定，用来在下次加载时匹配同一个 Actor。 */
	UPROPERTY()
	FName ActorName = NAME_None;

	/** 需要恢复位置/旋转/缩放时保存的变换。 */
	UPROPERTY()
	FTransform Transform = FTransform::Identity;

	/** 该 Actor 上所有标记了 SaveGame 说明符的成员变量的序列化字节。 */
	UPROPERTY()
	TArray<uint8> Bytes;
};

/** 两个存档 Actor 只要 ActorName 相同就视为同一个（供 AddUnique 去重）。 */
FORCEINLINE bool operator==(const FSavedActor& Left, const FSavedActor& Right)
{
	return Left.ActorName == Right.ActorName;
}

/** 一个地图的世界状态：包含该地图里所有需要保存的 Actor 数据。 */
USTRUCT(BlueprintType)
struct FSavedMap
{
	GENERATED_BODY()

	/** 地图资产的实际名称（不是 UI 显示名），用于区分不同地图的存档。 */
	UPROPERTY()
	FString MapAssetName = TEXT("");

	/** 该地图下所有已保存的 Actor 数据。 */
	UPROPERTY()
	TArray<FSavedActor> SavedActors;
};

/** 加载界面使用的存档数据对象。 */
UCLASS()
class AURA_API ULoadScreenSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 按地图资产名查找已保存的地图数据；找不到时返回空 FSavedMap。 */
	FSavedMap GetSavedMapWithMapName(const FString& InMapName) const;

	/** 存档里是否已经存在指定地图资产名的地图数据。 */
	bool HasMap(const FString& InMapName) const;

	/** 磁盘存档槽名，用来定位 SaveGame 文件。 */
	UPROPERTY()
	FString SlotName;

	/** 磁盘存档槽索引，和 SlotName 一起唯一定位一个 SaveGame 文件。 */
	UPROPERTY()
	int32 SlotIndex = 0;

	/** 玩家输入的角色名，和磁盘槽名分开，允许多个槽使用相同角色名。 */
	UPROPERTY()
	FString PlayerName = TEXT("Default Name");

	/** 玩家最后保存时所在的地图显示名。 */
	UPROPERTY()
	FString MapName = TEXT("Default Map Name");

	/** 保存该槽位下次进入加载菜单时应该显示的状态。 */
	UPROPERTY()
	TEnumAsByte<ESaveSlotStatus> SaveSlotStatus = Vacant;

	/** 【优化】首进标记只在第一次进入该存档槽时为 true，首次落盘后会被写成 false。 */
	UPROPERTY()
	bool bFirstTimeLoadIn = true;

	// 上次存档时的出生点标签；读档后 Play 时写进 GameInstance，供 ChoosePlayerStart 选择 PlayerStart。
	UPROPERTY()
	FName PlayerStartTag;

	// 玩家数值：下一次进入游戏时要恢复的角色状态。
	// 默认 1：从未保存过的槽位进入加载菜单时也显示 1 级，而不是 0。
	UPROPERTY()
	int32 PlayerLevel = 1;

	UPROPERTY()
	int32 XP = 0;

	UPROPERTY()
	int32 SpellPoints = 0;

	UPROPERTY()
	int32 AttributePoints = 0;

	UPROPERTY()
	float Strength = 0.f;

	UPROPERTY()
	float Intelligence = 0.f;

	UPROPERTY()
	float Resilience = 0.f;

	UPROPERTY()
	float Vigor = 0.f;

	// 已保存能力的完整信息；下次读档时据此恢复技能。
	UPROPERTY()
	TArray<FSavedAbility> SavedAbilities;

	// 世界状态：每个去过的地图对应一条 FSavedMap，内含该地图所有已保存 Actor 的数据。
	UPROPERTY()
	TArray<FSavedMap> SavedMaps;
};
