#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/SaveInterface.h"
#include "Checkpoint.generated.h"

class USphereComponent;

/**
 * 检查点：继承 PlayerStart，带 PlayerStartTag 可被 ChoosePlayerStart 选中作为出生点。
 * 玩家进入 Sphere 触发发光；未来在此保存 PlayerStartTag 作为存档检查点。
 */
UCLASS()
class AURA_API ACheckpoint : public APlayerStart, public ISaveInterface
{
	GENERATED_BODY()

public:
	ACheckpoint(const FObjectInitializer& ObjectInitializer);

	/** 该检查点是否已被玩家触发过；SaveGame 说明符使其能被 SaveWorldState 序列化。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, EditAnywhere, Category = "Checkpoint")
	bool bReached = false;

	// 玩家到达检查点时在蓝图侧实现（发光时间线等视觉反馈）。
	UFUNCTION(BlueprintImplementableEvent)
	void CheckpointReached(UMaterialInstanceDynamic* DynamicMaterialInstance);

	// ---- SaveInterface ----
	/** 检查点位置固定，读档时不恢复 Transform。 */
	virtual bool ShouldLoadTransform_Implementation() const override;

	/** 读档后若 bReached 为 true，重新触发发光并禁用触发球体。 */
	virtual void LoadActor_Implementation() override;
	// ---- End SaveInterface ----

protected:
	virtual void BeginPlay() override;

	// Sphere 重叠回调：验证进入者是玩家后触发发光（只触发一次）。
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// 创建动态材质实例并调用 CheckpointReached；同时禁掉 Sphere 碰撞防重复触发。
	void HandleGlowEffects();

	// 检查点外观 mesh（挂 SM_Checkpoint），发光参数也设在这个组件上。
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;

	// 触发区域：仅对 Pawn 做 Overlap 检测。
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
};
