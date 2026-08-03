#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AuraGameInstance.generated.h"

/**
 * 跨关卡持久的 GameInstance：关卡切换时仍存活，用来在 GameMode 选择 PlayerStart 之前
 * 提供出生点标签，以及读档所需的 Slot 名/索引。
 */

UCLASS()
class AURA_API UAuraGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 进入新关卡时 GameMode 据此挑选 PlayerStart；默认空，由 LoadScreen 设置。
	UPROPERTY(BlueprintReadWrite, Category = "Load Screen")
	FName PlayerStartTag = FName("Check1");

	// 读档定位用：新建槽时由 LoadScreen 写入。
	UPROPERTY(BlueprintReadWrite, Category = "Load Screen")
	FString LoadSlotName;

	UPROPERTY(BlueprintReadWrite, Category = "Load Screen")
	int32 LoadSlotIndex = 0;
};
