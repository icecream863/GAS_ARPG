#pragma once

#include "CoreMinimal.h"
#include "Aura/Aura.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/HighlightInterface.h"
#include "Interaction/SaveInterface.h"
#include "Checkpoint.generated.h"

class USphereComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 检查点：继承 PlayerStart，带 PlayerStartTag 可被 ChoosePlayerStart 选中作为出生点。
 * 玩家进入 Sphere 触发发光；未来在此保存 PlayerStartTag 作为存档检查点。
 */
UCLASS()
class AURA_API ACheckpoint : public APlayerStart, public ISaveInterface, public IHighlightInterface
{
	GENERATED_BODY()

public:
	ACheckpoint(const FObjectInitializer& ObjectInitializer);

	/** 高亮描边的自定义深度模板值；可在蓝图 Class Defaults 里按需覆写（默认棕褐色）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Checkpoint")
	int32 CustomDepthStencilOverride = CUSTOM_DEPTH_TAN;

	/** 是否绑定 C++ 的重叠回调（存档/发光）。信标等子类可在蓝图里关闭，改用蓝图的 Overlap 事件。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Checkpoint")
	bool bBindOverlapCallback = true;

	/** 【优化】到达后是否禁用触发球体。检查点默认保持可反复触发存档；信标子类设为 true 只亮一次。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Checkpoint")
	bool bDisableSphereOnReach = false;

	/** 该检查点是否已被玩家触发过；SaveGame 说明符使其能被 SaveWorldState 序列化。 */
	UPROPERTY(SaveGame, BlueprintReadWrite, EditAnywhere, Category = "Checkpoint")
	bool bReached = false;

	/** 创建动态材质实例并播放发光表现；子类/蓝图可在重叠事件里主动调用。 */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void HandleGlowEffects();

	// 玩家到达检查点时在蓝图侧实现（发光时间线等视觉反馈）。
	UFUNCTION(BlueprintImplementableEvent)
	void CheckpointReached(UMaterialInstanceDynamic* DynamicMaterialInstance);

	// ---- SaveInterface ----
	/** 检查点位置固定，读档时不恢复 Transform。 */
	virtual bool ShouldLoadTransform_Implementation() const override;

	/** 读档后若 bReached 为 true，重新触发发光并禁用触发球体。 */
	virtual void LoadActor_Implementation() override;
	// ---- End SaveInterface ----

	// ---- HighlightInterface ----
	/** 点击检查点时把自动奔跑目的地覆盖为 MoveToComponent 的位置。 */
	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;

	/** 悬停检查点：开启 RenderCustomDepth（用 CustomDepthStencilOverride 的模板值着色）。 */
	virtual void HighLightActor_Implementation() override;

	/** 移开光标：关闭 RenderCustomDepth。 */
	virtual void UnHighLightActor_Implementation() override;
	// ---- End HighlightInterface ----

protected:
	virtual void BeginPlay() override;

	// Sphere 重叠回调：验证进入者是玩家后触发发光并保存；可反复触发存档。
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 检查点外观 mesh（挂 SM_Checkpoint），发光参数也设在这个组件上。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;

	// 触发区域：仅对 Pawn 做 Overlap 检测。
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;

	// 点击检查点时玩家自动奔跑的目标位置（在 BP 视口里摆放）。
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> MoveToComponent;

private:
	// 缓存初始材质与动态实例：反复触发时复用同一个 MID，避免动态实例套娃。
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> CheckpointBaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CheckpointDynamicMaterial;
};
