#include "Loot/LootPickup.h"

#include "Character/IsoPlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemData.h"
#include "Net/UnrealNetwork.h"

ALootPickup::ALootPickup()
{
    bReplicates = true;

    PickupTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));
    RootComponent = PickupTrigger;
    PickupTrigger->InitSphereRadius(80.f);
    PickupTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PickupTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALootPickup::HandleOverlap);
}

void ALootPickup::BeginPlay()
{
    Super::BeginPlay();
    DestroyIfDepleted();
}

void ALootPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALootPickup, LootStack);
}

void ALootPickup::InitializeLoot(const FInventoryItemStack& Stack)
{
    LootStack = Stack;
    DestroyIfDepleted();
}

void ALootPickup::OnRep_LootStack(const FInventoryItemStack& OldValue)
{
    DestroyIfDepleted();
}

void ALootPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || !OtherActor)
    {
        return;
    }

    APawn* OverlappingPawn = Cast<APawn>(OtherActor);
    if (!OverlappingPawn)
    {
        return;
    }

    AIsoPlayerState* PlayerState = OverlappingPawn->GetPlayerState<AIsoPlayerState>();
    if (!PlayerState)
    {
        return;
    }

    if (UInventoryComponent* Inventory = PlayerState->GetInventoryComponent())
    {
        AttemptPickup(Inventory);
    }
}

void ALootPickup::AttemptPickup(UInventoryComponent* InventoryComponent)
{
    if (!InventoryComponent || !LootStack.IsValid())
    {
        return;
    }

    UInventoryItemData* ItemData = LootStack.Resolve();
    if (!ItemData)
    {
        Destroy();
        return;
    }

    const int32 Remaining = InventoryComponent->AddItem(ItemData, LootStack.Quantity);
    if (Remaining <= 0)
    {
        LootStack.Quantity = 0;
    }
    else
    {
        LootStack.Quantity = Remaining;
    }

    DestroyIfDepleted();
}

void ALootPickup::DestroyIfDepleted()
{
    if (!HasAuthority())
    {
        return;
    }

    if (!LootStack.IsValid() || LootStack.Quantity <= 0)
    {
        Destroy();
    }
}
