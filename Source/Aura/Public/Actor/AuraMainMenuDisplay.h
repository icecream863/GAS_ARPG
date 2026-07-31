// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraMainMenuDisplay.generated.h"

class UDecalComponent;
class UAudioComponent;
class UNiagaraComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USoundBase;
class UAuraMainMenuWidget;
class UWidgetComponent;

/**
 * 主菜单中的 Aura 展示 Actor。
 * 蓝图子类只负责配置美术资产和相对位置，循环动画统一由 C++ 驱动。
 */
UCLASS()
class AURA_API AAuraMainMenuDisplay : public AActor
{
	GENERATED_BODY()

public:
	AAuraMainMenuDisplay();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<USkeletalMeshComponent> AuraMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<USkeletalMeshComponent> StaffMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<UNiagaraComponent> FireballComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<UDecalComponent> MagicCircleDecal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<UWidgetComponent> TitleWidgetComponent;

	/** 主菜单循环音乐。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	TObjectPtr<USoundBase> MainMenuMusic;

	/** 覆盖该类即可更换主菜单界面外观。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	TSubclassOf<UAuraMainMenuWidget> MainMenuWidgetClass;

	/** 火球围绕初始位置上下浮动的最大距离，单位为厘米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu|Animation", meta = (ClampMin = "0.0"))
	float FireballAmplitude = 5.f;

	/** 魔法阵每秒绕自身 Roll 轴旋转的角度。负值表示反向旋转。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu|Animation")
	float MagicCircleRotationRate = -2.f;

private:
	/** 播放不受世界位置影响的主菜单音乐。 */
	void StartMainMenuMusic();

	/** 创建菜单、切换为 UI Only 输入并显示鼠标。 */
	void CreateMainMenuWidget();

	/** 累计并限制正弦函数使用的弧度时间，避免运行时间无限增长。 */
	void UpdateAnimationTime(float DeltaSeconds);

	/** 根据当前弧度时间更新火球相对初始位置的 Z 偏移。 */
	void UpdateFireballLocation() const;

	/** 按每秒旋转速率更新魔法阵的局部 Roll。 */
	void UpdateMagicCircleRotation(float DeltaSeconds) const;

	FVector FireballInitialRelativeLocation = FVector::ZeroVector;
	float AnimationTime = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MainMenuMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAuraMainMenuWidget> MainMenuWidget;
};
