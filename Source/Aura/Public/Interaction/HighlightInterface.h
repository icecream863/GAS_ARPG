#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UHighlightInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可高亮接口：任何 Actor（敌人、地图出入口等）实现它后，
 * 玩家光标悬停时就会被 AAuraPlayerController::CursorTrace 高亮。
 */
class AURA_API IHighlightInterface
{
	GENERATED_BODY()

public:
	/** 光标悬停时开启高亮（后处理材质按 CustomDepthStencilValue 着色）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Highlight")
	void HighLightActor();

	/** 光标移开时关闭高亮。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Highlight")
	void UnHighLightActor();

	/**
	 * 允许高亮对象覆盖“点击移动”的目的地（如检查点/地图出入口的特定到达点）。
	 * 不覆盖的类（如敌人）无需实现或留空。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Highlight")
	void SetMoveToLocation(FVector& OutDestination);
};
