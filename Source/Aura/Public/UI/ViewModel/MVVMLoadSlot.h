#pragma once

#include "CoreMinimal.h"
#include "Game/LoadScreenSaveGame.h"
#include "MVVMViewModelBase.h"
#include "MVVMLoadSlot.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSetWidgetSwitcherIndex, int32, WidgetSwitcherIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnableSelectSlotButton, bool, bEnable);

/** 单个存档槽对应的 ViewModel。 */
UCLASS()
class AURA_API UMVVMLoadSlot : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** 根据已经加载的槽位状态，通知复合 Widget 切换到对应页面。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void InitializeSlot();

	void SetLoadSlotName(const FString& InLoadSlotName);
	FString GetLoadSlotName() const { return LoadSlotName; }

	void SetSlotIndex(int32 InSlotIndex);
	int32 GetSlotIndex() const { return SlotIndex; }

	void SetPlayerName(const FString& InPlayerName);
	FString GetPlayerName() const { return PlayerName; }

	void SetMapName(const FString& InMapName);
	FString GetMapName() const { return MapName; }

	void SetPlayerLevel(int32 InPlayerLevel);
	int32 GetPlayerLevel() const { return PlayerLevel; }

	void SetSlotStatus(ESaveSlotStatus InSlotStatus);
	ESaveSlotStatus GetSlotStatus() const { return SlotStatus.GetValue(); }

	void SetPlayerStartTag(const FName& InPlayerStartTag);
	FName GetPlayerStartTag() const { return PlayerStartTag; }

	/** WidgetSwitcher 在蓝图中订阅该委托，只处理视觉切换。 */
	UPROPERTY(BlueprintAssignable, Category = "Load Screen")
	FSetWidgetSwitcherIndex SetWidgetSwitcherIndex;

	/** Taken 槽位 Widget 订阅该委托，用来启用或禁用“选择槽位”按钮。 */
	UPROPERTY(BlueprintAssignable, Category = "Load Screen")
	FEnableSelectSlotButton EnableSelectSlotButton;

private:
	// 【优化】提前采用 UE 5.8 所需的 FieldNotify，保证 Manual ViewModel 能被正确注入。
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	FString LoadSlotName;

	/** 存档槽索引，和 LoadSlotName 一起用于定位磁盘上的 SaveGame。 */
	UPROPERTY()
	int32 SlotIndex = 0;

	/** 玩家输入的角色名；下一节会绑定到 Taken 槽位的文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, Category = "Load Screen", meta = (AllowPrivateAccess = "true"))
	FString PlayerName;

	/** 当前槽位对应的地图显示名；本节只用于新建槽后立即显示，持久化在下一节实现。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, Category = "Load Screen", meta = (AllowPrivateAccess = "true"))
	FString MapName;

	/** Taken 槽位显示的玩家等级；新建槽为 1，读档时从存档对象恢复。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, Category = "Load Screen", meta = (AllowPrivateAccess = "true"))
	int32 PlayerLevel = 1;

	/** 当前槽位状态，数值直接对应 WidgetSwitcher 的页面索引。 */
	UPROPERTY()
	TEnumAsByte<ESaveSlotStatus> SlotStatus = Vacant;

	// 新建槽/读档时写入的出生点标签；Play 时写进 GameInstance，供 GameMode::ChoosePlayerStart 使用。
	UPROPERTY()
	FName PlayerStartTag;
};
