#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AreYouSureWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAreYouSureButtonClicked);

/**
 * 通用二次确认弹窗基类。
 * Designer 负责外观；C++ 只负责确认/取消两个按钮的语义事件。
 */
UCLASS()
class AURA_API UAreYouSureWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 【优化】保留 Confirm 命名作为兼容入口，避免上一节已完成的蓝图绑定失效。 */
	UPROPERTY(BlueprintAssignable, Category = "Are You Sure")
	FOnAreYouSureButtonClicked ConfirmButtonClicked;

	/** 删除按钮点击。课程里会让 LoadScreen 订阅这个事件来真正删除槽位。 */
	UPROPERTY(BlueprintAssignable, Category = "Are You Sure")
	FOnAreYouSureButtonClicked DeleteButtonClicked;

	/** 取消按钮点击。LoadScreen 会订阅它，用来恢复 Play/Delete 按钮。 */
	UPROPERTY(BlueprintAssignable, Category = "Are You Sure")
	FOnAreYouSureButtonClicked CancelButtonClicked;



	/** 课程命名：Delete 按钮 OnClicked 入口。内部直接复用删除广播。 */
	// 和 LoadScreen ViewModel 里有同名函数，但这个用于广播， 那个用于回调 真正删除槽位。
	UFUNCTION(BlueprintCallable, Category = "Are You Sure")
	void DeleteButtonPressed();

	/** 给 WBP_AreYouSure 的 Cancel 按钮 OnClicked 调用。 */
	UFUNCTION(BlueprintCallable, Category = "Are You Sure")
	void CancelButtonPressed();
};
