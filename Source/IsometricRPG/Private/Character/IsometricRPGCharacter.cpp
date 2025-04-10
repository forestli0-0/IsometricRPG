// Fill out your copyright notice in the Description page of Project Settings.
#include "Character/IsometricRPGCharacter.h"
#include "EnhancedInputComponent.h" // Add this include to resolve the identifier
#include "Character/IsometricRPGAttributeSetBase.h" // Add this include to resolve the identifier
#include "Components/SceneComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h" // Add this include to resolve the identifier
#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
// Sets default values
AIsometricRPGCharacter::AIsometricRPGCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 初始化技能系统组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>(TEXT("AttributeSet"));

	// 创建输入组件
	IRPGInputComponent = CreateDefaultSubobject<UIsometricInputComponent>(TEXT("IRPGInputComponent"));
	// 不要让角色面朝摄像机方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
}

// Called when the game starts or when spawned

void AIsometricRPGCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 给角色添加默认技能
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(URPGGameplayAbility_Attack::StaticClass(), 1, 0));

}

// Called every frame
void AIsometricRPGCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AIsometricRPGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		IRPGInputComponent->SetupInput(EIC, Cast<APlayerController>(GetController()));
	}
}

UAbilitySystemComponent* AIsometricRPGCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AIsometricRPGCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	// 利用GE初始化属性
	if (DefaultAttributes)
	{
		// 打印默认属性的类名
		UE_LOG(LogTemp, Warning, TEXT("DefaultAttributes class name: %s"), *DefaultAttributes->GetName());
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, 1.f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}


    if (AttributeSet)  
    {  
		UIsometricRPGAttributeSetBase* TempAttributeSet = Cast<UIsometricRPGAttributeSetBase>(AttributeSet);
        // 检查参数问题的代码片段
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("%s 的控制器是 %s"), *this->GetName(), *NewController->GetName()));
		float CurrentHealth = TempAttributeSet->GetHealth();
		float MaxHealth = TempAttributeSet->GetMaxHealth();

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
			FString::Printf(TEXT("Health: %f / %f"), CurrentHealth, MaxHealth));

        
	}
}
