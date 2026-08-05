#include "Actor/EnemySpawnPoint.h"

#include "Character/AuraEnemy.h"

void AAuraEnemySpawnPoint::SpawnEnemy()
{
	// 【优化】未配置敌人种类时直接返回，避免 SpawnActorDeferred 传入空类崩溃。
	if (!EnemyClass)
	{
		return;
	}

	// 【优化】UE 5.8 的 SpawnActorDeferred 已不再接收 FActorSpawnParameters，
	// 改为把碰撞处理方式作为独立参数传入（课程中的旧 API 已移除）。
	AAuraEnemy* Enemy = GetWorld()->SpawnActorDeferred<AAuraEnemy>(
		EnemyClass,
		GetActorTransform(),
		nullptr,  // Owner
		nullptr,  // Instigator
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy)
	{
		return;
	}

	// Deferred 生成：先设置等级与职业，再 FinishSpawning，属性初始化会用上它们。
	Enemy->SetLevel(EnemyLevel);
	Enemy->SetCharacterClass(CharacterClass);
	Enemy->FinishSpawning(GetActorTransform());

	// AI 角色默认不会自带控制器，必须手动生成默认控制器才能运行行为树。
	Enemy->SpawnDefaultController();
}
