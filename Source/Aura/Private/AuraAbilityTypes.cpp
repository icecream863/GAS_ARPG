#include "AuraAbilityTypes.h"


bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	/**
	 * NetSerialize = “网络序列化”。同一个函数同时负责：
	 * - Ar.IsSaving(): 把数据写进网络包（Server -> Client）
	 * - Ar.IsLoading(): 从网络包读出来还原数据
	 *
	 * 这里的核心思路是：
	 * 1) 先用 RepBits(位掩码) 写出「哪些字段存在/需要同步」。
	 *    这样可以避免把无效指针/空数组/不存在的 HitResult 等内容也发出去，节省带宽。
	 * 2) 再根据 RepBits 的每一位，按固定顺序序列化对应字段。
	 *    读写两端必须保持完全一致的顺序，否则会“读串包”。
	 */
	uint32 RepBits = 0;
	// ------------------------------------------------------------
	// 1) Saving：计算位掩码（哪些字段需要写出）
	// ------------------------------------------------------------
	if (Ar.IsSaving()) // 当前在写出去（把数据写进网络包）
	{
		/**RepBits 第 N 位为 1 代表“该字段存在/需要同步”。
		// 注意：这些字段大部分来自父类 FGameplayEffectContext，我们只是把它的逻辑
		// 复制到了这里，并额外增加 bit7/bit8 来同步我们的自定义字段。
		//
		// Bit 映射表：
		//   bit0: Instigator
		//   bit1: EffectCauser
		//   bit2: AbilityCDO
		//   bit3: SourceObject
		//   bit4: Actors 数组
		//   bit5: HitResult
		//   bit6: WorldOrigin
		//   bit7: bIsBlockedHit   (自定义)
		//   bit8: bIsCriticalHit  (自定义)
		//   bit9: bIsSuccessfulDebuff
		//   bit10-12: DebuffDamage / Duration / Frequency
		//   bit13: DamageType
		//   bit14: DeathImpulse
		//   bit15: KnockbackForce
		*/
		if (bReplicateInstigator && Instigator.IsValid())
		{
			RepBits |= 1 << 0;
		}
		if (bReplicateEffectCauser && EffectCauser.IsValid() )
		{
			RepBits |= 1 << 1;
		}
		if (AbilityCDO.IsValid())
		{
			RepBits |= 1 << 2;
		}
		if (bReplicateSourceObject && SourceObject.IsValid())
		{
			RepBits |= 1 << 3;
		}
		if (Actors.Num() > 0)
		{
			RepBits |= 1 << 4;
		}
		if (HitResult.IsValid())
		{
			RepBits |= 1 << 5;
		}
		if (bHasWorldOrigin)
		{
			RepBits |= 1 << 6;
		}
		if (bIsBlockedHit)
		{	
			RepBits |= 1 << 7;
		}
		if (bIsCriticalHit)
		{
			RepBits |= 1 << 8;
		}
		if (bIsSuccessfulDebuff)
		{
			RepBits |= 1 << 9;
		}
		if (DebuffDamage > 0.f)
		{
			RepBits |= 1 << 10;
		}
		if (DebuffDuration > 0.f)
		{
			RepBits |= 1 << 11;
		}
		if (DebuffFrequency > 0.f)
		{
			RepBits |= 1 << 12;
		}
		if (DamageType.IsValid())
		{
			RepBits |= 1 << 13;
		}
		if (!DeathImpulse.IsZero())
		{
			RepBits |= 1 << 14;
		}
		if (!KnockbackForce.IsZero())
		{
			RepBits |= 1 << 15;
		}
	}

	// ------------------------------------------------------------
	// 2) 先序列化位掩码本身。最高使用 bit15，因此总共需要 16 位。
	//    Loading 时会先把 RepBits 读回来，后续就知道要读哪些字段。
	// ------------------------------------------------------------
	Ar.SerializeBits(&RepBits, 16);
	
	// ------------------------------------------------------------
	// 3) 根据 RepBits 序列化/反序列化各字段（顺序必须固定），可以知道 Ar.IsSaving() Ar.IsLoading(): 
	// ------------------------------------------------------------
	if (RepBits & (1 << 0))
	{
		// Instigator / EffectCauser 等都是弱指针（TWeakObjectPtr），
		// 通过 UPackageMap 走网络对象映射（NetGUID）来同步引用。
		Ar << Instigator;
	}
	if (RepBits & (1 << 1))
	{
		Ar << EffectCauser;
	}
	if (RepBits & (1 << 2))
	{
		Ar << AbilityCDO;
	}
	if (RepBits & (1 << 3))
	{
		Ar << SourceObject;
	}
	if (RepBits & (1 << 4))
	{
		// 安全序列化数组；模板参数 31 表示数组长度/序列化位数方面的约束（防止爆包）。
		SafeNetSerializeTArray_Default<31>(Ar, Actors);
	}
	if (RepBits & (1 << 5))
	{
		// HitResult 是 TSharedPtr<FHitResult>。
		// Loading 时要先确保 SharedPtr 已经指向有效内存，再 NetSerialize 读进去。
		if (Ar.IsLoading())
		{
			if (!HitResult.IsValid())
			{
				HitResult = TSharedPtr<FHitResult>(new FHitResult());
			}
		}
		HitResult->NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 6))
	{
		Ar << WorldOrigin;
		bHasWorldOrigin = true;
	}
	else
	{
		bHasWorldOrigin = false;
	}
	if (RepBits & (1 << 7))
	{
		// 这里 Ar<<bool 实际会把 bool 再写一次。
		// 严格来说：因为 bit7 已经代表“bIsBlockedHit=true”，所以也可以直接在 Loading 时
		// 令 bIsBlockedHit = true 而不额外序列化 bool。
		// 目前这样写功能上没问题，只是略微冗余。
		Ar << bIsBlockedHit;
	}
	if (RepBits & (1 << 8))
	{
		// 同上：bit8 已经表示“bIsCriticalHit=true”。
		Ar << bIsCriticalHit;
	}
	if (RepBits & (1 << 9))
	{
		Ar << bIsSuccessfulDebuff;
	}
	if (RepBits & (1 << 10))
	{
		Ar << DebuffDamage;
	}
	if (RepBits & (1 << 11))
	{
		Ar << DebuffDuration;
	}
	if (RepBits & (1 << 12))
	{
		Ar << DebuffFrequency;
	}
	if (RepBits & (1 << 13))
	{
		if (Ar.IsLoading() && !DamageType.IsValid())
		{
			DamageType = MakeShared<FGameplayTag>();
		}
		DamageType->NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 14))
	{
		DeathImpulse.NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 15))
	{
		KnockbackForce.NetSerialize(Ar, Map, bOutSuccess);
	}

	if (Ar.IsLoading())
	{
		// Loading 完成后补一次 AddInstigator：
		// 主要用于初始化 InstigatorAbilitySystemComponent 等内部缓存。
		AddInstigator(Instigator.Get(), EffectCauser.Get()); // Just to initialize InstigatorAbilitySystemComponent
	}	
	
	// bOutSuccess 用于告诉上层“网络序列化是否成功”。这里只要没失败就置 true。
	bOutSuccess = true;
	return true;
	
}
