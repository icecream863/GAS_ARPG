#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LoadScreenSaveGame.generated.h"

/** 存档槽当前应该显示哪一种界面。数值顺序必须匹配 WidgetSwitcher 页签顺序。 */
UENUM(BlueprintType)
enum ESaveSlotStatus : uint8
{
	Vacant UMETA(DisplayName = "Vacant"),
	EnterName UMETA(DisplayName = "Enter Name"),
	Taken UMETA(DisplayName = "Taken")
};

/** 加载界面使用的存档数据对象。 */
UCLASS()
class AURA_API ULoadScreenSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
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
};
