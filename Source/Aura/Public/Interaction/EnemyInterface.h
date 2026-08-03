// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EnemyInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UEnemyInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class AURA_API IEnemyInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/**
	 *  BlueprintCallable：该函数可以在蓝图（Blueprint）里被调用（会出现在蓝图节点里）。
		BlueprintNativeEvent：该函数是“可被蓝图重写的原生事件”。也就是：
		你可以在 C++ 里提供默认实现；
		蓝图里也可以 Override（重写）它来替换/扩展行为。
		实现方式上，带 BlueprintNativeEvent 的函数在 C++ 里通常要写一个带 _Implementation 后缀的实现函数 
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetCombatTarget(AActor* InCombatTarget);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	AActor* GetCombatTarget();
};
