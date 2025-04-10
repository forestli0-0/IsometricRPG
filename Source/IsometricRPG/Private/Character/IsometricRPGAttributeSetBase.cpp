// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/IsometricRPGAttributeSetBase.h"

UIsometricRPGAttributeSetBase::UIsometricRPGAttributeSetBase()
{
    Health.SetBaseValue(100.0f);  // 设置基础生命值为 100.0
    MaxHealth.SetBaseValue(100.0f);  // 设置最大生命值为 100.0
    HealthRegenRate.SetBaseValue(1.0f);  // 设置生命恢复速率为 1.0
}