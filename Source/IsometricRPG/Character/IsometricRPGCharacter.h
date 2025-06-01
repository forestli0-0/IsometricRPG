// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/ObjectMacros.h"
#include "AbilitySystemInterface.h" 
#include "AbilitySystemComponent.h" 
#include "IsometricComponents/IsometricInputComponent.h"
#include "GameplayTagContainer.h"
#include "IsometricRPGCharacter.generated.h"

// Define ESkillSlot and FEquippedAbilityInfo here or in a separate types header
UENUM(BlueprintType)
enum class ESkillSlot : uint8
{
	Skill_Q UMETA(DisplayName = "Skill Q"),
	Skill_W UMETA(DisplayName = "Skill W"),
	Skill_E UMETA(DisplayName = "Skill E"),
	Skill_R UMETA(DisplayName = "Skill R"),
	Skill_A UMETA(DisplayName = "Skill A"),
	RightClick UMETA(DisplayName = "Right Click"),
	Skill_Passive UMETA(DisplayName="Skill Passive"),
	Skill_Summoner1 UMETA(DisplayName="Skill Summoner 1"),
	Skill_Summoner2 UMETA(DisplayName="Skill Summoner 2"),
	Skill_Death UMETA(DisplayName = "Skill Death"),
	Invalid UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FEquippedAbilityInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	ESkillSlot Slot = ESkillSlot::Invalid;
	
	UPROPERTY(BlueprintReadOnly, Category="Ability") 
	FGameplayAbilitySpecHandle AbilitySpecHandle; 

	FEquippedAbilityInfo() : AbilityClass(nullptr), Slot(ESkillSlot::Invalid) {}
	FEquippedAbilityInfo(TSubclassOf<UGameplayAbility> InAbility, ESkillSlot InSlot) 
		: AbilityClass(InAbility), Slot(InSlot) {}
};

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

	// IAbilitySystemInterface implementation
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	TArray<TWeakObjectPtr<AActor>> CurrentAbilityTargets;

public:
	// 技能系统组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Character")
	UAbilitySystemComponent* AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Character")
	UAttributeSet* AttributeSet;


public:
	bool bAbilitiesInitialized = false;
	// 初始化技能系统
	virtual void PossessedBy(AController* NewController) override;
	// Server-side initialization of abilities
    virtual void InitAbilities();
	// 初始化技能 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

public:
	// 输入组件
    // Add this include to the top of the file
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	UIsometricInputComponent* IRPGInputComponent;

	// --- Skill Management ---
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	FEquippedAbilityInfo GetEquippedAbilityInfo(ESkillSlot Slot) const;

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abilities")
	void Server_EquipAbilityToSlot(TSubclassOf<UGameplayAbility> NewAbilityClass, ESkillSlot Slot);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Abilities")
	void Server_UnequipAbilityFromSlot(ESkillSlot Slot);

	// Initialize default abilities that the character starts with
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    TArray<FEquippedAbilityInfo> DefaultAbilities;

	void OnSkillOutOfRange(const FGameplayEventData* EventData);
protected:
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_EquippedAbilities, Category = "Abilities")
	TArray<FEquippedAbilityInfo> EquippedAbilities;
	
	UFUNCTION()
	void OnRep_EquippedAbilities();

	// Server-side helper functions
	void GrantAbilityInternal(FEquippedAbilityInfo& Info, bool bRemoveExistingFirst = false);
	void ClearAbilityInternal(FEquippedAbilityInfo& Info);

	// Attribute initialization
	virtual void InitializeAttributes();

	// Current Health (方便蓝图访问)
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetCurrentHealth() const;
	UFUNCTION(BlueprintPure, Category="Attributes")
	float GetMaxHealth() const;
};
