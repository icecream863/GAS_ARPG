#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LoadScreenHUD.generated.h"

class UMVVMLoadScreen;
class ULoadScreenWidget;

/**
 * 加载菜单专用 HUD。
 * 它负责先创建 ViewModel，再创建 Widget，最后触发蓝图侧的短初始化入口。
 */
UCLASS()
class AURA_API ALoadScreenHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Load Screen")
	UMVVMLoadScreen* GetLoadScreenViewModel() const { return LoadScreenViewModel; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TSubclassOf<ULoadScreenWidget> LoadScreenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Load Screen")
	TSubclassOf<UMVVMLoadScreen> LoadScreenViewModelClass;

	UPROPERTY()
	TObjectPtr<ULoadScreenWidget> LoadScreenWidget;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMVVMLoadScreen> LoadScreenViewModel;
};
