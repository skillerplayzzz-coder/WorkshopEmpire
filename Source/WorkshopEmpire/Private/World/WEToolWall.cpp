#include "World/WEToolWall.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/WEPlayerController.h"
#include "Interaction/WEInteractable.h"
#include "Inventory/WEInventoryComponent.h"
#include "Items/WEItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/WECharacter.h"
#include "Sound/SoundBase.h"
#include "WELog.h"
#include "Workshop/WEToolHand.h"

#define LOCTEXT_NAMESPACE "WEToolWall"

namespace WEToolWallConst
{
	const TCHAR* BoardPath = TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_Pegboard.SM_Gen_Pegboard");
	const TCHAR* HookPath = TEXT("/Game/WorkshopEmpire/Art/Generated/SM_Gen_PegHook.SM_Gen_PegHook");
	const TCHAR* OutlinePath = TEXT("/Game/WorkshopEmpire/Materials/Props/MI_Prop_ToolOutline.MI_Prop_ToolOutline");
	const TCHAR* TakeSound = TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_drawKnife1.SFX_K_drawKnife1");
	const TCHAR* HangSound = TEXT("/Game/WorkshopEmpire/Audio/P21/SFX_K_beltHandle1.SFX_K_beltHandle1");
	constexpr float BoardThickness = 1.6f;
	constexpr float MaxToolLength = 34.f;
	constexpr float RowSpacing = 46.f;

