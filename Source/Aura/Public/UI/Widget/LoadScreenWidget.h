#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadScreenWidget.generated.h"

class UMVVMLoadScreen;
class UAreYouSureWidget;

/**
 * 加载菜单系列 Widget 的 C++ 基类。
 * Designer 继续负责布局；C++ 提供统一的初始化入口、ViewModel 查找和槽位索引。
 */
UCLASS()
class AURA_API ULoadScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Load Screen")
	void BlueprintInitializeWidget();

	UFUNCTION(BlueprintPure, Category = "Load Screen")
	UMVVMLoadScreen* FindLoadScreenViewModel() const;

	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void SetSlotIndex(int32 InSlotIndex) { SlotIndex = InSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Load Screen")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Delete 按钮入口：弹出二次确认，并临时禁用 Play/Delete。 */
	UFUNCTION(BlueprintCallable, Category = "Load Screen")
	void ShowDeleteConfirmation();

	/** 【优化】让 C++ 创建弹窗时直接用视口居中点，不需要蓝图手动计算 Widget 宽度。 */
	UFUNCTION(BlueprintPure, Category = "Load Screen")
	float GetCenteredViewportXPosition() const;

protected:
	virtual void NativeDestruct() override;

	// 【优化】课程原本在蓝图基类中声明该变量；放在 C++ 后所有加载界面子类可直接复用。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Load Screen")
	int32 SlotIndex = INDEX_NONE;

	/** WBP_LoadScreen Class Defaults 指定为 WBP_AreYouSure。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Load Screen|Confirmation")
	TSubclassOf<UAreYouSureWidget> AreYouSureWidgetClass;

	/** 当前屏幕上的确认弹窗；防止连续点击 Delete 生成多个弹窗。 */
	UPROPERTY(BlueprintReadOnly, Category = "Load Screen|Confirmation")
	TObjectPtr<UAreYouSureWidget> ActiveAreYouSureWidget;

	/** 弹窗距离视口顶部的 Y 位置，保留给蓝图 Class Defaults 调整。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Load Screen|Confirmation")
	float DeleteConfirmationViewportY = 100.f;

	/** 如果蓝图没有课程函数 EnablePlayAndDeleteButtons，可实现这个事件作为备用入口。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Load Screen")
	void ReceivePlayAndDeleteButtonsEnabledChanged(bool bEnable);

private:
	void SetPlayAndDeleteButtonsEnabled(bool bEnable);

	UFUNCTION()
	void HandleDeleteConfirmationAccepted();

	UFUNCTION()
	void HandleDeleteConfirmationCancelled();
};
