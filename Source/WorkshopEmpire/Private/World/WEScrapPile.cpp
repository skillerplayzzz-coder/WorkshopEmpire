#include "World/WEScrapPile.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Items/WELootTable.h"
#include "Items/WEPickupItem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "WELog.h"
#include "World/WEPlaceholderVisuals.h"

#define LOCTEXT_NAMESPACE "WEScrapPile"

AWEScrapPile::AWEScrapPile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMat(WEPlaceholder::MaterialPath);
	static ConstructorHelpers::FObjectFinder<UWELootTable> DefaultLoot(TEXT("/Game/WorkshopEmpire/Data/Loot/LT_ScrapPile.LT_ScrapPile"));
	PlaceholderMaterial = PlaceholderMat.Object;
	LootTable = DefaultLoot.Object;

	Heap = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Heap"));
	Heap->SetMobility(EComponentMobility::Static);
	Heap->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Heap;
	if (CubeMesh.Succeeded())
	{
		Heap->SetStaticMesh(CubeMesh.Object);
	}
	Random.Initialize(4711);
}

void AWEScrapPile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWEScrapPile, NextRefillTime);
}

void AWEScrapPile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// A low heap of tilted blocks (placeholder).
	Heap->ClearInstances();
	const FVector Blocks[][2] = {
		{ FVector(0.f, 0.f, 25.f), FVector(160.f, 120.f, 50.f) },
		{ FVector(-40.f, 30.f, 60.f), FVector(90.f, 70.f, 40.f) },
		{ FVector(35.f, -25.f, 55.f), FVector(70.f, 80.f, 35.f) },
		{ FVector(5.f, 10.f, 90.f), FVector(50.f, 40.f, 30.f) },
	};
	float Yaw = 0.f;
	for (const auto& Block : Blocks)
	{
		Heap->AddInstance(FTransform(FRotator(4.f, Yaw, -3.f), Block[0], Block[1] / 100.f));
		Yaw += 23.f;
	}
	WEPlaceholder::ApplyTint(Heap, PlaceholderMaterial, FLinearColor(0.20f, 0.11f, 0.07f));
}

void AWEScrapPile::BeginPlay()
{
	Super::BeginPlay();
	WEPlaceholder::ApplyTint(Heap, PlaceholderMaterial, FLinearColor(0.20f, 0.11f, 0.07f));
	if (HasAuthority())
	{
		Random.Initialize(FMath::Rand());
		OnRefillTimer();
		GetWorldTimerManager().SetTimer(RefillTimer, this, &AWEScrapPile::OnRefillTimer, RefillMinutes * 60.f, true);
	}
}

void AWEScrapPile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RefillTimer);
	Super::EndPlay(EndPlayReason);
}

void AWEScrapPile::OnRefillTimer()
{
	Refill();
	NextRefillTime = GetWorld()->GetTimeSeconds() + RefillMinutes * 60.f;
	ForceNetUpdate();
}

int32 AWEScrapPile::CountItemsAround() const
{
	int32 Count = 0;
	for (TActorIterator<AWEPickupItem> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed() && !It->IsHeld() && !It->GetPlacedOn()
			&& FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) <= Radius)
		{
			++Count;
		}
	}
	return Count;
}

int32 AWEScrapPile::Refill()
{
	if (!HasAuthority() || !LootTable)
	{
		return 0;
	}
	int32 Added = 0;
	for (int32 Missing = MaxItems - CountItemsAround(); Missing > 0; --Missing)
	{
		const FWEItemInstance Item = LootTable->RollOne(Random);
		if (!Item.IsValid())
		{
			break;
		}
		// Around the heap, not on it.
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const float Distance = Random.FRandRange(120.f, FMath::Max(130.f, Radius - 80.f));
		const FVector Location = GetActorLocation() + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 40.f);
		if (AWEPickupItem::SpawnWorldItem(GetWorld(), Item, FTransform(FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f), Location)))
		{
			++Added;
		}
	}
	if (Added > 0)
	{
		UE_LOG(LogWorkshopEmpire, Log, TEXT("Scrap pile: %d new items"), Added);
	}
	return Added;
}

FText AWEScrapPile::GetInteractionText(const AWECharacter* Interactor) const
{
	return LOCTEXT("Name", "Schrotthaufen");
}

bool AWEScrapPile::CanInteract(const AWECharacter* Interactor, FText& OutReason) const
{
	// Nothing to press: the hint says when new scrap comes.
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const float Now = GameState ? static_cast<float>(GameState->GetServerWorldTimeSeconds()) : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
	const int32 Minutes = FMath::Max(1, FMath::CeilToInt((NextRefillTime - Now) / 60.f));
	OutReason = FText::Format(LOCTEXT("Hint", "Schrotthaufen: Was herumliegt, ist gratis · neuer Schrott in ca. {0} min"), FText::AsNumber(Minutes));
	return false;
}

bool AWEScrapPile::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	OutInfo.Title = LOCTEXT("Title", "Schrotthaufen");
	OutInfo.Description = LOCTEXT("Desc", "Altmetall, kaputte Geräte, Kabelreste. Was hier herumliegt, darf man mitnehmen – selten ist sogar etwas Wertvolles dabei.");
	OutInfo.Details.Add(FText::Format(LOCTEXT("Rule", "Bis zu {0} Teile, Nachschub alle {1} Minuten"), FText::AsNumber(MaxItems), FText::AsNumber(FMath::RoundToInt(RefillMinutes))));
	return true;
}

#undef LOCTEXT_NAMESPACE
