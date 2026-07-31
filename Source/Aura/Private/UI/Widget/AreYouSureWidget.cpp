#include "UI/Widget/AreYouSureWidget.h"



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
