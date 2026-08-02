#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveInterface.generated.h"

/**
 * 标记需要随世界状态一起存档的 Actor。
 * 实现该接口的 Actor 会被 AAuraGameModeBase::SaveWorldState 遍历并序列化
 * 其上所有带 SaveGame 说明符的成员变量。
 */
UINTERFACE(MinimalAPI)
class USaveInterface : public UInterface
{
	GENERATED_BODY()
};

/** 标记需要随世界状态存档的 Actor 的 C++ 接口类。 */
class AURA_API ISaveInterface
{
	GENERATED_BODY()

public:
	/** 加载世界状态时是否需要恢复该 Actor 的 Transform；检查点等固定 Actor 返回 false。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	bool ShouldLoadTransform() const;

	/** 加载存档数据后执行的自定义恢复逻辑（如检查点根据 bReached 重新发光）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	void LoadActor();
};
