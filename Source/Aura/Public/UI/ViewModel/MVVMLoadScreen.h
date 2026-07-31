#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Templates/SubclassOf.h"
#include "MVVMLoadScreen.generated.h"

class UMVVMLoadSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSlotSelected);

/**
 * 加载菜单的根 ViewModel。
 * 它拥有全部槽位 ViewModel，并通过索引把槽位数据提供给对应的复合 Widget。
 */
UCLASS()
class AURA_API UMVVMLoadScreen : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void InitializeLoadSlots();
	void LoadData();

	UFUNCTION(BlueprintPure, Category = "Load Screen")
	UMVVMLoadSlot* GetLoadSlotViewModelByIndex(int32 Index) const;

	/** 已有槽位中的“选择槽位”按钮入口。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void SelectSlotButtonPressed(int32 Slot);

	/** 空槽位中的“新游戏”按钮入口。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void NewGameButtonPressed(int32 Slot);

	/** 输入名字后的“创建槽位”按钮入口。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void NewSlotButtonPressed(int32 Slot, const FString& EnteredName);

	/** 主界面 Play 按钮入口。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void PlayButtonPressed();

	/** 确认弹窗里的 Delete 按钮入口。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void DeleteButtonPressed();

	/** 主加载界面订阅该委托，用来在选中有效槽位后启用 Play/Delete 按钮。 */
	UPROPERTY(BlueprintAssignable, Category = "Load Screen")
	FSlotSelected SlotSelected;

	void SetNumLoadSlots(int32 InNumLoadSlots);
	int32 GetNumLoadSlots() const { return NumLoadSlots; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TSubclassOf<UMVVMLoadSlot> LoadSlotViewModelClass;

private:
	// 【优化】UPROPERTY 容器本身会跟踪其中的 UObject，无需再维护三个重复指针。
	UPROPERTY()
	TMap<int32, TObjectPtr<UMVVMLoadSlot>> LoadSlots;

	/** 当前被选中的 Taken 槽位。删除/播放都依赖它。 */
	UPROPERTY()
	TObjectPtr<UMVVMLoadSlot> SelectedSlot;

	// 【优化】UE 5.8 的 Property Path ViewModel 必须至少存在一条实际 FieldNotify 绑定。
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	int32 NumLoadSlots = 3;
};
