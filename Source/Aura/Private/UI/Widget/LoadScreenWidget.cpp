#include "UI/Widget/LoadScreenWidget.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UI/HUD/LoadScreenHUD.h"
#include "UI/ViewModel/MVVMLoadScreen.h"
#include "UI/Widget/AreYouSureWidget.h"

void ULoadScreenWidget::NativeDestruct()
{
	if (ActiveAreYouSureWidget)
	{
		ActiveAreYouSureWidget->DeleteButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationAccepted);
		ActiveAreYouSureWidget->CancelButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationCancelled);
		ActiveAreYouSureWidget->RemoveFromParent();
		ActiveAreYouSureWidget = nullptr;
	}

	Super::NativeDestruct();
}

UMVVMLoadScreen* ULoadScreenWidget::FindLoadScreenViewModel() const
{
	// 【优化】从 OwningPlayer 获取 HUD，支持多人 PIE，不依赖固定的 Player 0。
	const APlayerController* OwningPlayer = GetOwningPlayer();
	const ALoadScreenHUD* LoadScreenHUD = OwningPlayer ? Cast<ALoadScreenHUD>(OwningPlayer->GetHUD()) : nullptr;
	return LoadScreenHUD ? LoadScreenHUD->GetLoadScreenViewModel() : nullptr;
}

void ULoadScreenWidget::ShowDeleteConfirmation()
{
	if (ActiveAreYouSureWidget)
	{
		return;
	}

	if (!ensureMsgf(AreYouSureWidgetClass, TEXT("WBP_LoadScreen 未设置 AreYouSureWidgetClass")))
	{
		return;
	}

	ActiveAreYouSureWidget = CreateWidget<UAreYouSureWidget>(GetOwningPlayer(), AreYouSureWidgetClass);
	if (!ActiveAreYouSureWidget)
	{
		return;
	}

	SetPlayAndDeleteButtonsEnabled(false);

	ActiveAreYouSureWidget->DeleteButtonClicked.AddDynamic(this, &ThisClass::HandleDeleteConfirmationAccepted);
	ActiveAreYouSureWidget->CancelButtonClicked.AddDynamic(this, &ThisClass::HandleDeleteConfirmationCancelled);
	ActiveAreYouSureWidget->AddToViewport();

	// 【优化】课程在蓝图里读取 SizeBox 宽度并计算 X；这里用 0.5 对齐点直接居中，逻辑更短且不依赖具体宽度。
	ActiveAreYouSureWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.f));
	ActiveAreYouSureWidget->SetPositionInViewport(FVector2D(GetCenteredViewportXPosition(), DeleteConfirmationViewportY));
}

float ULoadScreenWidget::GetCenteredViewportXPosition() const
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	return ViewportSize.X * 0.5f;
}

void ULoadScreenWidget::SetPlayAndDeleteButtonsEnabled(const bool bEnable)
{
	// 【优化】兼容课程蓝图里已经创建的 EnablePlayAndDeleteButtons(bool) 函数，
	// 这样本节不需要你再搭一份重复的按钮启用/禁用逻辑。
	if (UFunction* EnableButtonsFunction = FindFunction(TEXT("EnablePlayAndDeleteButtons")))
	{
		struct FEnablePlayAndDeleteButtonsParams
		{
			bool bEnable;
		};

		FEnablePlayAndDeleteButtonsParams Params;
		Params.bEnable = bEnable;
		ProcessEvent(EnableButtonsFunction, &Params);
		return;
	}

	ReceivePlayAndDeleteButtonsEnabledChanged(bEnable);
}

void ULoadScreenWidget::HandleDeleteConfirmationCancelled()
{
	if (ActiveAreYouSureWidget)
	{
		ActiveAreYouSureWidget->DeleteButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationAccepted);
		ActiveAreYouSureWidget->CancelButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationCancelled);
		ActiveAreYouSureWidget = nullptr;
	}

	SetPlayAndDeleteButtonsEnabled(true);
}

void ULoadScreenWidget::HandleDeleteConfirmationAccepted()
{
	if (UMVVMLoadScreen* LoadScreenViewModel = FindLoadScreenViewModel())
	{
		LoadScreenViewModel->DeleteButtonPressed();
	}

	if (ActiveAreYouSureWidget)
	{
		ActiveAreYouSureWidget->DeleteButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationAccepted);
		ActiveAreYouSureWidget->CancelButtonClicked.RemoveDynamic(this, &ThisClass::HandleDeleteConfirmationCancelled);
		ActiveAreYouSureWidget = nullptr;
	}
}
