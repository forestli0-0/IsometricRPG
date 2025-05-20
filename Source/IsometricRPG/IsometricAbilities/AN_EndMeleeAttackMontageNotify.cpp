#include "AN_EndMeleeAttackMontageNotify.h"
#include "Character/IsometricRPGCharacter.h"
#include "IsometricComponents/ActionQueueComponent.h"

void UAN_EndMeleeAttackMontageNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (MeshComp && MeshComp->GetOwner())
	{
		auto owner = MeshComp->GetOwner();
		auto IsoCharacter = Cast<AIsometricRPGCharacter>(owner);
		if (IsoCharacter)
		{
			auto  ActionQueue = IsoCharacter->FindComponentByClass<UActionQueueComponent>();
			// 攻击帧关闭
			if (ActionQueue)
			{
				ActionQueue->bAttackInProgress = false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Notify: Owner is not AIsometricRPGCharacter"));
		}
	}
}
