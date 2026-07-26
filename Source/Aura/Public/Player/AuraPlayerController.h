// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Engine/HitResult.h"

#include "AuraPlayerController.generated.h"

class UDamageTextComponent;
class UAuraInputConfig;
class IEnemyInterface;
class UInputAction;
class UInputMappingContext;
class UAuraAbilitySystemComponent;
class UNiagaraSystem;
class USplineComponent;

/**
 * 玩家控制器：负责输入映射、光标追踪（敌人高亮/目标选择）、
 * 移动（点击移动 / WASD）、自动奔跑（样条路径）以及技能输入Tag转发。
 */
UCLASS()
class AURA_API AAuraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAuraPlayerController();
	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(Client, Reliable)//服务端调用，客户端执行， 要确保调用正确的 PlayerController（不是总假设玩家是 PlayerIndex 0 的那个），因此需要 Reliable 来保证 RPC 不丢失。
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bIsBlockedHit, bool bIsCriticalHit);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	// ---------- 输入配置 ----------

	/** 玩家的主输入映射上下文 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> AuraContext;

	/** WASD键盘移动输入动作（与Move()绑定，仅处理键盘移动，点击移动由LMB技能Tag处理） */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Shift修饰键：按下时强制LMB释放技能而非移动（原地施法） */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ShiftAction;

	/** 追踪Shift键是否按下 */
	void ShiftPressed()  { bShiftKeyDown = true; };
	void ShiftReleased() { bShiftKeyDown = false; };
	bool bShiftKeyDown = false;

	/** WASD键盘移动：根据相机朝向计算前/右方向并施加移动输入 */
	void Move(const struct FInputActionValue& InputActinValue);

	/** 每帧光标射线检测：检测光标下的Actor，用于敌人高亮和目标选择 */
	void CursorTrace();

	/** 输入死区，抑制微小输入抖动 */
	UPROPERTY(EditAnywhere, Category = "Input")
	float DeadZone = 0.15f;

	// ---------- 光标 / 敌人高亮 ----------

	/** 当前帧光标下的敌人（IEnemyInterface） */
	IEnemyInterface* ThisActor;
	/** 上一帧光标下的敌人（IEnemyInterface） */
	IEnemyInterface* LastActor;
	/** 最新一次光标射线检测的碰撞结果 */
	FHitResult CursorHit;

	// ---------- 技能输入（含点击移动） ----------
	// LMB 只有 Pressed / Released / Held，不通过 Move()，因此点击移动逻辑也在这里处理：
	//   Pressed → 判定是否选中敌人（bTargeting），停止自动奔跑
	//   Held   → 选中敌人或按Shift时转发技能；否则跟随光标直接移动
	//   Released → 转发技能；短按（<ShortPressThreshold）且非敌人非Shift → 导航寻路 + 自动奔跑

	/** GameplayTag 驱动的输入动作 Pressed / Released / Held 回调 */
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	/** 输入配置资产：SetupInputComponent时通过BindAbilityActions批量绑定Tag→技能回调 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UAuraInputConfig> InputConfig;

	// ---------- 能力系统 ----------

	/** 缓存的技能系统组件引用 */
	UPROPERTY()
	TObjectPtr<UAuraAbilitySystemComponent> AuraAbilitySystemComponent;

	/** 懒加载获取技能系统组件 */
	UAuraAbilitySystemComponent* GetASC();

	// ---------- 点击移动 / 自动奔跑 ----------

	/** 目标点：Held时跟随光标位置；Released短按时更新为导航路径终点 */
	FVector CachedDestination = FVector::ZeroVector;
	/** LMB按住累计时间，Released时与ShortPressThreshold比较区分短按/长按 */
	float FollowTime = 0.f;
	/** 短按判定阈值（秒），短按触发导航寻路+自动奔跑，长按仅跟随光标 */
	float ShortPressThreshold = 0.5f;
	/** 是否正在沿Spline自动奔跑 */
	bool bAutoRunning = false;
	/** 是否选中敌人（Pressed时若光标下有敌人则置true，Held/Released优先释放技能而非移动） */
	bool bTargeting = false;

	/** 自动奔跑到达终点时判定到达的距离容差 */
	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius = 50.f;

	/** 导航路径样条：短按松开时将寻路点填入Spline，AutoRun()沿其移动角色 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UNiagaraSystem> ClickNiagaraSystem;

	/** 每帧沿Spline移动角色，到达CachedDestination容差范围内停止 */
	void AutoRun();
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageTextComponent> DamageTextComponentClass;
};
