#include "UI/Widget/AreYouSureWidget.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Components/TextBlock.h"

void UAreYouSureWidget::DeleteButtonPressed()
{
	// 只广播删除事件，避免一次确认同时触发两套语义事件。
	DeleteButtonClicked.Broadcast();
	RemoveFromParent();
}

void UAreYouSureWidget::CancelButtonPressed()
{
	// 取消只需要通知外部恢复按钮，然后关闭自己。
	CancelButtonClicked.Broadcast();
	RemoveFromParent();
}

float UAreYouSureWidget::GetCenteredViewportXPosition() const
{
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	return ViewportSize.X * 0.5f;
}

void UAreYouSureWidget::SetMessageText(const FText& InMessageText)
{
	if (UTextBlock* MessageText = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Message"))))
	{
		MessageText->SetText(InMessageText);
	}
}

void UAreYouSureWidget::SetConfirmButtonText(const FText& InButtonText)
{
	// WBP_AreYouSure 的确认按钮是 WBP_WideButton，其内部文字控件名为 “Text”。
	if (UUserWidget* ConfirmButton = Cast<UUserWidget>(GetWidgetFromName(TEXT("Button_Delete"))))
	{
		if (UTextBlock* ButtonText = Cast<UTextBlock>(ConfirmButton->GetWidgetFromName(TEXT("Text"))))
		{
			ButtonText->SetText(InButtonText);
		}
	}
}

void UAreYouSureWidget::ShowAsCenteredConfirmation(const float ViewportY)
{
	AddToViewport();
	SetAlignmentInViewport(FVector2D(0.5f, 0.f));
	SetPositionInViewport(FVector2D(GetCenteredViewportXPosition(), ViewportY));
}