	UStaticMesh* LoadMesh(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	/** Hanging pose: tip down, handle up, flat against the board (the generated tools point down along -Z). */
	FRotator HangRotation(EWEToolType Tool)
	{
		// (The hammer's handle runs along +X and its head along Z: roll the head flat, then the handle up.)
		return Tool == EWEToolType::Hammer ? FRotator(90.f, 0.f, 90.f) : FRotator(0.f, 90.f, 0.f);
	}
}

static bool IsDrill(const UWEItemDefinition* Def)
{
	return Def && Def->ItemId == FName(TEXT("CordlessDrill"));
}

void AWEToolWall::GatherTools(const AActor* Character, TArray<const UWEItemDefinition*>& OutTools)
{
	const AWECharacter* WECharacter = Cast<AWECharacter>(Character);
	if (const UWEInventoryComponent* Inventory = WECharacter ? WECharacter->GetPlayerInventory() : nullptr)
	{
		for (const FWEInventorySlotData& Entry : Inventory->GetAllSlots())
		{
			if (Entry.Item.Definition && Entry.Item.Definition->ToolType != EWEToolType::None)
			{
				OutTools.AddUnique(Entry.Item.Definition);
			}
		}
	}
	if (!Character || !Character->GetWorld())
	{
		return;
	}
	for (TActorIterator<AWEToolWall> It(Character->GetWorld()); It; ++It)
	{
		if (!It->IsUnlocked() || FVector::Dist2D(It->GetActorLocation(), Character->GetActorLocation()) > ReachCm)
		{
			continue;
		}
		for (const FWEInventorySlotData& Entry : It->GetStorage()->GetAllSlots())
		{
			if (Entry.Item.Definition && Entry.Item.Definition->ToolType != EWEToolType::None)
			{
				OutTools.AddUnique(Entry.Item.Definition);
			}
		}
	}
}

FVector AWEToolWall::GetHookLocation(int32 Slot) const
{
	const int32 Column = Slot % HookColumns;
	const int32 Row = Slot / HookColumns;
	const float Spacing = (Size.Y - 34.f) / (HookColumns - 1);
	const float BoardFront = -Size.X * 0.5f + 2.f + WEToolWallConst::BoardThickness;
	return FVector(BoardFront, (Column - (HookColumns - 1) * 0.5f) * Spacing, Size.Z - 22.f - Row * WEToolWallConst::RowSpacing);
}

int32 AWEToolWall::CountTools(const UWEInventoryComponent* Inventory)
{
	int32 Count = 0;
	for (const FWEInventorySlotData& Entry : Inventory ? Inventory->GetAllSlots() : TArray<FWEInventorySlotData>())
	{
		Count += Entry.Item.Definition && Entry.Item.Definition->ToolType != EWEToolType::None ? 1 : 0;
	}
	return Count;
}

void AWEToolWall::BeginPlay()
{
	Super::BeginPlay();
	Boxes->SetVisibility(false); // (the base class shows the boxes again when there is no model)
	if (!bBoundStorage)
	{
		bBoundStorage = true;
		Storage->OnInventoryChanged.AddUObject(this, &AWEToolWall::OnStorageChanged);
	}
	RefreshHooks();
}

void AWEToolWall::RebuildVisuals()
{
	Super::RebuildVisuals();
	// The shelf boxes stay for collision; a pegboard with hooks shows instead.
	Boxes->SetVisibility(false);
	auto MakePart = [this](UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(GetRootComponent());
		Part->RegisterComponent();
		return Part;
	};
	if (!Board)
	{
		Board = MakePart(WEToolWallConst::LoadMesh(WEToolWallConst::BoardPath));
	}
	// The board model: origin bottom centre of its back, 150 wide, 140 high.
	Board->SetRelativeLocation(FVector(-Size.X * 0.5f + 2.f, 0.f, Size.Z - 160.f));
	Board->SetRelativeScale3D(FVector(1.f, (Size.Y - 10.f) / 150.f, 1.f));
	const int32 NumHooks = FMath::Max(Storage->GetNumSlots(), 1);
	UStaticMesh* HookMesh = WEToolWallConst::LoadMesh(WEToolWallConst::HookPath);
	while (Hooks.Num() < NumHooks)
	{
		Hooks.Add(MakePart(HookMesh));
		UStaticMeshComponent* Tool = MakePart(nullptr);
		Hanging.Add(Tool);
		UStaticMeshComponent* Outline = MakePart(nullptr);
		Outline->SetCastShadow(false);
		Outlines.Add(Outline);
		OutlineMesh.Add(nullptr);
		OutlineType.Add(EWEToolType::None);
	}
	for (int32 i = 0; i < Hooks.Num(); ++i)
	{
		Hooks[i]->SetRelativeLocation(GetHookLocation(i));
		Hooks[i]->SetVisibility(i < NumHooks);
	}
	RefreshHooks();
}

void AWEToolWall::OnStorageChanged(UWEInventoryComponent* Changed)
{
	const int32 Before = GetHangingCount();
	if (Hooks.Num() < Storage->GetNumSlots())
	{
		RebuildVisuals(); // more slots from an upgrade: more hooks
	}
	RefreshHooks();
	const int32 After = GetHangingCount();
	if (After != Before && GetNetMode() != NM_DedicatedServer && HasActorBegunPlay())
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, After < Before ? WEToolWallConst::TakeSound : WEToolWallConst::HangSound, nullptr, LOAD_Quiet | LOAD_NoWarn))
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation() + FVector(0.f, 0.f, 120.f), 0.6f);
		}
	}
}

