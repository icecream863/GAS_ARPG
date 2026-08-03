#pragma once

#include "CoreMinimal.h"
#include "Checkpoint/Checkpoint.h"
#include "MapEntrance.generated.h"

/**
 * 地图出入口：继承检查点，保留高亮与“点击跑到指定位置”的行为，
 * 但重叠时改为：用目的地出生点标签保存进度 → 保存世界状态 → 切换到目标地图。
 */
UCLASS()
class AURA_API AMapEntrance : public ACheckpoint
{
	GENERATED_BODY()

public:
	AMapEntrance(const FObjectInitializer& ObjectInitializer);

	/** 传送目标地图资产；在关卡实例上设置。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Entrance")
	TSoftObjectPtr<UWorld> DestinationMap;

	/** 到达目标地图后出生的 PlayerStart 标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Entrance")
	FName DestinationPlayerStartTag;

	// ---- HighlightInterface ----
	/** 地图出入口始终可高亮（不检查 bReached）。 */
	virtual void HighLightActor_Implementation() override;
	// ---- End HighlightInterface ----
	
	// ---- SaveInterface ----
	/** 读档时不恢复发光表现（地图入口不需要“已点亮”状态）。 */
	virtual void LoadActor_Implementation() override;
	// ---- End SaveInterface ----

protected:
	/** 覆盖检查点的重叠逻辑：保存到目的地出生点，保存世界状态后切换地图。 */
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                             const FHitResult& SweepResult) override;


};
