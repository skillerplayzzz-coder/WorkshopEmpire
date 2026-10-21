#include "Items/WEWearVisuals.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace WEWear
{
	void ApplyAmounts(UMeshComponent* Mesh, const FWEItemWear& Wear, float Seed, TObjectPtr<UMaterialInstanceDynamic>& Cache)
	{
		if (!Mesh || !Mesh->GetWorld() || Mesh->GetWorld()->GetNetMode() == NM_DedicatedServer)
		{
			return;
		}
		if (!Wear.IsVisible())
		{
			if (Mesh->GetOverlayMaterial() == Cache.Get())
			{
				Mesh->SetOverlayMaterial(nullptr);
			}
			return;
		}
		if (!Cache)
		{
			static TWeakObjectPtr<UMaterialInterface> Base;
			if (!Base.IsValid())
			{
				Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/M_WEWear.M_WEWear"), nullptr, LOAD_Quiet | LOAD_NoWarn);
			}
			if (!Base.IsValid())
			{
				return;
			}
			Cache = UMaterialInstanceDynamic::Create(Base.Get(), Mesh);
		}
		Cache->SetScalarParameterValue(TEXT("Dirt"), Wear.Dirt);
		Cache->SetScalarParameterValue(TEXT("Scratches"), Wear.Scratches);
		Cache->SetScalarParameterValue(TEXT("Yellowing"), Wear.Yellowing);
		Cache->SetScalarParameterValue(TEXT("Damage"), Wear.Damage);
		Cache->SetScalarParameterValue(TEXT("Seed"), Seed);
		Mesh->SetOverlayMaterial(Cache);
	}

	void Apply(UMeshComponent* Mesh, const FWEItemInstance& Item, TObjectPtr<UMaterialInstanceDynamic>& Cache)
	{
		ApplyAmounts(Mesh, Item.GetWear(), (GetTypeHash(Item.InstanceId) % 997) / 31.f, Cache);
	}
}