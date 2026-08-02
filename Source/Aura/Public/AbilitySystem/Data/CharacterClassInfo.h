// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"

#include "Engine/DataAsset.h"
#include "CharacterClassInfo.generated.h"

/**
 * 角色职业信息数据资产
 * 包含角色职业的相关信息，如技能列表、属性加成等
 * 通过创建不同的CharacterClassInfo数据资产，可以定义不同职业的特点和能力
 * 例如，元素使职业可能拥有强大的魔法攻击技能，而战士职业可能拥有高防御和近战攻击技能，游侠职业可能拥有远程攻击和敏捷属性加成
 * 这些数据资产可以在游戏中被角色类或其他系统引用，以实现职业相关的功能和效果
 * 通过使用数据资产，可以方便地管理和调整不同职业的属性和技能，而无需修改代码逻辑，提高了游戏的可
 * 维护性和扩展性
 */


/** 
 * 配置好DataAsset后，角色类可以通过引用这个DataAsset来获取职业相关的信息，例如在角色初始化时根据职业类型加载对应的属性加成和技能列表。
 * 这样就实现了职业信息的集中管理和灵活调整，方便后续添加新的职业或修改现有职业的特点，而不需要修改角色类的代码逻辑
 * UI界面也可以通过绑定这个DataAsset来显示职业相关的信息，
 * 例如在角色选择界面显示不同职业的属性加成和技能描述，或者在角色状态界面显示当前职业的特点和能力
 */
class UGameplayEffect;
class UGameplayAbility;

UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	Elementalist,//元素使
	Warrior,//战士
	Ranger//游侠（射手）
};

USTRUCT(BlueprintType)
struct FCharacterClassDefaultInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class DefaultInfo")
	TSubclassOf<UGameplayEffect> PrimaryAttribute;//GE初始化主属性，这个结构体在内部，由下面的 TMap 暴露
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class DefaultInfo")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class DefaultInfo")
	FScalableFloat XPReward = FScalableFloat();//血量系数
};


UCLASS()
class AURA_API UCharacterClassInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Class DefaultInfo")
	TMap<ECharacterClass, FCharacterClassDefaultInfo> CharacterClassInformation;//角色职业信息映射表
	
	// 获得角色默认属性， 不是公共属性
	FCharacterClassDefaultInfo GetCharacterClassDefaultInfo(ECharacterClass CharacterClass);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TSubclassOf<UGameplayEffect> SecondaryAttribute;

	// 玩家读档后需要持续监听主属性变化并重新计算次级属性。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TSubclassOf<UGameplayEffect> SecondaryAttributesInfinite;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TSubclassOf<UGameplayEffect> PrimaryAttributesSetByCaller;
	
	/** TSubclassOf<UGameplayAbility> = 技能类/蓝图（用来做配置、做模板，知道要生成什么）。
	UGameplayAbility* = 技能实例（真正在游戏里释放和计算冷却的具体对象，通常挂在角色的技能系统组件 ASC 里）。*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TArray<TSubclassOf<UGameplayAbility>> CommonAbilities;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TSubclassOf<UGameplayEffect> VitalAttribute;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Common Class DefaultInfo")
	TObjectPtr<UCurveTable> DamageCalculationCoefficients;
};
