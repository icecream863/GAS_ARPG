// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AuraAbilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/CharacterClassInfo.h"
#include "AuraAbilitySystemLibrary.generated.h"

class UAbilityInfo;
class AAuraHUD;
struct FWidgetControllerParams;
class USpellMenuWidgetController;
struct FGameplayEffectContextHandle;
class UAbilitySystemComponent;
class UAttributeMenuWidgetController;
class UOverlayWidgetController;
/**
 * 
 */
UCLASS()
class AURA_API UAuraAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | WidgetController", meta = (DefaultToSelf = "WorldContextObject"))
	static bool MakeWidgetControllerParams(const UObject* WorldContextObject, FWidgetControllerParams& OutWCParams, AAuraHUD*& OutAuraHUD);
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | WidgetController", meta = (DefaultToSelf = "WorldContextObject"))
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | WidgetController", meta = (DefaultToSelf = "WorldContextObject"))
	static UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | WidgetController", meta = (DefaultToSelf = "WorldContextObject"))
	static USpellMenuWidgetController* GetSpellMenuWidgetController(const UObject* WorldContextObject);
	
	
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | CharacterClassDefaults")
	static void InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC);
	
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | CharacterClassDefaults")
	static void GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass);
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | CharacterClassDefaults")
	static UCharacterClassInfo* GetCharacterClassInfo(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | Ability Info")
	static UAbilityInfo* GetAbilityInfo(const UObject* WorldContextObject);
	
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")	
	static bool IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle);
	
	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")	
	static bool IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static bool IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static float GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static float GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static float GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static FGameplayTag GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static FVector GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static FVector GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle);
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")	
	static void SetBlockedHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, bool bInIsBlockedHit);
	//UPARAM(ref) 的作用是：告诉蓝图系统“这个参数是按引用传递的输入/输出参数”，让蓝图节点能真正修改调用者传进来的那个变量。不加可能变成 输出，因为是非const的引用参数,蓝图会当成输出。
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")	
	static void SetCriticalHit(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetIsSuccessfulDebuff(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, bool bInIsSuccessfulDebuff);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetDebuffDamage(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, float InDebuffDamage);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetDebuffDuration(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, float InDebuffDuration);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetDebuffFrequency(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, float InDebuffFrequency);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetDamageType(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FGameplayTag& InDamageType);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetDeathImpulse(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector& InDeathImpulse);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static void SetKnockbackForce(UPARAM(ref) FGameplayEffectContextHandle& EffectContextHandle, const FVector& InKnockbackForce);

	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayEffects")
	static FGameplayEffectContextHandle ApplyDamageEffect(const FDamageEffectParams& DamageEffectParams);
	
	UFUNCTION(BlueprintCallable, Category = "AuraAbilitySystemLibrary | GameplayMechanics")	
	static void GetLivePlayersWithinRadius(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors,const TArray<AActor*>& ActorsToIgnore, float Radius,const FVector& SphereOrigin);

	UFUNCTION(BlueprintPure, Category = "AuraAbilitySystemLibrary | GameplayMechanics")	
	static bool IsNotFriend(const AActor* FirstActor, const AActor* SecondActor);
	
	static int32 GetRewardForClassAndLevel(const UObject* WorldContextObject, const ECharacterClass CharacterClass, const int32 CharacterLevel);
	
};
