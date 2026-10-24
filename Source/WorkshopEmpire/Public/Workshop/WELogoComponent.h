#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WELogoComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Phase 20: the company logo on something (van, wall above the door, company sign, an overall): a square plate with
 * the symbol in the brand colours and, if wanted, the name below it. Faces its local +X, centred on itself. Follows
 * the brand (UWEBrandComponent) on every machine; visual only.
 */
UCLASS(ClassGroup = (WorkshopEmpire), meta = (BlueprintSpawnableComponent))
class WORKSHOPEMPIRE_API UWELogoComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWELogoComponent();

	/** Edge length of the symbol plate in cm. */
	UPROPERTY(EditAnywhere, Category = "Logo")
	float Size = 60.f;

	/** Name under the symbol (not on clothes). */
	UPROPERTY(EditAnywhere, Category = "Logo")
	bool bShowName = true;

	/** Height of the name letters in cm (0 = a third of the plate). */
	UPROPERTY(EditAnywhere, Category = "Logo")
	float NameHeight = 0.f;

	/** Quality overhaul: the name stands on a light board (the company sign): the darker brand colour, never a light one. */
	UPROPERTY(EditAnywhere, Category = "Logo")
	bool bOnLightBoard = false;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** First person: the wearer does not see his own chest logo. */
	void SetLogoOwnerNoSee(bool bNoSee);
	/** Re-reads the brand now. */
	void Refresh();
	/** Tests: what is shown. */
	FString GetShownName() const;
	FName GetShownSymbol() const { return ShownSymbol; }

protected:
	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Plate;
	UPROPERTY(Transient) TObjectPtr<UTextRenderComponent> NameText;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PlateMaterial;
	uint32 ShownHash = 0;
	FName ShownSymbol;
	float CheckTimer = 0.f;
};
