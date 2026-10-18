#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WEIconBaker.generated.h"

class AWEPickupItem;
class UPointLightComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UWEItemDefinition;

/**
 * Development tool (Phase 17.5): renders a picture of every item definition from its real world look and writes
 * Saved/ItemIcons/T_Item_<ItemId>.png (transparent background). Tools/UI/import_ui.py imports them as the
 * inventory pictures. Started with -WEBakeIcons (needs rendering, not -NullRHI); quits when done.
 */
UCLASS(NotPlaceable, Transient)
class WORKSHOPEMPIRE_API AWEIconBaker : public AActor
{
	GENERATED_BODY()

public:
	AWEIconBaker();

	static bool IsRequested();

protected:
	virtual void BeginPlay() override;

	void Next();
	void Capture();
	void SaveIcon(const FString& ItemId);

	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> ColorCapture;
	UPROPERTY() TObjectPtr<USceneCaptureComponent2D> MaskCapture;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> ColorTarget;
	UPROPERTY() TObjectPtr<UTextureRenderTarget2D> MaskTarget;
	UPROPERTY() TObjectPtr<UPointLightComponent> KeyLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> FillLight;
	UPROPERTY() TObjectPtr<AActor> Current;
	UPROPERTY() TArray<TObjectPtr<UWEItemDefinition>> Queue;
	/** -WEBakeMeshes=<asset paths>: previews of static meshes (Saved/MeshPreviews) instead of item icons. */
	UPROPERTY() TArray<TObjectPtr<class UStaticMesh>> MeshQueue;
	bool bMeshMode = false;

	int32 Index = -1;
	int32 Written = 0;
	FTimerHandle StepTimer;
};
