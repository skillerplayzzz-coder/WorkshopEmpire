#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Items/WEItemTypes.h"
#include "WEAssemblyDefinition.generated.h"

class UStaticMesh;

/** How a part is held in place. Decides the hands-on action, its animation and its sound. */
UENUM(BlueprintType)
enum class EWEFastenerKind : uint8
{
	Screw,       // turned out with the part's tool
	Thumbscrew,  // turned out by hand
	Clip,        // pressed open (RAM retention clips, PCIe latch)
	Plug,        // pulled off along Direction (power cables, data cables)
	Lever        // flipped open (CPU socket lever)
};

/** One screw, clip or plug of a part, in the device's own space (cm, origin at the bottom centre of the device). */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEFastener
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	EWEFastenerKind Kind = EWEFastenerKind::Screw;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FVector Location = FVector::ZeroVector;

	/** Screws: the screw axis (comes out along it). Plugs: pull direction. Clips/levers: the way they open. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FVector Direction = FVector(-1.f, 0.f, 0.f);

	/** Optional: where the fastener model's X axis points (a lever's arm, the side a clip tips away from). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FVector Side = FVector::ZeroVector;

	/** Fasteners of a lower order come first (cables before the bracket screw before the latch). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	uint8 Order = 0;
};

/**
 * One component that is really built into the device: its own model at its mounting position, how it is fastened,
 * what has to come out first and which way it is pulled out. Part i corresponds to step i of the device's
 * UWEDisassemblyDefinition (tool, time and outputs live there, so economy, tests and employees keep working).
 */
USTRUCT(BlueprintType)
struct WORKSHOPEMPIRE_API FWEAssemblyPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FText DisplayName;

	/** Model in device space (modelled at its mounting position, so all parts at identity form the device). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TArray<FWEFastener> Fasteners;

	/** Parts (Ids) that must be out before this one is reachable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TArray<FName> Requires;

	/** The way the part comes out (device space) and how far before it is free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FVector PullDirection = FVector(-1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly", meta = (Units = "cm"))
	float PullDistance = 6.f;

	/** Opening it is not a disassembly (side panel): a repair is still possible afterwards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bServiceAccess = false;

	/** Stays beside the device on the bench after removal (side panel), instead of going into the bench storage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bLaysBeside = false;

	/** The device body itself: removing it uses the device up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bShell = false;

	/** The device works without it (CD drive, case fan): the function test does not need it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	bool bOptional = false;

	/** Phase 19: better parts that fit the same place (variant 1, 2, 3); the part from the disassembly step is variant 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TArray<TObjectPtr<class UWEItemDefinition>> Alternatives;
};

/**
 * Part layout of a device (Phase 18a): the PC is a real case with a mainboard, graphics card, RAM, cooler, CPU,
 * drives, fans and power supply mounted inside. The workbench shows it part by part, and each part comes out on
 * its own (in any order its prerequisites allow).
 */
UCLASS(BlueprintType)
class WORKSHOPEMPIRE_API UWEAssemblyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxParts = 31;

	int32 FindPart(FName Id) const { return Parts.IndexOfByPredicate([Id](const FWEAssemblyPart& P) { return P.Id == Id; }); }

	/** Not out yet and every part it requires is out. */
	bool IsAvailable(int32 Part, int32 RemovedMask) const;
	/** Names of the parts that still block Part. */
	TArray<FText> GetBlockers(int32 Part, int32 RemovedMask) const;
	/** First available part in list order (the order employees and "automatic" use), or INDEX_NONE. */
	int32 GetDefaultPart(int32 RemovedMask) const;
	bool IsComplete(int32 RemovedMask) const;

	// --- Reassembly ---------------------------------------------------------------------------------------
	/**
	 * Part is out and can go back in now: the way to its place is open (everything it requires is still out) and
	 * what it is mounted on is in (the parts that need it out first). Covers (service access) only need the way open.
	 */
	bool CanInstall(int32 Part, int32 RemovedMask) const;
	/** Why Part cannot go back in yet ("Erst ausbauen: ..." / "Erst einbauen: ..."); empty if it can. */
	FText GetInstallBlocker(int32 Part, int32 RemovedMask) const;
	/** Parts the function test needs that are still out (not optional, not covers). */
	TArray<FText> GetMissingForTest(int32 RemovedMask) const;
	/** "Taken apart" count of the device (FWEItemInstance::DisassemblyStep): parts out, without the optional ones. */
	uint8 CountTakenApart(int32 RemovedMask) const;
	/** Optional parts that are out (they lower the value a little). */
	int32 CountMissingOptional(int32 RemovedMask) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	TArray<FWEAssemblyPart> Parts;

	/** How the device lies on the bench while it is worked on (PC: on its right side, open side up). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assembly")
	FRotator BenchRotation = FRotator::ZeroRotator;
};
