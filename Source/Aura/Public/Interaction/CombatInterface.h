// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class UNiagaraSystem;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnASCRegistered, UAbilitySystemComponent*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, DeadActor);

USTRUCT(BlueprintType)
struct FTaggedMontage 
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag MontageTag;//对应 动画 和 动画中的通知事件标记
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag SocketTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UAnimMontage* Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USoundBase* ImpactSound = nullptr;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class AURA_API ICombatInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent)
	int32 GetPlayerLevel();  
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FVector GetCombatSocketLocation(const FGameplayTag& MontageTag);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)//这个函数只能在蓝图中实现，不能在C++中实 现，如果C++中调用这个函数，必须在蓝图中实现，否则会报错。
	void UpdateFacingTarget(const FVector& FacingTarget);
	//利用函数重载，具体类重载这个函数，而掉用只需转化成接口指针调用这个函数就好，接口指针会根据实际对象类型调用对应的重载函数。
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)//这个函数既可以在蓝图中实现，也可以在C++中实现，如果蓝图没有实现，就会调用C++中的实现。
	UAnimMontage* GetHitReactMontage();
	
	virtual void Die(const FVector& DeathImpulse) = 0;
	virtual FOnASCRegistered& GetOnASCRegisteredDelegate() = 0;
	virtual FOnDeath& GetOnDeathDelegate() = 0;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsDead() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	AActor* GetAvatar();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TArray<FTaggedMontage> GetAttackMontages();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	UNiagaraSystem* GetBloodEffect();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FTaggedMontage GetTaggedMontageByTag(const FGameplayTag& MontageTag);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	int32 GetMinionCount();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void IncrementMinionCount(int32 Amount);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	ECharacterClass GetCharacterClass();

	// MMC 可通过这个委托订阅“非 GAS Attribute 的外部依赖变化”（例如 PlayerState.Level）。
	virtual FOnExternalGameplayModifierDependencyChange* GetExternalGameplayModifierDependencyMulticast() { return nullptr; }
};
