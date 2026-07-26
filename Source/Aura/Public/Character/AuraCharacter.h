// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/PlayerInterface.h"
#include "AuraCharacter.generated.h"

class UCameraComponent;
class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class AURA_API AAuraCharacter : public AAuraCharacterBase, public IPlayerInterface
{
	GENERATED_BODY()
	
public:
	AAuraCharacter();
	
	// 服务器在 Pawn 被控制器接管时调用（常用于初始化 ASC/能力相关信息）
	virtual void PossessedBy(AController* NewController) override;
	
	// 客户端 PlayerState 复制到位后调用（用于补齐客户端初始化）
	virtual void OnRep_PlayerState() override;
	
	/** Begin PlayerInterface */
	virtual void AddToXP_Implementation(int32 InXP) override;
	virtual void LevelUp_Implementation() override;
	virtual void AddToPlayerLevel_Implementation(int32 InPlayerLevel) override;
	virtual void AddToAttributePoints_Implementation(int32 InAttributePoints) override;
	virtual void AddToSpellPoints_Implementation(int32 InSpellPoints) override;
	virtual int32 FindLevelForXP_Implementation(int32 InXP) const override;
	virtual int32 GetXP_Implementation() const override;
	virtual int32 GetSpellPoints_Implementation() const override;
	virtual int32 GetAttributePoints_Implementation() const override;
	virtual int32 GetAttributePointsReward_Implementation(int Level) const override;
	virtual int32 GetSpellPointsReward_Implementation(int Level) const override;
	/** End PlayerInterface */
	
	
	/* CombatInterface */
	// 提供给战斗接口的角色等级（通常从 PlayerState 读取）
	virtual int32 GetPlayerLevel_Implementation() override;
	/* End CombatInterface */
	
	// 角色开局初始化入口
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<UNiagaraComponent> LevelUpNiagaraComponent;
	
	
	
protected:
	// 统一初始化 AbilityActorInfo，绑定 Owner/Avatar 到 ASC, 初始化默认属性
	virtual void InitAbilityActorInfo() override;
	virtual void OnRep_Stunned() override;
	
private:
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> TopDownCameraComponent;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLevelUpParticles();
	
}; 
