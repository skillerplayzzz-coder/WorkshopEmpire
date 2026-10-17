#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/WEGameState.h"
#include "Fx/WEFx.h"
#include "Machines/WEMachineDefinition.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/WECharacter.h"
#include "Player/WEFootstepComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Staff/WEEmployee.h"
#include "Staff/WEStaffComponent.h"
#include "WETestItems.h"
#include "WETestWorld.h"
#include "UI/WEGraphicsSettings.h"
#include "World/WESurface.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace WETestItems;

namespace WEVisualTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

using namespace WEVisualTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEFxBurstTest, "WorkshopEmpire.Visual.ParticleBurstsAndLoops", Flags)
bool FWEFxBurstTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	for (const EWEFxKind Kind : { EWEFxKind::Sparks, EWEFxKind::Dust, EWEFxKind::Smoke, EWEFxKind::Chips, EWEFxKind::Embers, EWEFxKind::Steam, EWEFxKind::Motes })
	{
		const AWEFx::FSettings S = AWEFx::GetSettings(Kind);
		const FString Name = StaticEnum<EWEFxKind>()->GetNameStringByValue(static_cast<int64>(Kind));
		TestTrue(*FString::Printf(TEXT("%s: lives"), *Name), S.Life > 0.f && S.SizeStart > 0.f);
		TestTrue(*FString::Printf(TEXT("%s: bursts or loops"), *Name), S.BurstCount > 0 || S.SpawnRate > 0.f);
	}
	AWEFx* Sparks = AWEFx::Burst(TW.World, EWEFxKind::Sparks, FVector(0.f, 0.f, 100.f), FVector::UpVector);
	if (!TestNotNull(TEXT("Burst spawned"), Sparks))
	{
		return false;
	}
	TestEqual(TEXT("All sparks at once"), Sparks->GetNumParticles(), AWEFx::GetSettings(EWEFxKind::Sparks).BurstCount);
	UInstancedStaticMeshComponent* Instances = Sparks->FindComponentByClass<UInstancedStaticMeshComponent>();
	TestTrue(TEXT("Drawn as instances"), Instances && Instances->GetInstanceCount() == Sparks->GetNumParticles());
	TestTrue(TEXT("Glow material"), Instances && Instances->GetMaterial(0) && Instances->GetMaterial(0)->GetBaseMaterial()->GetName() == TEXT("M_WEFxGlow"));
	// Sparks fall: after 0.2 s they are lower or have bounced, and they all die.
	for (int32 i = 0; i < 4; ++i)
	{
		Sparks->Tick(0.05f);
	}
	TestTrue(TEXT("Still flying"), Sparks->GetNumParticles() > 0);
	for (int32 i = 0; i < 40 && IsValid(Sparks); ++i)
	{
		Sparks->Tick(0.05f);
	}
	TestTrue(TEXT("Burst cleans itself up"), !IsValid(Sparks) || Sparks->IsActorBeingDestroyed());

	// A loop emits only while active.
	AActor* Machine = TW.Spawn<AActor>(FVector(300.f, 0.f, 0.f));
	USceneComponent* Root = NewObject<USceneComponent>(Machine);
	Machine->SetRootComponent(Root);
	Root->RegisterComponent();
	AWEFx* Smoke = AWEFx::Loop(Root, EWEFxKind::Smoke, FVector(0.f, 0.f, 250.f));
	if (!TestNotNull(TEXT("Loop spawned"), Smoke))
	{
		return false;
	}
	TestEqual(TEXT("Loop follows the machine"), Smoke->GetAttachParentActor(), Machine);
	for (int32 i = 0; i < 20; ++i)
	{
		Smoke->Tick(0.05f);
	}
	const int32 Emitted = Smoke->GetNumParticles();
	TestTrue(TEXT("Smoke rises while on"), Emitted > 0);
	Smoke->SetEmitting(false);
	for (int32 i = 0; i < 200; ++i)
	{
		Smoke->Tick(0.05f);
	}
	TestEqual(TEXT("Fades out when off"), Smoke->GetNumParticles(), 0);
	TestTrue(TEXT("Loop stays for the next run"), IsValid(Smoke) && !Smoke->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWESurfaceTest, "WorkshopEmpire.Visual.Surfaces", Flags)
