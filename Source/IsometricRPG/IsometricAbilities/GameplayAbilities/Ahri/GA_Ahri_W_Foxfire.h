#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "IsometricAbilities/GameplayAbilities/SelfCast/GA_SelfCastAbility.h"
#include "GA_Ahri_W_Foxfire.generated.h"

class AActor;
class AProjectileBase;
class UNiagaraComponent;
class UNiagaraSystem;
class UGameplayEffect;
class USoundBase;

USTRUCT(BlueprintType)
struct FFoxfireRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	float CurrentAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	float SpawnTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	float AcquireEligibleTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	float FallbackClosestTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	TWeakObjectPtr<AActor> AssignedTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	bool bLaunched = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Foxfire")
	float AssignedDamageAmount = 0.0f;
};

UCLASS(BlueprintType)
class ISOMETRICRPG_API UGA_Ahri_W_Foxfire : public UGA_SelfCastAbility
{
	GENERATED_BODY()

public:
	UGA_Ahri_W_Foxfire();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	int32 FoxfireCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float SearchRange = 725.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FoxfireSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitRadius = 135.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitHeight = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitSpeed = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float OrbitUpdateInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float BaseMagicDamage = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float DamagePerLevel = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float APRatio = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FoxfireDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float InitialAcquireDelay = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float AcquireDelayPerFoxfire = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float FallbackClosestDelay = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float RecentBasicAttackMaxAge = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float VisibilityTraceHeight = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Targeting")
	bool bAllowPlayerControlledHeroTargets = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float RepeatTargetDamageMultiplier = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float MinionExecuteHealthThreshold = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float MinionExecuteDamageMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire")
	float HomingAcceleration = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Movement")
	float MoveSpeedBonusPercent = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Movement")
	float MoveSpeedDecayDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Movement")
	float MoveSpeedDecayStepInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cooldown")
	TArray<float> CooldownByLevel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Projectile")
	TSubclassOf<AProjectileBase> SkillShotProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|GameplayEffect")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|GameplayEffect")
	TSubclassOf<UGameplayEffect> MoveSpeedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	UNiagaraSystem* FoxfireVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	UNiagaraSystem* HitVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|VFX")
	UNiagaraSystem* OrbitVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	USoundBase* CastSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foxfire|Audio")
	USoundBase* HitSound;

protected:
	virtual void ExecuteSkill(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateCancelAbility) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void ExecuteSelfCast();

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	float CalculateDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void CreateFoxfires();

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void FireNextFoxfire();

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void FireFoxfire(int32 FoxfireIndex);

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	AActor* FindFoxfireTarget() const;

	UFUNCTION()
	void OnFoxfireProjectileHit(AActor* HitActor, const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void OnFoxfireHitTarget(AActor* HitActor, int32 FoxfireIndex);

	UFUNCTION(BlueprintCallable, Category = "Foxfire")
	void OnAllFoxfiresComplete();

private:
	void TickFoxfireOrbit();
	void UpdateFoxfireVisual(FFoxfireRuntimeState& FoxfireState) const;
	FVector GetFoxfireRelativeOffset(const FFoxfireRuntimeState& FoxfireState) const;
	FVector GetFoxfireWorldLocation(const FFoxfireRuntimeState& FoxfireState) const;
	FVector GetVisibilityTraceStart() const;
	FVector GetTargetTraceLocation(AActor* TargetActor) const;

	AActor* FindFoxfireTargetForState(const FFoxfireRuntimeState& FoxfireState, bool bAllowFallbackClosest) const;
	bool TryLaunchFoxfire(int32 FoxfireIndex, AActor* ForcedTarget = nullptr);
	void LaunchAuthoritativeProjectile(const FFoxfireRuntimeState& FoxfireState, AActor* TargetActor);

	bool IsEnemyActor(AActor* Actor) const;
	bool IsVisibleTarget(AActor* TargetActor) const;
	bool IsHeroTarget(AActor* TargetActor) const;
	bool IsMinionTarget(AActor* TargetActor) const;
	bool IsAliveTarget(AActor* TargetActor) const;
	bool IsTargetInSearchRange(AActor* TargetActor) const;

	float GetCooldownDurationForLevel() const;
	float GetCurrentHealthForActor(AActor* TargetActor) const;
	float GetMaxHealthForActor(AActor* TargetActor) const;
	bool IsBelowMinionExecuteThreshold(AActor* TargetActor) const;
	float CalculateAssignedDamageForTarget(AActor* TargetActor) const;

	void ApplyMoveSpeedDecay();
	void ApplyMoveSpeedDecayStep();
	void StopMoveSpeedDecay(bool bRemoveActiveEffect);
	void ClearFoxfireTimer();
	void ClearFoxfireVisuals();
	void CleanupAbilityState(bool bWasCancelled);
	void TryCompleteAbility();
	bool ShouldSpawnPresentation() const;
	bool HasAnyPendingFoxfires() const;

private:
	UPROPERTY(Transient)
	TArray<FFoxfireRuntimeState> RuntimeFoxfires;

	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, int32> AssignedFoxfireCounts;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitActors;

	FTimerHandle OrbitTimerHandle;
	FTimerHandle MoveSpeedDecayTimerHandle;

	FActiveGameplayEffectHandle ActiveMoveSpeedHandle;

	float FoxfireStartTime = 0.0f;
	float LastOrbitUpdateTime = 0.0f;
	float MoveSpeedBuffStartTime = 0.0f;
	bool bFoxfireLifecycleComplete = false;
	bool bMoveSpeedDecayComplete = true;
	bool bAbilityCleanupPerformed = false;
};
