// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "OverlayWidgetController.generated.h"


class UAbilityInfo;
// datatable 的 row struct， 需要继承 FTableRowBase
USTRUCT(BlueprintType)
struct FUIWidgetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MessageTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Message = FText();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UAuraUserWidget> MessageWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Image = nullptr;

};

struct FOnAttributeChangeData;

// Delegate 类型命名习惯通常以 Signature 结尾
// 这里定义了一个动态多播委托，参数是一个 float 类型的新值。当属性变化时，这个委托会被广播，界面可以绑定这个委托来更新显示。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelChangedSignature, int32, NewLevel, bool, bLevelUp);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageUIWidgetRowSignature, FUIWidgetRow, Row);
// 这里不用指针，是因为 FUIWidgetRow 里没有 UObject 派生类的成员变量，且结构体本身不大，直接传值更方便；如果结构体里有 UObject 派生类的成员变量，或者结构体本身较大，就应该传指针以避免性能问题。
/* 动态委托的参数限制：动态委托的参数类型只能是：
蓝图支持的基本类型（int32、float、bool 等）
UObject 派生类的指针（例如 AActor*、UTexture2D*）
结构体（USTRUCT 标记的类型）
其他蓝图可识别的特殊类型（如 FString、FName、FText） */




/**
 *
 */
UCLASS()
class AURA_API UOverlayWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
public:

	// 初始化 UI：把当前属性值广播给界面（进入关卡/创建 UI 时用）
	virtual void BroadcastInitialValues() override;

	// 绑定 ASC/属性变化回调（把属性变化转为 UI 事件）
	virtual void BindCallbacksToDependencies() override;
	/*你这里看起来像 Broadcast 了两次，其实是两种职责：
	BroadcastInitialValues()
	UI 初始化时，主动把当前值发一次
	BindCallbacksToDependencies() 里的 Broadcast
	属性以后发生变化时，再自动发一次
	*/

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangedSignature OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Message")
	FMessageUIWidgetRowSignature MessageWidgetRowDelegate;



	UPROPERTY(BlueprintAssignable, Category = "GAS|XP")
	FOnAttributeChangedSignature OnXPPercentChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Level")
	FOnLevelChangedSignature OnPlayerLevelChangedDelegate;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data")
	TObjectPtr<UDataTable> MessageWidgetDataTable;



	template<typename T>
	static T* GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag);

	void OnXPChanged(int32 NewXP);
	void OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PreviousSlot) const;
};


template <typename T>
T* UOverlayWidgetController::GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag)
{
	return DataTable->FindRow<T>(Tag.GetTagName(), FString() );
}
