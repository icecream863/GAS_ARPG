// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "AuraProjectile.generated.h"

class UNiagaraSystem;
class USphereComponent;
class USceneComponent;
class UProjectileMovementComponent;

UCLASS()
class AURA_API AAuraProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AAuraProjectile();

	/**
	 * 保存真实追踪目标并在服务端监听其死亡/销毁。
	 * 目标提前消失时，投射物会立即走正常 Impact 路径，而不是依靠 Tick 轮询移动距离。
	 */
	void SetHomingTarget(AActor* Target);
	
	virtual void Destroyed() override;
	
	UPROPERTY(VisibleAnywhere)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true) )
	FDamageEffectParams DamageEffectParams;
	/**
	*作用：这是最关键的设置。当你使用 SpawnActorFromClass 节点生成这个 Actor（比如子弹）时，这个变量会直接出现在生成节点的输入引脚上。
	解决的问题：它避免了“先生成、再赋值”的尴尬。如果在赋值前子弹就撞到了物体，此时变量为空就会报错；
	使用 ExposeOnSpawn 可以确保子弹在诞生那一刻就已经持有了伤害数据。	
	*/

	// 地面点击没有可追踪的 Actor 时，由投射物持有这个虚拟目标，确保它随投射物一起被 GC。
	UPROPERTY()
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;
	
protected:
	
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 命中和客户端销毁都需要播放同一组反馈，集中处理可避免重复播放。
	void OnHit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;
	
	
private:
	
	float LifeSpan = 15.f;
	bool bHit = false;

	UFUNCTION()
	void OnHomingTargetDied(AActor* DeadActor);

	UFUNCTION()
	void OnHomingTargetDestroyed(AActor* DestroyedActor);

	void UnbindHomingTarget();
	void DetonateWhenHomingTargetIsLost();

	TWeakObjectPtr<AActor> HomingTargetActor;
	
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ImpactEffect;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> ImpactSound;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> LoopingSound;
	
	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;
	
	
};
