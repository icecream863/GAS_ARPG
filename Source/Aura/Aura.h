// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define CUSTOM_DEPTH_RED 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel1

/**
 * 技能目标选择专用的 Trace Channel。
 *
 * 不复用 Visibility：Visibility 同时服务于鼠标悬停、高亮等表现逻辑；Target
 * 只描述“鼠标射线是否应命中这个物体”，因此可单独让角色、阻挡体或淡出物体忽略它。
 */
#define ECC_Target ECollisionChannel::ECC_GameTraceChannel2

// 这行 #define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel1 
// 在 Source/Aura/Aura.h 里把自定义的碰撞通道 ECC_GameTraceChannel1 取了一个更有语义的名字 ECC_Projectile。
// ECC_GameTraceChannel1 就是第一个“自定义 Trace 通道”的槽位。后面依次是 ECC_GameTraceChannel2、ECC_GameTraceChannel3 等。
// 你在编辑器里给这个槽位起的名字只是显示名，代码里仍用这些枚举名引用
