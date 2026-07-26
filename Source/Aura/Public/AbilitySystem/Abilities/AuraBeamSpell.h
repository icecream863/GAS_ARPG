// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "AuraBeamSpell.generated.h"

class AActor;
class ACharacter;
class APlayerController;
class USceneComponent;

/**
 * Beam 类技能的公共数据层。
 *
 * 蓝图负责技能的时序、Montage 和 Niagara；C++ 负责保存鼠标命中结果，
 * 这样后续的电链目标筛选、伤害和特效终点计算都能使用同一份可靠数据。
 */
UCLASS()
class AURA_API UAuraBeamSpell : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level) override;
	virtual FString GetNextLevelDescription(int32 Level) override;

	/**
	 * 从 TargetData 的 HitResult 中缓存本次施法所需的信息。
	 * 若鼠标没有命中可阻挡对象，本次 Beam 施法没有合法终点，因此立即取消。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam")
	void StoreMouseDataInfo(const FHitResult& HitResult);

	/**
	 * 一次性缓存施法者所需的运行时对象：PlayerController 和 Character。
	 * Beam 蓝图随后可以隐藏鼠标，或取得 CharacterMovement 临时锁定移动。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam")
	void StoreOwnerVariables();   

	/**
	 * 保留旧节点，防止已连接的旧蓝图在本次 C++ 更新后失效。
	 * 新逻辑请改用 StoreOwnerVariables，它会同时缓存 OwnerCharacter。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam", meta = (DeprecatedFunction, DeprecationMessage = "Use StoreOwnerVariables instead."))
	void StoreOwnerPlayerController();

	/**
	 * 从武器尖端向预期终点做一次球形追踪，并把第一处阻挡命中作为电束的真实终点。
	 * 这一步只更新本次施法缓存；蓝图随后据此决定 Cue 应挂在敌人还是施法者身上。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam")
	void TraceFirstTarget(const FVector& BeamTargetLocation);

	/**
	 * 查找首个命中敌人附近、尚存活的额外链式目标。
	 * 仅产生数据，不直接添加 Gameplay Cue 或施加伤害，表现与伤害时序仍由蓝图/GAS 控制。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam")
	void StoreAdditionalTargets(TArray<AActor*>& OutAdditionalTargets);

	/**
	 * 给一组额外链式目标添加 ShockLoop Cue。
	 * 蓝图只需要传入 StoreAdditionalTargets 返回的数组；Cue 参数和清理缓存由 C++ 统一维护。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam|ShockLoop")
	void AddShockLoopCuesToAdditionalTargets(const TArray<AActor*>& AdditionalTargets);

	/** 给单个额外目标添加 ShockLoop Cue，并缓存移除时必须复用的参数。 */
	UFUNCTION(BlueprintCallable, Category = "Beam|ShockLoop")
	void AddShockLoopCueToAdditionalTarget(AActor* AdditionalTarget);

	/** 从单个额外目标移除此前添加的 ShockLoop Cue。 */
	UFUNCTION(BlueprintCallable, Category = "Beam|ShockLoop")
	void RemoveShockLoopCueFromAdditionalTarget(AActor* AdditionalTarget);

	/** 结束 Electrocute 时统一清掉所有额外目标上的 ShockLoop Cue。 */
	UFUNCTION(BlueprintCallable, Category = "Beam|ShockLoop")
	void RemoveAllAdditionalShockLoopCues();

	/**
	 * 对首目标与额外链式目标施加一跳无 Debuff 的持续伤害。
	 * Blueprint 定时器每次触发只调用这一节点，避免复制 Spec 创建和 ForEach 连线。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam|Damage")
	void ApplyPeriodicDamage(const TArray<AActor*>& AdditionalTargets);

	/**
	 * 施法结束时对仍存活的首目标和额外目标应用一次完整伤害。
	 * 与 PeriodicDamage 不同，这次会携带 DebuffChance、Duration 等全部默认参数。
	 */
	UFUNCTION(BlueprintCallable, Category = "Beam|Damage")
	void ApplyFinalDamageToTargets(const TArray<AActor*>& AdditionalTargets);

	/** 首目标死亡时触发；GA_Electrocute 在蓝图中负责停止 Timer、清理首目标 Cue 并结束能力。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Beam|Death")
	void PrimaryTargetDied(AActor* DeadActor);

	/** 额外链式目标死亡时触发；蓝图负责从自身的 AdditionalTargets 数组中移除该目标。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Beam|Death")
	void AdditionalTargetDied(AActor* DeadActor);

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/** 鼠标射线的落点。Beam 特效和链式搜索以此作为初始终点。 */
	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	FVector MouseHitLocation = FVector::ZeroVector;

	/** 鼠标射线直接命中的 Actor；可能为空，例如点击地面时。 */
	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<AActor> MouseHitActor = nullptr;

	/** 施法者的本地 PlayerController，用于隐藏/恢复鼠标光标。 */
	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<APlayerController> OwnerPlayerController = nullptr;

	/**
	 * 施法者角色。只在激活阶段转换一次，之后蓝图可直接取得 CharacterMovement。
	 * 不能在播放起手 Montage 前禁用移动，否则 Motion Warping 的根运动也会被一并禁用。
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	/** 单次施法最多命中的总目标数，含首个目标；技能等级会进一步限制实际数量。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam", meta = (ClampMin = "1"))
	int32 MaxNumShockTargets = 5;

	/** 首个目标周围用于寻找下一跳敌人的半径，单位为厘米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam", meta = (ClampMin = "0.0"))
	float ShockTargetSearchRadius = 850.f;

	/**
	 * 持续电击的结算间隔，0.1 秒表示每秒结算 10 次。
	 * 这是技能配置而不是蓝图临时变量，默认值由 C++ 提供，GA 子类仍可在 Class Defaults 覆写。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Damage", meta = (ClampMin = "0.01"))
	float DamageDeltaTime = 0.1f;

	/** 松键后至少维持多久的施法表现，防止点击瞬间结束导致看不到电束。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Timing", meta = (ClampMin = "0.0"))
	float MinSpellTime = 0.5f;

	/** 持续电束 Cue。默认使用项目已注册的 GameplayCue.ShockLoop，也允许蓝图子类覆写。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|ShockLoop", meta = (GameplayTagFilter = "GameplayCue"))
	FGameplayTag ShockLoopGameplayCueTag;

	/** 已添加 Cue 的额外目标列表；Blueprint 不需要遍历 Map，结束时直接用这个数组清理。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> AdditionalShockLoopTargets;

	/** Add/Remove GameplayCue 必须使用同一份参数，否则 GameplayCueManager 可能找不到对应实例。 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, FGameplayCueParameters> AdditionalActorsToCueParams;

private:
	/** 本次持续电击实际标记过的目标；Ability 结束时统一恢复。 */
	TSet<TWeakObjectPtr<AActor>> BeingShockedTargets;
	void MarkTargetBeingShocked(AActor* Target);
	void ClearBeingShockedTargets();

	void BindToPrimaryTargetDeath(AActor* Target);
	void BindToAdditionalTargetDeath(AActor* Target);
	void UnbindDeathDelegates();

	/** 记录已订阅的目标，EndAbility 时必须解除，避免 InstancedPerActor 技能在结束后收到旧死亡事件。 */
	TWeakObjectPtr<AActor> BoundPrimaryTarget;
	TArray<TWeakObjectPtr<AActor>> BoundAdditionalTargets;
};
