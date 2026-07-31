#include "UI/HUD/LoadScreenHUD.h"

#include "GameFramework/PlayerController.h"
#include "UI/ViewModel/MVVMLoadScreen.h"
#include "UI/Widget/LoadScreenWidget.h"

void ALoadScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	checkf(LoadScreenViewModelClass, TEXT("BP_LoadScreenHUD 未设置 LoadScreenViewModelClass"));
	checkf(LoadScreenWidgetClass, TEXT("BP_LoadScreenHUD 未设置 LoadScreenWidgetClass"));

	// 必须先创建完整的 ViewModel 树，Widget 初始化时才能安全取得每个槽位的数据。
	LoadScreenViewModel = NewObject<UMVVMLoadScreen>(this, LoadScreenViewModelClass);
	LoadScreenViewModel->InitializeLoadSlots();

	APlayerController* OwningPlayer = GetOwningPlayerController();
	LoadScreenWidget = CreateWidget<ULoadScreenWidget>(OwningPlayer, LoadScreenWidgetClass);
	LoadScreenWidget->AddToViewport();
	LoadScreenWidget->BlueprintInitializeWidget();
	
	LoadScreenViewModel->LoadData();

	if (OwningPlayer)
	{
		FInputModeUIOnly InputMode;
		// 【优化】UE 5.8 不强制聚焦不可聚焦的根 Widget，避免 InputMode 警告。
		OwningPlayer->SetInputMode(InputMode);
		OwningPlayer->bShowMouseCursor = true;
	}
}
