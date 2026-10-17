#include "World/WESurface.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "World/WEPlaceholderVisuals.h"

namespace WESurface
{
	FString GetInstancePath(EWESurface Kind)
	{
		const UEnum* Enum = StaticEnum<EWESurface>();
		const FString Name = Enum ? Enum->GetNameStringByValue(static_cast<int64>(Kind)) : FString();
		return FString::Printf(TEXT("/Game/WorkshopEmpire/Materials/Surfaces/MI_WE%s.MI_WE%s"), *Name, *Name);
	}

	void Apply(UMeshComponent* Mesh, EWESurface Kind, const FLinearColor& Tint, float Grime)
	{
		if (!Mesh)
		{
			return;
		}
		UMaterialInterface* Surface = nullptr;
		if (Kind != EWESurface::Placeholder)
		{
			// Quiet load: a missing instance (textures not imported) simply means placeholder look.
			Surface = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *GetInstancePath(Kind), nullptr, LOAD_Quiet | LOAD_NoWarn));
		}
		if (!Surface)
		{
			WEPlaceholder::ApplyTint(Mesh, LoadObject<UMaterialInterface>(nullptr, WEPlaceholder::MaterialPath), Tint);
			return;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Surface, Mesh);
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		if (Grime >= 0.f)
		{
			MID->SetScalarParameterValue(TEXT("Grime"), Grime);
		}
		for (int32 i = 0; i < FMath::Max(1, Mesh->GetNumMaterials()); ++i)
		{
			Mesh->SetMaterial(i, MID);
		}
	}
}
