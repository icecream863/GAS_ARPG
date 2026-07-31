// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuraMainMenuWidget.generated.h"

/**
 * 主菜单 Widget 的语义基类。
 * 布局、文字、字体和简单按钮事件保留在 WBP_MainMenu 中，方便在 UMG Designer 里预览和调整。
 */
UCLASS()
class AURA_API UAuraMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
};
