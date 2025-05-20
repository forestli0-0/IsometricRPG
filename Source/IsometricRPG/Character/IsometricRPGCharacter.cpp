// Fill out your copyright notice in the Description page of Project Settings.
#include "IsometricRPGCharacter.h"
#include "EnhancedInputComponent.h" // Add this include to resolve the identifier
#include "Character/IsometricRPGAttributeSetBase.h" // Add this include to resolve the identifier
#include "Components/SceneComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h" // Add this include to resolve the identifier
#include "IsometricAbilities/RPGGameplayAbility_Attack.h"
#include "IsometricAbilities/GA_Death.h"
#include "IsometricAbilities/GA_HeroMeleeAttackAbility.h"
#include "IsometricComponents/ActionQueueComponent.h"
#include "AnimationBlueprintLibrary.h"
// Sets default values
AIsometricRPGCharacter::AIsometricRPGCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 初始化技能系统组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UIsometricRPGAttributeSetBase>(TEXT("AttributeSet"));

	// 初始化动作队列组件
	ActionQueueComponent = CreateDefaultSubobject<UActionQueueComponent>(TEXT("ActionQueue"));

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
	//AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(URPGGameplayAbility_Attack::StaticClass(), 1, 0));
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_HeroMeleeAttackAbility::StaticClass(), 1, 0));
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UGA_Death::StaticClass(), 1, INDEX_NONE, this));
	ActionQueueComponent->InitializeAbilitySlots();
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
	TSubclassOf<UGameplayEffect> DefaultAttributes = StaticLoadClass(
		UGameplayEffect::StaticClass(),
		nullptr,
		TEXT("/Game/Blueprint/GameEffects/GE_InitializeAttributes.GE_InitializeAttributes_C")
	);
	// 利用GE初始化属性
	if (DefaultAttributes)
	{
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
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
            FString::Printf(TEXT("%s 的控制器是 %s"), *this->GetName(), *NewController->GetName()));
		float CurrentHealth = TempAttributeSet->GetHealth();
		float MaxHealth = TempAttributeSet->GetMaxHealth();
	}

}

