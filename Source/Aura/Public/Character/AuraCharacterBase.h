// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "NiagaraSystem.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "AuraCharacterBase.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;
class UDebuffNiagaraComponent;

UCLASS(Abstract)// 不会作为实例
class AURA_API AAuraCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	
	AAuraCharacterBase();
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<FTaggedMontage> AttackMontages;
	
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//这是重载 IAbilitySystemInterface里的函数
	
	UAttributeSet* GetAttributeSet() const{ return AttributeSet; }
	
	virtual void BeginPlay() override;
	
	/** CombatInterface */
	virtual void Die(const FVector& DeathImpulse) override;
	virtual UAnimMontage* GetHitReactMontage_Implementation() override;
	virtual FVector GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag) override;
	virtual bool IsDead_Implementation() const override;
	virtual AActor* GetAvatar_Implementation()  override;
	virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() override;
	virtual UNiagaraSystem* GetBloodEffect_Implementation() override;
	virtual FTaggedMontage GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) override;
	virtual int32 GetMinionCount_Implementation() override;
	virtual void IncrementMinionCount_Implementation(int32 Amount) override;
	virtual ECharacterClass GetCharacterClass_Implementation() override;
	virtual FOnExternalGameplayModifierDependencyChange* GetExternalGameplayModifierDependencyMulticast() override;
	virtual FOnASCRegistered& GetOnASCRegisteredDelegate() override;
	virtual FOnDeath& GetOnDeathDelegate() override;
	/** End CombatInterface */
	
	

	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastHandleDeath(const FVector& DeathImpulse);
	
	/**
	 *这是一个在虚幻引擎 (Unreal Engine) 中声明的 多播 RPC (Remote Procedure Call) 函数。
	 *UFUNCTION(NetMulticast, Reliable)：
	 *NetMulticast：表示该函数如果在服务器上调用，将会在服务器以及所有连接的客户端上广播并执行。通常用于在所有端同步视觉效果或音效。
	 *Reliable：表示该网络调用是可靠的，引擎会确保数据包成功送达并执行，不会因为网络环境差而发生丢包。
	 *virtual void MulticastHandleDeath(const FVector& DeathImpulse);：表示这是一个处理角色死亡逻辑的虚函数，通常会在所有客户端同步死亡表现和冲量。
	 *在虚幻引擎中，声明为 RPC（如 NetMulticast、Server 或 Client）的函数，在 C++ 中实现时需要在函数名后加上 _Implementation 后缀。
	  对于 MulticastHandleDeath，你应该在对应的 .cpp 文件（如 AuraCharacterBase.cpp）中按照以下方式编写它的实现：
	  void AAuraCharacterBase::MulticastHandleDeath_Implementation(const FVector& DeathImpulse)
	  {
	  // 在这里编写角色死亡时需要在所有端同步执行的逻辑
	  // 例如：播放死亡动画、生成布娃娃效果、禁用碰撞、播放音效等
	  }
	  虚幻引擎的 Unreal Header Tool (UHT) 会自动生成调用底层网络代码的桩函数（Stub），并通过你编写的 _Implementation 函数来执行具体的业务逻辑。
	 */
	
	/**
* 在虚幻引擎中，这三者是远程过程调用（RPC）的类型，它们的主要区别在于函数的执行位置和调用方向：

*   **NetMulticast（多播）**：
*   **调用方**：只能由服务器调用。
*   **执行方**：在服务器以及**所有**连接的客户端上执行。
*   **用途**：用于在所有端同步视觉效果、音效或全局状态（如角色死亡、全屏广播）。
*   
*   **Server（服务器）**：
*   **调用方**：通常由客户端调用。
*   **执行方**：仅在**服务器**端执行。
*   **用途**：用于客户端向服务器发送授权请求或玩家输入（如请求开火、购买物品），服务器在执行前通常会进行作弊校验。
*   
*   **Client（客户端）**：
*   **调用方**：只能由服务器调用。
*   **执行方**：仅在**拥有该 Actor 的特定客户端**上执行。
*   **用途**：用于服务器向特定玩家发送私有信息（如更新该玩家的UI、播放只有该玩家能听到的提示音）。
	 */
	
protected:
	
	/*  Combat */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<USkeletalMeshComponent> Weapon;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	FName WeaponTipSocketName;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	FName LeftHandSocketName;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	FName RightHandSocketName;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	FName TailSocketName;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Combat")
	UNiagaraSystem* BloodEffect;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Combat")
	USoundBase* DeathSound;
	
	//不需要每个角色都 CharacterClassInfo,只需要一个全局的 CharacterClassInfo 就够了，角色类里只需要一个 CharacterClass 枚举来标识自己的职业类型就行了
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Character Class Default")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;//默认元素使
	
	/* End Combat  */
	
	/* Minions */
	int32 MinionCount = 0;
	
	bool bDead = false;
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
	virtual void InitAbilityActorInfo();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debuff")
	TObjectPtr<UDebuffNiagaraComponent> BurnDebuffComponent;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttributes;
	//用来初始化属性的GE，分别是主属性和次属性，蓝图里可以设置，或者直接在代码里设置默认值
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;
	

	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	
	virtual void InitialDefaultAttributes() const;
	
	void AddCharacterAbilities() const;

	// 广播后，注册到该委托的 MMC 会让对应 Active GameplayEffect 重新计算 Modifier Magnitude。
	FOnExternalGameplayModifierDependencyChange ExternalGameplayModifierDependencyMulticast;
	FOnASCRegistered OnASCRegistered;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnDeath OnDeath;
	
private:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupPassiveAbilities;
	
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
	
	
};
