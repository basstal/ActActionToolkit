﻿#pragma once

#include "ActActionSequenceStructs.generated.h"

USTRUCT()
struct FActActionHitBoxData
{
	GENERATED_BODY()
	FActActionHitBoxData();
	/**
	 * 开始帧
	 */
	UPROPERTY(EditAnywhere)
	int32 Begin;

	/**
	 * 结束帧
	 */
	UPROPERTY(EditAnywhere)
	int32 End;

	/**
	 * 攻击盒数据
	 */
	UPROPERTY(EditAnywhere)
	FOrientedBox Box;

	/**
	 * 攻击盒分组
	 */
	UPROPERTY(EditAnywhere)
	FName GroupId;
};
