#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraEffectActor.h"
#include "PickupBase.generated.h"

class USoundBase;

/**
 * 拾取物基类（药水/水晶等掉落物的公共父类）。
 * 提供可选的旋转与正弦浮动表现。
 * 【优化】课程在蓝图 BP_Pickup_Base 里手动调用 Start 函数并在 Event Tick 设置位置/旋转，
 * 这里改为 C++ 默认实现并默认开箱即用；蓝图仍可覆写或关闭。
 */
UCLASS()
class AURA_API AAuraPickupBase : public AAuraEffectActor
{
	GENERATED_BODY()

public:
	AAuraPickupBase();

	virtual void Tick(float DeltaTime) override;

	/** 是否每帧旋转。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	bool bRotates = true;

	/** 旋转速度（度/秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float RotationRate = 45.f;

	/** 是否启用正弦上下浮动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	bool bSinusoidalMovement = false;

	/** 正弦振幅（上下浮动高度，厘米）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SineAmplitude = 8.f;

	/** 正弦周期常数；实际周期 = 2π / 该值（越大浮动越快）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SinePeriodConstant = 4.f;

	/** 浮动基准位置；StartSinusoidalMovement 会重置为当前位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	FVector InitialLocation;

	/** 每帧计算出的位置（C++ 直接应用，蓝图也可读取/修改）。 */
	UPROPERTY(BlueprintReadWrite, Category = "Pickup Movement")
	FVector CalculatedLocation;

	/** 每帧计算出的旋转（C++ 直接应用，蓝图也可读取/修改）。 */
	UPROPERTY(BlueprintReadWrite, Category = "Pickup Movement")
	FRotator CalculatedRotation;

	/** 默认开箱即用；非拾取物用途可在蓝图里关闭。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	bool bStartWithPickupMovement = true;

	// ---- Spawn Effect（出生弹跳表现）----
	/**
	 * 【优化】课程在 BP_Pickup_Base 里用 Timeline 曲线实现出生弹跳与缩放；
	 * 这里改为 C++ 解析公式（正弦抛物线 + 反弹），参数化且默认开箱即用，
	 * 效果等价，无需连接蓝图时间线。
	 */

	/** 出生动画总时长（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SpawnDuration = 1.5f;

	/** 出生弹跳的顶点高度（厘米）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SpawnApex = 100.f;

	/** 出生缩放的最大过冲幅度（相对 1，0.2 表示短暂放大 20%）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SpawnScaleOvershoot = 0.2f;

	/** 落地时的轻微反弹幅度（0~1，0 表示不反弹）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float SpawnBounceAmount = 0.15f;

	/** 出生动画结束后掉落物的最终停留高度相对生成点的 Z 偏移（负值更贴地）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Movement")
	float GroundRestingZOffset = 0.f;

	// ---- Pickup Sounds（拾取物音效）----
	/** 【优化】课程在蓝图 BeginPlay/落地判断/Event Destroyed 里播放；
	 *  这里改为 C++ 直接播放，蓝图只需指定音效资产。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Sounds")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Sounds")
	TObjectPtr<USoundBase> GroundImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup Sounds")
	TObjectPtr<USoundBase> ConsumeSound;

	/** 从当前位置开始正弦浮动（重置基准点与计时）。 */
	UFUNCTION(BlueprintCallable, Category = "Pickup Movement")
	void StartSinusoidalMovement();

	/** 从当前朝向开始旋转。 */
	UFUNCTION(BlueprintCallable, Category = "Pickup Movement")
	void StartRotation();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 出生动画是否进行中；结束后切换到旋转/浮动。 */
	bool bSpawnAnimationActive = false;

	/** 出生动画累计时间。 */
	float SpawnTime = 0.f;

	/** 落地音效是否已播放（出生动画阶段只播一次）。 */
	bool bGroundImpactPlayed = false;

	/** 每帧更新旋转与浮动并应用到 Actor。 */
	void ItemMovement(float DeltaTime);

	/** 正弦浮动累计时间；每到 2π/SinePeriodConstant 归零防溢出。 */
	UPROPERTY()
	float RunningTime = 0.f;
};
