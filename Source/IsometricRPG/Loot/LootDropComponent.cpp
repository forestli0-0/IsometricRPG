#include "Loot/LootDropComponent.h"

#include "AbilitySystemInterface.h"
#include "Character/IsometricRPGAttributeSetBase.h"
#include "Loot/LootPickup.h"
#include "Loot/LootTableDataAsset.h"
#include "Net/UnrealNetwork.h"

ULootDropComponent::ULootDropComponent()
{
    SetIsReplicatedByDefault(true);
}

void ULootDropComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwnerRole() == ROLE_Authority)
    {
        BindToAttributeSet();
    }
}

void ULootDropComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindFromAttributeSet();
    Super::EndPlay(EndPlayReason);
}

void ULootDropComponent::BindToAttributeSet()
{
    if (CachedAttributeSet.IsValid())
    {
        return;
    }

    if (AActor* OwnerActor = GetOwner())
    {
        if (IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(OwnerActor))
        {
            if (UAbilitySystemComponent* ASC = AbilityInterface->GetAbilitySystemComponent())
            {
                const UIsometricRPGAttributeSetBase* AttributeSetConst = ASC->GetSet<UIsometricRPGAttributeSetBase>();
                UIsometricRPGAttributeSetBase* AttributeSet = const_cast<UIsometricRPGAttributeSetBase*>(AttributeSetConst);
                CachedAttributeSet = Cast<UIsometricRPGAttributeSetBase>(AttributeSet);
            }
        }
        else
        {
			UE_LOG(LogTemp, Warning, TEXT("[LootDropComponent] Owner does not implement AbilitySystemInterface: %s"), *OwnerActor->GetName());
        }
    }

    if (UIsometricRPGAttributeSetBase* AttributeSet = CachedAttributeSet.Get())
    {
        AttributeSet->OnHealthChanged.AddDynamic(this, &ULootDropComponent::HandleHealthChanged);
    }
}

void ULootDropComponent::UnbindFromAttributeSet()
{
    if (UIsometricRPGAttributeSetBase* AttributeSet = CachedAttributeSet.Get())
    {
        AttributeSet->OnHealthChanged.RemoveDynamic(this, &ULootDropComponent::HandleHealthChanged);
    }

    CachedAttributeSet = nullptr;
}

void ULootDropComponent::HandleHealthChanged(UIsometricRPGAttributeSetBase* AttributeSet, float NewHealth)
{
    if (bLootDropped || !AttributeSet || NewHealth > 0.f)
    {
        return;
    }

    DropLoot();
}

void ULootDropComponent::DropLoot()
{
    if (bLootDropped || GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    bLootDropped = true;

    if (!LootTable)
    {
        if (bDestroyOwnerAfterDrop && GetOwner())
        {
            GetOwner()->Destroy();
        }
        return;
    }

    TArray<FInventoryItemStack> GeneratedLoot;
    FRandomStream RandomStream(FMath::Rand());
    LootTable->GenerateLoot(GeneratedLoot, RandomStream);

    for (const FInventoryItemStack& Stack : GeneratedLoot)
    {
        SpawnPickup(Stack);
    }

    if (bDestroyOwnerAfterDrop && GetOwner())
    {
        GetOwner()->Destroy();
    }
}

void ULootDropComponent::SpawnPickup(const FInventoryItemStack& Stack) const
{
    if (!Stack.IsValid() || !GetWorld())
    {
        return;
    }

    TSubclassOf<ALootPickup> PickupToSpawn = LootPickupClass;
    if (!PickupToSpawn)
    {
        PickupToSpawn = ALootPickup::StaticClass();
    }

    const FVector SpawnLocation = GetRandomScatterLocation();
    const FTransform SpawnTransform(SpawnLocation);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();

    if (ALootPickup* Pickup = GetWorld()->SpawnActorDeferred<ALootPickup>(PickupToSpawn, SpawnTransform, GetOwner()))
    {
        Pickup->InitializeLoot(Stack);
        Pickup->FinishSpawning(SpawnTransform);
    }
}

FVector ULootDropComponent::GetRandomScatterLocation() const
{
    if (!GetOwner())
    {
        return FVector::ZeroVector;
    }

    FVector Origin = GetOwner()->GetActorLocation();
    if (ScatterRadius <= KINDA_SMALL_NUMBER)
    {
        return Origin;
    }

    const FVector2D RandomCircle = FMath::RandPointInCircle(ScatterRadius);
    return Origin + FVector(RandomCircle.X, RandomCircle.Y, 0.f);
}
