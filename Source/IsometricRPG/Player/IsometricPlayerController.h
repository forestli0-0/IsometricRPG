#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "IsometricRPG/IsometricAbilities/Types/HeroAbilityTypes.h"
#include "IsometricPlayerController.generated.h"


class UInputMappingContext;
class UInputAction;
class IsometricInputComponent;
UCLASS()
class ISOMETRICRPG_API AIsometricPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AIsometricPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* ClickMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_LeftClick;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_RightClick;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_A;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Q;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_W;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_E;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_R;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Summoner1;
	UPROPERTY(EditDefaultsOnly, Category = "Input|Actions")
	UInputAction* Action_Summoner2;

	// 处理鼠标左键点击: 选择目标，可以是敌人也可以是友方
	UFUNCTION()
	void HandleLeftClickInput(const FInputActionValue& Value);
	// 处理鼠标右键点击: 移动或者攻击目标
	UFUNCTION()
	void HandleRightClickInput(const FInputActionValue& Value);

protected:
	// 处理技能输入
	void HandleSkillInput(EAbilityInputID InputID);

};
