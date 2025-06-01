// filepath: f:\Unreal Projects\IsometricRPG\Source\IsometricRPG\IsometricAbilities\AN_EndMeleeAttackMontageNotify.cpp
#include "AN_EndMeleeAttackMontageNotify.h"
#include "Character/IsometricRPGCharacter.h"

void UAN_EndMeleeAttackMontageNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		auto owner = MeshComp->GetOwner();
		auto IsoCharacter = Cast<AIsometricRPGCharacter>(owner);
		if (IsoCharacter)
		{

		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Notify: Owner is not AIsometricRPGCharacter"));
		}
	}
}
