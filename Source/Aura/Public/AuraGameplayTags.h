// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * 
 * AuraGameplayTags
 * 单一实例类，存储游戏中所有的GameplayTag, 以便于在代码中使用，减少硬编码字符串的使用
 * 通过在构造函数中初始化所有的GameplayTag, 可以确保在游戏运行
 */

struct  FAuraGameplayTags
{
	
public:
	static const FAuraGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();
	
	//标签比较时，比的是 FGameplayTag 代表的标签值（注册后的标签名），不是 C++ 变量名。
	//~ Primary Attributes
	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Resilience;
	FGameplayTag Attributes_Primary_Vigor;
	
	//~ Secondary Attributes
	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	FGameplayTag Attributes_Secondary_BlockChance;
	FGameplayTag Attributes_Secondary_CriticalHitChance;
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	FGameplayTag Attributes_Secondary_CriticalHitResistance;
	FGameplayTag Attributes_Secondary_HealthRegeneration;
	FGameplayTag Attributes_Secondary_ManaRegeneration;
	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxMana;
	
	//~ InputAction Tags
	FGameplayTag InputTag_LMB;
	FGameplayTag InputTag_RMB;
	FGameplayTag InputTag_1;
	FGameplayTag InputTag_2;
	FGameplayTag InputTag_3;
	FGameplayTag InputTag_4;
	FGameplayTag InputTag_Passive_1;
	FGameplayTag InputTag_Passive_2;

	//~ 玩家控制屏蔽标签
	// 技能激活时通过 Activation Owned Tags 临时授予 ASC；玩家控制器统一查询它们，
	// 以便同一套机制可复用于所有需要锁定局部交互的技能。
	FGameplayTag Player_Block_CursorTrace;
	FGameplayTag Player_Block_InputPressed;
	FGameplayTag Player_Block_InputHeld;
	FGameplayTag Player_Block_InputReleased;
	
	//~ Damage Tags
	FGameplayTag Damage;
	
	//~ Damage Types 伤害 类型
	TMap<FGameplayTag,FGameplayTag> DamageTypesToResistances;// 这个数组的作用是：把所有伤害类型的标签都放在一起，方便我们在计算伤害时遍历它们来获取对应的数值。
	
	FGameplayTag Damage_Fire;
	FGameplayTag Damage_Lightning;
	FGameplayTag Damage_Arcane;
	FGameplayTag Damage_Physical;
	 
	//~ Resistance Tags 伤害 抗性
	FGameplayTag Damage_Resistance_Fire;
	FGameplayTag Damage_Resistance_Lightning;
	FGameplayTag Damage_Resistance_Arcane;
	FGameplayTag Damage_Resistance_Physical;

	//~ Debuff Tags
	TMap<FGameplayTag, FGameplayTag> DamageTypesToDebuffs;

	FGameplayTag Debuff_Burn;
	FGameplayTag Debuff_Stun;
	FGameplayTag Debuff_Arcane;
	FGameplayTag Debuff_Physical;

	//~ Debuff Parameters
	FGameplayTag Debuff_Chance;
	FGameplayTag Debuff_Damage;
	FGameplayTag Debuff_Duration;
	FGameplayTag Debuff_Frequency;
	
	//~ Set By Caller Tags
	FGameplayTag Effect_HitReact;
	
	//~ 战斗插槽 标签
	FGameplayTag CombatSocket_Weapon;
	FGameplayTag CombatSocket_LeftHand;
	FGameplayTag CombatSocket_RightHand;
	FGameplayTag CombatSocket_Trail;
	
	//~ 蒙太奇 动画 标签
	FGameplayTag Montage_Attack_1;
	FGameplayTag Montage_Attack_2;
	FGameplayTag Montage_Attack_3;
	FGameplayTag Montage_Attack_4;

	//  技能触发标签
	FGameplayTag Abilities_None;
	FGameplayTag Abilities_Attack;
	FGameplayTag Abilities_Summon;

	FGameplayTag Abilities_Fire_FireBolt;
	FGameplayTag Abilities_Lightning_Electrocute;

	//~ 被动法术标签
	FGameplayTag Abilities_Passive_HaloOfProtection;
	FGameplayTag Abilities_Passive_LifeSiphon;
	FGameplayTag Abilities_Passive_ManaSiphon;

	FGameplayTag Abilities_HitReact;

	FGameplayTag Abilities_Status_Locked;
	FGameplayTag Abilities_Status_Eligible;
	FGameplayTag Abilities_Status_Unlocked;
	FGameplayTag Abilities_Status_Equipped;

	FGameplayTag Abilities_Type_Offensive;
	FGameplayTag Abilities_Type_Passive;
	FGameplayTag Abilities_Type_None;
	
	//~ 技能冷却标签
	FGameplayTag Cooldown_Fire_FireBolt;

	//~ 经验
	FGameplayTag Attributes_Meta_IncomingXP;
	
private:
	

	static  FAuraGameplayTags GameplayTags;
};
