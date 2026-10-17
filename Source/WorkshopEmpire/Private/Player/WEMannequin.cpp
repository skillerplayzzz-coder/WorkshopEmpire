#include "Player/WEMannequin.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace WEMannequin
{
	void Configure(USkeletalMeshComponent* Mesh, float CapsuleHalfHeight)
	{
		if (!Mesh)
		{
			return;
		}
		// The mannequin's feet are at its origin and it faces +Y.
		Mesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -CapsuleHalfHeight), FRotator(0.f, -90.f, 0.f));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Mesh->bEnableUpdateRateOptimizations = true;
	}

	void ApplyBody(USkeletalMeshComponent* Mesh, bool bQuinn)
	{
		if (!Mesh)
		{
			return;
		}
		USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, bQuinn ? QuinnPath : MannyPath);
		UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, *FString::Printf(TEXT("%s.ABP_Unarmed_C"), AnimBlueprintPath));
		if (Body && Mesh->GetSkeletalMeshAsset() != Body)
		{
			Mesh->SetSkeletalMesh(Body);
		}
		if (AnimClass && Mesh->GetAnimClass() != AnimClass)
		{
			Mesh->SetAnimInstanceClass(AnimClass);
		}
	}

	void Tint(USkeletalMeshComponent* Mesh, const FLinearColor& Clothes)
	{
		if (!Mesh || !Mesh->GetSkeletalMeshAsset())
		{
			return;
		}
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(i);
			if (MID)
			{
				// Slot 1 is the torso (work clothes); head and legs are darker.
				MID->SetVectorParameterValue(TEXT("Paint Tint"), i == 1 ? Clothes : Clothes * 0.35f + FLinearColor(0.08f, 0.08f, 0.09f));
			}
		}
	}

	FLinearColor PlayerColor(int32 Index)
	{
		static const FLinearColor Colors[] = {
			FLinearColor(0.85f, 0.38f, 0.08f), FLinearColor(0.12f, 0.36f, 0.85f), FLinearColor(0.2f, 0.62f, 0.25f), FLinearColor(0.5f, 0.25f, 0.75f)
		};
		return Colors[FMath::Abs(Index) % 4];
	}
}
