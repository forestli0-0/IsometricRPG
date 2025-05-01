// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/ObjectMacros.h"
#include "AbilitySystemInterface.h" 
#include "AbilitySystemComponent.h" 
#include <IsometricComponents/IsometricInputComponent.h>
#include "IsometricRPGCharacter.generated.h"


UCLASS()
class ISOMETRICRPG_API AIsometricRPGCharacter : public ACharacter, public IAbilitySystemInterface
{

	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AIsometricRPGCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// IAbilitySystemInterface implementation
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	// 技能系统组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Character")
	UAbilitySystemComponent* AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Character")
	UAttributeSet* AttributeSet;

public:
	// 初始化技能系统
	virtual void PossessedBy(AController* NewController) override;

public:
	// 输入组件
    // Add this include to the top of the file
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	UIsometricInputComponent* IRPGInputComponent;

};