void AWEToolWall::RefreshHooks()
{
	UMaterialInterface* OutlineMaterial = LoadObject<UMaterialInterface>(nullptr, WEToolWallConst::OutlinePath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	for (int32 i = 0; i < Hanging.Num(); ++i)
	{
		const FWEItemInstance* Item = Storage->GetItemInSlot(i);
		const UWEItemDefinition* Def = Item ? Item->Definition.Get() : nullptr;
		const EWEToolType Tool = Def ? Def->ToolType : EWEToolType::None;
		const TCHAR* Path = Tool != EWEToolType::None ? AWEToolHand::MeshFor(Tool, IsDrill(Def)) : nullptr;
		UStaticMesh* Mesh = Path ? WEToolWallConst::LoadMesh(Path) : nullptr;
		if (!Mesh && Def && Tool != EWEToolType::None)
		{
			Mesh = Def->WorldMesh; // a tool without a work-view model: its pickup model
		}
		if (Mesh)
		{
			OutlineMesh[i] = Mesh;
			OutlineType[i] = Tool;
		}
		// Hang it so its top sits just under the hook, scaled down if it is very long.
		auto Place = [this, i](UStaticMeshComponent* C, UStaticMesh* M, EWEToolType T, float ScaleBoost, float FrontOffset)
		{
			C->SetStaticMesh(M);
			if (!M)
			{
				return;
			}
			const FRotator Rotation = WEToolWallConst::HangRotation(T);
			const FBox Bounds = M->GetBoundingBox().TransformBy(FTransform(Rotation));
			const float Scale = FMath::Min(1.f, WEToolWallConst::MaxToolLength / FMath::Max(Bounds.GetSize().Z, 1.f)) * ScaleBoost;
			const FVector Hook = GetHookLocation(i);
			const FVector Offset(Hook.X - Bounds.Min.X * Scale + FrontOffset, Hook.Y - Bounds.GetCenter().Y * Scale, Hook.Z + 1.f - Bounds.Max.Z * Scale);
			C->SetRelativeLocationAndRotation(Offset, Rotation);
			C->SetRelativeScale3D(FVector(Scale));
		};
		Place(Hanging[i], Mesh, Tool, 1.f, 0.4f);
		Hanging[i]->SetVisibility(Mesh != nullptr);
		// The painted outline of whatever hung here last (flush with the board, where the tool belongs).
		UStaticMesh* Shape = OutlineMesh[i];
		if (Shape && !Mesh)
		{
			Place(Outlines[i], Shape, OutlineType[i], 1.04f, 0.f);
			for (int32 m = 0; OutlineMaterial && m < Outlines[i]->GetNumMaterials(); ++m)
			{
				Outlines[i]->SetMaterial(m, OutlineMaterial);
			}
			// Flat: a thin slice on the board, not a second tool (mesh Y is the board normal in both hanging poses).
			const FVector S = Outlines[i]->GetRelativeScale3D();
			Outlines[i]->SetRelativeScale3D(FVector(S.X, 0.03f, S.Z));
			FVector L = Outlines[i]->GetRelativeLocation();
			L.X = GetHookLocation(i).X + 0.1f;
			Outlines[i]->SetRelativeLocation(L);
		}
		Outlines[i]->SetVisibility(Shape != nullptr && !Mesh);
	}
}

int32 AWEToolWall::GetHangingCount() const
{
	int32 Count = 0;
	for (const UStaticMeshComponent* C : Hanging)
	{
		Count += C && C->IsVisible() && C->GetStaticMesh() ? 1 : 0;
	}
	return Count;
}

int32 AWEToolWall::FindAimedTool(const AWECharacter* Character) const
{
	if (!Character)
	{
		return INDEX_NONE;
	}
	FVector Eye;
	FRotator View;
	Character->GetActorEyesViewPoint(Eye, View);
	const FVector Dir = View.Vector();
	int32 Best = INDEX_NONE;
	float BestAlong = 400.f;
	for (int32 i = 0; i < Hanging.Num(); ++i)
	{
		const FWEItemInstance* Item = Storage->GetItemInSlot(i);
		if (!Item || !Item->Definition || Item->Definition->ToolType == EWEToolType::None)
		{
			continue;
		}
		// The middle of the hanging tool.
		const FVector Point = GetActorTransform().TransformPosition(GetHookLocation(i) + FVector(3.f, 0.f, -WEToolWallConst::MaxToolLength * 0.4f));
		const float Along = FVector::DotProduct(Point - Eye, Dir);
		const float Off = FVector::Dist(Eye + Dir * Along, Point);
		if (Along > 0.f && Along < BestAlong && Off < 16.f)
		{
			BestAlong = Along;
			Best = i;
		}
	}
	return Best;
}

int32 AWEToolWall::HangUpTools(AWECharacter* Character)
{
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	if (!HasAuthority() || !Inventory)
	{
		return 0;
	}
	int32 Hung = 0;
	for (const FWEInventorySlotData& Entry : Inventory->GetAllSlots())
	{
		if (!Entry.Item.Definition || Entry.Item.Definition->ToolType == EWEToolType::None)
		{
			continue;
		}
		const int32 Free = Storage->FindFirstEmptySlot();
		if (Free == INDEX_NONE)
		{
			break;
		}
		if (Inventory->MoveItem(Entry.SlotIndex, Storage, Free) == EWEInventoryResult::Success)
		{
			++Hung;
		}
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Tool wall %s: %d tool(s) hung up, %d hanging"), *GetName(), Hung, CountTools(Storage));
	return Hung;
}

bool AWEToolWall::TakeTool(AWECharacter* Character, int32 Slot)
{
	UWEInventoryComponent* Inventory = Character ? Character->GetPlayerInventory() : nullptr;
	const FWEItemInstance* Item = Storage->GetItemInSlot(Slot);
	if (!HasAuthority() || !Inventory || !Item || !Item->Definition || Item->Definition->ToolType == EWEToolType::None)
	{
		return false;
	}
	const FText Name = Item->Definition->DisplayName;
	const int32 Free = Inventory->FindFirstEmptySlot();
	if (Free == INDEX_NONE || Storage->MoveItem(Slot, Inventory, Free) != EWEInventoryResult::Success)
	{
		if (AWEPlayerController* PC = Cast<AWEPlayerController>(Character->GetController()))
		{
			PC->ClientShowMessage(LOCTEXT("NoRoom", "Kein Platz im Inventar."));
		}
		return false;
	}
	UE_LOG(LogWorkshopEmpire, Log, TEXT("Tool wall %s: took %s from hook %d"), *GetName(), *Name.ToString(), Slot);
	return true;
}

bool AWEToolWall::GetSecondaryInteraction(const AWECharacter* Interactor, FText& OutText) const
{
	const int32 Carried = CountTools(Interactor ? Interactor->GetPlayerInventory() : nullptr);
	if (!HasStorage() || Carried == 0 || Storage->FindFirstEmptySlot() == INDEX_NONE)
	{
		return false;
	}
	OutText = Carried == 1 ? LOCTEXT("HangOne", "Werkzeug aufhängen") : FText::Format(LOCTEXT("HangMany", "{0} Werkzeuge aufhängen"), FText::AsNumber(Carried));
	return true;
}

void AWEToolWall::InteractSecondary(AWECharacter* Interactor)
{
	HangUpTools(Interactor);
}

bool AWEToolWall::GetAlternativeInteraction(const AWECharacter* Interactor, FText& OutText, FText& OutReason) const
{
	const int32 Slot = HasStorage() ? FindAimedTool(Interactor) : INDEX_NONE;
	const FWEItemInstance* Item = Slot != INDEX_NONE ? Storage->GetItemInSlot(Slot) : nullptr;
	if (!Item || !Item->Definition)
	{
		return false;
	}
	OutText = FText::Format(LOCTEXT("Take", "{0} nehmen"), Item->Definition->DisplayName);
	return true;
}

void AWEToolWall::InteractAlternative(AWECharacter* Interactor)
{
	const int32 Slot = FindAimedTool(Interactor);
	if (Slot != INDEX_NONE)
	{
		TakeTool(Interactor, Slot);
	}
}

bool AWEToolWall::GetExamineInfo(FWEExamineInfo& OutInfo) const
{
	if (!Super::GetExamineInfo(OutInfo))
	{
		return false;
	}
	OutInfo.Description = LOCTEXT("Desc", "Werkzeug an seinem Platz: Was hier hängt, ist an der Werkbank in der Nähe griffbereit. Leere Haken zeigen den Umriss des fehlenden Werkzeugs.");
	OutInfo.Details.Add(FText::Format(LOCTEXT("Hanging", "Hängt: {0} Werkzeug(e)"), FText::AsNumber(CountTools(Storage))));
	return true;
}

#undef LOCTEXT_NAMESPACE