bool FWESurfaceTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	TestEqual(TEXT("Instance path"), WESurface::GetInstancePath(EWESurface::Brick), FString(TEXT("/Game/WorkshopEmpire/Materials/Surfaces/MI_WEBrick.MI_WEBrick")));
	AActor* Owner = TW.Spawn<AActor>(FVector::ZeroVector);
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
	Mesh->RegisterComponent();
	WESurface::Apply(Mesh, EWESurface::Placeholder, FLinearColor::Red);
	UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
	TestTrue(TEXT("Placeholder keeps the old look"), MID && MID->GetBaseMaterial()->GetName() == TEXT("M_WEPlaceholder"));
	// Every surface kind either has its textured instance or falls back cleanly (never an empty material).
	for (int32 i = 1; i < StaticEnum<EWESurface>()->NumEnums() - 1; ++i)
	{
		const EWESurface Kind = static_cast<EWESurface>(i);
		WESurface::Apply(Mesh, Kind, FLinearColor::White, 0.5f);
		UMaterialInterface* Material = Mesh->GetMaterial(0);
		const bool bTextured = Material && Material->GetBaseMaterial()->GetName() == TEXT("M_WESurface");
		TestTrue(*FString::Printf(TEXT("%s has a material"), *StaticEnum<EWESurface>()->GetNameStringByIndex(i)), Material != nullptr);
		if (bTextured)
		{
			AddInfo(FString::Printf(TEXT("%s: textured"), *StaticEnum<EWESurface>()->GetNameStringByIndex(i)));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEMachineFeedbackTest, "WorkshopEmpire.Visual.MachineFeedbackData", Flags)
bool FWEMachineFeedbackTest::RunTest(const FString& Parameters)
{
	// Real data: every machine sounds (loop, start, stop) and shows its work; looping sounds loop, 3D sounds fade with distance.
	for (const TCHAR* Name : { TEXT("Shredder"), TEXT("Sorter"), TEXT("Press"), TEXT("Assembler"), TEXT("Furnace"), TEXT("Packer") })
	{
		const UWEMachineDefinition* Def = LoadObject<UWEMachineDefinition>(nullptr, *FString::Printf(TEXT("/Game/WorkshopEmpire/Data/Machines/MD_%s.MD_%s"), Name, Name));
		if (!TestNotNull(Name, Def))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s: running sound loops"), Name), Def->RunningSound && Cast<USoundWave>(Def->RunningSound) && Cast<USoundWave>(Def->RunningSound)->bLooping);
		TestTrue(*FString::Printf(TEXT("%s: start and stop"), Name), Def->StartSound && Def->StopSound);
		TestTrue(*FString::Printf(TEXT("%s: end of cycle"), Name), Def->CycleSound != nullptr && Def->CycleFx != EWEFxKind::None);
	}
	const UWEMachineDefinition* Furnace = LoadObject<UWEMachineDefinition>(nullptr, TEXT("/Game/WorkshopEmpire/Data/Machines/MD_Furnace.MD_Furnace"));
	TestTrue(TEXT("Furnace smokes from the chimney and glows at the mouth"), Furnace && Furnace->RunningFx == EWEFxKind::Smoke && Furnace->SecondRunningFx == EWEFxKind::Embers
		&& Furnace->RunningFxOffset.Z > 250.f);
	const USoundWave* Crunch = LoadObject<USoundWave>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_ShredderCrunch.SFX_ShredderCrunch"));
	TestTrue(TEXT("Machine sounds fade with distance"), Crunch && Crunch->AttenuationSettings && Crunch->AttenuationSettings->Attenuation.bAttenuate);
	const USoundWave* RoomTone = LoadObject<USoundWave>(nullptr, TEXT("/Game/WorkshopEmpire/Audio/SFX_RoomTone.SFX_RoomTone"));
	TestTrue(TEXT("Ambience loops"), RoomTone && RoomTone->bLooping);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWECharacterLookTest, "WorkshopEmpire.Visual.AnimatedCharacters", Flags)
bool FWECharacterLookTest::RunTest(const FString& Parameters)
{
	FWETestWorld TW;
	AWEGameState* GameState = TW.SpawnGameState<AWEGameState>();
	AWECharacter* Player = SpawnPlayer(TW, FVector(0.f, 0.f, 100.f));
	USkeletalMeshComponent* Body = Player ? Player->GetMesh() : nullptr;
	TestTrue(TEXT("Player has a mannequin"), Body && Body->GetSkeletalMeshAsset() && Body->GetSkeletalMeshAsset()->GetName().StartsWith(TEXT("SKM_")));
	TestTrue(TEXT("Animated (locomotion AnimBP)"), Body && Body->GetAnimClass() && Body->GetAnimClass()->GetName().Contains(TEXT("ABP_Unarmed")));
	TestTrue(TEXT("Own body hidden in first person"), Body && Body->bOwnerNoSee);
	TestTrue(TEXT("Feet on the floor"), Body && FMath::IsNearlyEqual(Body->GetRelativeLocation().Z, -88.f, 1.f));
	TestNotNull(TEXT("Player footsteps"), Player ? Player->FindComponentByClass<UWEFootstepComponent>() : nullptr);

	FWEEmployeeProfile Profile;
	Profile.Name = TEXT("Anna Bauer");
	Profile.Role = EWEStaffRole::Driver;
	AWEEmployee* Worker = GameState->GetStaff()->SpawnEmployee(Profile, FTransform(FVector(200.f, 0.f, 100.f)));
	USkeletalMeshComponent* WorkerBody = Worker ? Worker->GetMesh() : nullptr;
	TestTrue(TEXT("Employee is a mannequin too"), WorkerBody && WorkerBody->GetSkeletalMeshAsset() && WorkerBody->GetAnimClass());
	UMaterialInstanceDynamic* Clothes = WorkerBody ? Cast<UMaterialInstanceDynamic>(WorkerBody->GetMaterial(1)) : nullptr;
	FLinearColor Color;
	TestTrue(TEXT("Work clothes in the role colour"), Clothes && Clothes->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Paint Tint")), Color)
		&& Color.Equals(Worker->GetTint(), 0.01f));
	TestNotNull(TEXT("Employee footsteps"), Worker ? Worker->FindComponentByClass<UWEFootstepComponent>() : nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWEGraphicsQualityTest, "WorkshopEmpire.Visual.GraphicsQuality", WEVisualTests::Flags)
bool FWEGraphicsQualityTest::RunTest(const FString& Parameters)
{
	const int32 Before = WEGraphics::GetQuality();
	WEGraphics::SetQuality(1, false);
	TestEqual(TEXT("Set to Mittel"), WEGraphics::GetQuality(), 1);
	TestEqual(TEXT("Button shows the level"), WEGraphics::GetButtonLabel().ToString(), FString(TEXT("Grafik: Mittel")));
	WEGraphics::SetQuality(WEGraphics::NumLevels - 1, false);
	TestEqual(TEXT("Episch wraps to Niedrig"), WEGraphics::CycleQuality(false), 0);
	TestEqual(TEXT("Out of range is clamped"), (WEGraphics::SetQuality(9, false), WEGraphics::GetQuality()), WEGraphics::NumLevels - 1);
	TestEqual(TEXT("Names"), WEGraphics::GetQualityName(2).ToString(), FString(TEXT("Hoch")));
	WEGraphics::SetQuality(Before, false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
