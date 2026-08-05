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

	/**
	 * 【优化】课程在 WBP_AreYouSure 里新建 CenteredXPosition 纯函数；
	 * 这里放到 C++（与 LoadScreen 的居中逻辑一致），Overlay 蓝图直接调用即可。
	 * 返回视口水平中心 X，配合 SetAlignmentInViewport(0.5, 0) 使用。
	 */
	UFUNCTION(BlueprintPure, Category = "Are You Sure")
	float GetCenteredViewportXPosition() const;

	/**
	 * 【优化】设置弹窗提示文本（LoadScreen 删除确认 / Overlay 退出确认共用同一弹窗，
	 * 各自实例独立设置，互不影响）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Are You Sure")
	void SetMessageText(const FText& InMessageText);

	/** 设置确认按钮的文字（例如删除确认显示 “Delete”，退出确认显示 “Quit”）。 */
	UFUNCTION(BlueprintCallable, Category = "Are You Sure")
	void SetConfirmButtonText(const FText& InButtonText);

	/**
	 * 以居中的方式显示在视口上（AddToViewport + 水平居中 + 指定 Y）。
	 * 【优化】LoadScreen 的 C++ 已有同款定位逻辑；这里放到弹窗自身，
	 * Overlay 蓝图无需再手动 SetPositionInViewport/SetAlignmentInViewport。
	 */
	UFUNCTION(BlueprintCallable, Category = "Are You Sure")
	void ShowAsCenteredConfirmation(float ViewportY = 100.f);
};
