#include "World/WEPlaceholderVisuals.h"

#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void WEPlaceholder::ApplyTint(UMeshComponent* Mesh, UMaterialInterface* BaseMaterial, const FLinearColor& Color)
{
	if (!Mesh)
	{
		return;
	}
	UMaterialInterface* Parent = BaseMaterial ? BaseMaterial : Mesh->GetMaterial(0);
	if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Parent))
	{
		Parent = Existing->Parent; // never stack instances on repeated construction
	}
	if (!Parent)
	{
		return;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Mesh);
	MID->SetFlags(RF_Transient);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	Mesh->SetMaterial(0, MID);
}
