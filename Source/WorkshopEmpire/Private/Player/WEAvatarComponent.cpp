#include "Player/WEAvatarComponent.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WELog.h"

namespace WEAvatar
{
	const TCHAR* Root = TEXT("/Game/Characters/Quaternius");

	FString AssetPath(const FString& Folder, const FString& Name)
	{
		return FString::Printf(TEXT("%s/%s/%s.%s"), Root, *Folder, *Name, *Name);
	}

	template <typename T>
	T* Load(const FString& Path)
	{
		return LoadObject<T>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}

	USkeletalMesh* BodyMesh(bool bFemale)
	{
		const TCHAR* Name = bFemale ? TEXT("Superhero_Female_FullBody") : TEXT("Superhero_Male_FullBody");
		return Load<USkeletalMesh>(AssetPath(FString::Printf(TEXT("%s/%s/SkeletalMeshes"), bFemale ? TEXT("Female") : TEXT("Male"), Name), Name));
	}

	USkeletalMesh* HairMesh(const TCHAR* Name)
	{
		return Load<USkeletalMesh>(AssetPath(FString::Printf(TEXT("Hair/%s/%s/SkeletalMeshes"), Name, Name), Name));
	}

	/** Hair textures: set 1 (short styles, beard, regular brows) and set 2 (long, buns, fine brows). */
	UTexture2D* HairTexture(bool bSet2)
	{
		return bSet2 ? Load<UTexture2D>(AssetPath(TEXT("Hair/Hair_Long/Hair_Long/Textures"), TEXT("T_Hair_2_BaseColor")))
			: Load<UTexture2D>(AssetPath(TEXT("Hair/Hair_SimpleParted/Hair_SimpleParted/Textures"), TEXT("T_Hair_1_BaseColor")));
	}

	TArray<FTransform> RefComponentSpace(const FReferenceSkeleton& Ref)
	{
		const TArray<FTransform>& Local = Ref.GetRefBonePose();
		TArray<FTransform> Out;
		Out.SetNum(Local.Num());
		for (int32 i = 0; i < Local.Num(); ++i)
		{
			const int32 Parent = Ref.GetParentIndex(i);
			Out[i] = Parent == INDEX_NONE ? Local[i] : Local[i] * Out[Parent];
		}
		return Out;
	}

	/** Quaternius has three spine bones, the mannequin five. */
	FName SourceName(FName Target)
	{
		if (Target == TEXT("spine_02")) { return TEXT("spine_03"); }
		if (Target == TEXT("spine_03")) { return TEXT("spine_05"); }
		return Target;
	}

	/** The bone whose position gives a limb bone its direction. */
	FName DirectionChild(const FString& Bone)
	{
		const FString Side = Bone.Right(2); // "_l" / "_r"
		static const TMap<FString, FString> Children = {
			{ TEXT("clavicle"), TEXT("upperarm") }, { TEXT("upperarm"), TEXT("lowerarm") }, { TEXT("lowerarm"), TEXT("hand") }, { TEXT("hand"), TEXT("middle_01") },
			{ TEXT("thigh"), TEXT("calf") }, { TEXT("calf"), TEXT("foot") }, { TEXT("foot"), TEXT("ball") },
			{ TEXT("index_01"), TEXT("index_02") }, { TEXT("index_02"), TEXT("index_03") }, { TEXT("middle_01"), TEXT("middle_02") }, { TEXT("middle_02"), TEXT("middle_03") },
			{ TEXT("ring_01"), TEXT("ring_02") }, { TEXT("ring_02"), TEXT("ring_03") }, { TEXT("pinky_01"), TEXT("pinky_02") }, { TEXT("pinky_02"), TEXT("pinky_03") },
			{ TEXT("thumb_01"), TEXT("thumb_02") }, { TEXT("thumb_02"), TEXT("thumb_03") },
		};
		const FString Base = Bone.LeftChop(2);
		const FString* Child = (Side == TEXT("_l") || Side == TEXT("_r")) ? Children.Find(Base) : nullptr;
		return Child ? FName(*(*Child + Side)) : NAME_None;
	}

	// Look tables.
	const FLinearColor HairColors[] = { FLinearColor(0.02f, 0.018f, 0.016f), FLinearColor(0.07f, 0.04f, 0.02f), FLinearColor(0.18f, 0.11f, 0.05f),
		FLinearColor(0.55f, 0.42f, 0.22f), FLinearColor(0.35f, 0.10f, 0.03f), FLinearColor(0.36f, 0.36f, 0.36f) };
	const TCHAR* HairStyles[] = { nullptr, TEXT("Hair_SimpleParted"), TEXT("Hair_Buzzed"), TEXT("Hair_BuzzedFemale"), TEXT("Hair_Long"), TEXT("Hair_Buns") };
	const FLinearColor OutfitColors[] = { FLinearColor(0.08f, 0.18f, 0.45f), FLinearColor(0.32f, 0.33f, 0.35f), FLinearColor(0.12f, 0.30f, 0.14f),
		FLinearColor(0.50f, 0.08f, 0.06f), FLinearColor(0.035f, 0.035f, 0.04f) };
	// The pack has a light and a dark skin texture; the tint spreads them over five clearly different tones.
	const float SkinBlend[] = { 0.f, 0.2f, 0.6f, 1.f, 1.f };
	const FLinearColor SkinTint[] = { FLinearColor(1.08f, 1.04f, 1.02f), FLinearColor(0.97f, 0.9f, 0.85f), FLinearColor(0.85f, 0.74f, 0.66f), FLinearColor(0.62f, 0.52f, 0.46f),
		FLinearColor(0.38f, 0.31f, 0.28f) };
}

UWEAvatarComponent::UWEAvatarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicatedByDefault(false);
}

void UWEAvatarComponent::SetDriver(USkeletalMeshComponent* InDriver)
{
	Driver = InDriver;
	if (Driver)
	{
		AddTickPrerequisiteComponent(Driver);
	}
}

void UWEAvatarComponent::SetOwnerNoSee(bool bNoSee)
{
	bOwnerNoSee = bNoSee;
	if (Body)
	{
		Body->SetOwnerNoSee(bNoSee);
	}
	for (USceneComponent* Part : Parts)
	{
		if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(Part))
		{
			P->SetOwnerNoSee(bNoSee);
		}
	}
}

void UWEAvatarComponent::ApplyLook(const FWECharacterLook& InLook)
{
	FWECharacterLook Clamped = InLook;
	Clamped.Clamp();
	if (bHasLook && Clamped == Look && Body)
	{
		return;
	}
	Look = Clamped;
	bHasLook = true;
	BuildBody();
}

USkeletalMeshComponent* UWEAvatarComponent::MakeFollower(const TCHAR* MeshName)
{
	USkeletalMesh* Mesh = MeshName ? WEAvatar::HairMesh(MeshName) : nullptr;
	if (!Mesh || !Body)
	{
		return nullptr;
	}
	USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	C->SetSkeletalMesh(Mesh);
	C->SetupAttachment(Body);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetOwnerNoSee(bOwnerNoSee);
	C->RegisterComponent();
	C->SetLeaderPoseComponent(Body);
	Parts.Add(C);
	return C;
}

UStaticMeshComponent* UWEAvatarComponent::MakeAccessory(const TCHAR* Name)
{
	const FString Full = FString::Printf(TEXT("SM_Gen_%s_%s"), Name, Look.Body == 1 ? TEXT("Female") : TEXT("Male"));
	UStaticMesh* Mesh = WEAvatar::Load<UStaticMesh>(WEAvatar::AssetPath(TEXT("Accessories"), Full));
	if (!Mesh || !Body || !Body->GetSkinnedAsset())
	{
		return nullptr;
	}
	// Modelled in body space at the bind pose: hang it on the head bone with the inverse of the head's rest transform.
	const FReferenceSkeleton& Ref = Body->GetSkinnedAsset()->GetRefSkeleton();
	const int32 Head = Ref.FindBoneIndex(TEXT("Head"));
	const TArray<FTransform> CS = WEAvatar::RefComponentSpace(Ref);
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	C->SetStaticMesh(Mesh);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetOwnerNoSee(bOwnerNoSee);
	C->SetupAttachment(Body, Head != INDEX_NONE ? FName(TEXT("Head")) : NAME_None);
	C->RegisterComponent();
	if (Head != INDEX_NONE)
	{
		C->SetRelativeTransform(CS[Head].Inverse());
	}
	Parts.Add(C);
	return C;
}

void UWEAvatarComponent::BuildBody()
{
	for (USceneComponent* Part : Parts)
	{
		if (Part) { Part->DestroyComponent(); }
	}
	Parts.Reset();
	HairMaterials.Reset();
	const bool bFemale = Look.Body == 1;
	USkeletalMesh* Mesh = WEAvatar::BodyMesh(bFemale);
	if (!Mesh)
	{
		// Models not imported: the mannequin stays visible.
		if (Driver) { Driver->SetHiddenInGame(false); }
		return;
	}
	if (!Body || BuiltBody != Look.Body)
	{
		if (Body) { Body->DestroyComponent(); }
		Body = NewObject<UPoseableMeshComponent>(GetOwner(), TEXT("AvatarBody"), RF_Transient);
		Body->SetSkinnedAssetAndUpdate(Mesh);
		Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Same place as the driver, so both share one component space.
		if (Driver)
		{
			Body->SetupAttachment(Driver->GetAttachParent());
			Body->SetRelativeTransform(Driver->GetRelativeTransform());
			Body->bCastHiddenShadow = Driver->bCastHiddenShadow;
		}
		else
		{
			Body->SetupAttachment(this);
		}
		Body->SetOwnerNoSee(bOwnerNoSee);
		Body->RegisterComponent();
		BuiltBody = Look.Body;
		BuildMapping();
	}
	if (Driver)
	{
		// The mannequin only drives: hidden, but its animation keeps running.
		Driver->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Driver->SetHiddenInGame(true);
	}
	MakeFollower(Look.Face == 1 || Look.Face == 3 ? TEXT("Eyebrows_Female") : TEXT("Eyebrows_Regular"));
	if (Look.Face >= 2 && !bFemale)
	{
		MakeFollower(TEXT("Hair_Beard"));
	}
	if (Look.Hair > 0 && Look.Hair < UE_ARRAY_COUNT(WEAvatar::HairStyles))
	{
		MakeFollower(WEAvatar::HairStyles[Look.Hair]);
	}
	if (Look.Headwear == 1) { MakeAccessory(TEXT("Cap")); }
	if (Look.Headwear == 2) { MakeAccessory(TEXT("Beanie")); }
	if (Look.Glasses == 1) { MakeAccessory(TEXT("Sunglasses")); }
	ApplyMaterials();
	Retarget();
}

void UWEAvatarComponent::ApplyMaterials()
{
	if (!Body)
	{
		return;
	}
	const bool bFemale = Look.Body == 1;
	UMaterialInterface* AvatarBase = WEAvatar::Load<UMaterialInterface>(WEAvatar::AssetPath(TEXT(""), TEXT("M_WEAvatar")).Replace(TEXT("//"), TEXT("/")));
	UMaterialInterface* HairBase = WEAvatar::Load<UMaterialInterface>(WEAvatar::AssetPath(TEXT(""), TEXT("M_WEHair")).Replace(TEXT("//"), TEXT("/")));
	const FLinearColor HairColor = WEAvatar::HairColors[FMath::Clamp<int32>(Look.HairColor, 0, UE_ARRAY_COUNT(WEAvatar::HairColors) - 1)];
	auto HairMID = [this, HairBase, HairColor](UMeshComponent* Mesh, bool bSet2, bool bHide)
	{
		if (!HairBase || !Mesh)
		{
			return;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(HairBase, this);
		MID->SetTextureParameterValue(TEXT("HairTex"), WEAvatar::HairTexture(bSet2));
		MID->SetVectorParameterValue(TEXT("HairColor"), HairColor);
		MID->SetScalarParameterValue(TEXT("Hide"), bHide ? 1.f : 0.f);
		HairMaterials.Add(MID);
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Mesh->SetMaterial(i, MID);
		}
	};
	// Body: skin + clothes material on the body slot; the built-in brows are hidden (the brows are a follower).
	const TArray<FName> Slots = Body->GetMaterialSlotNames();
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FString SlotName = Slots[i].ToString();
		if (SlotName.Contains(TEXT("Superhero")) && AvatarBase)
		{
			BodyMaterial = UMaterialInstanceDynamic::Create(AvatarBase, this);
			Body->SetMaterial(i, BodyMaterial);
		}
		else if (SlotName.Contains(TEXT("Hair")) && HairBase)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(HairBase, this);
			MID->SetScalarParameterValue(TEXT("Hide"), 1.f);
			Body->SetMaterial(i, MID);
		}
	}
	for (USceneComponent* Part : Parts)
	{
		if (USkeletalMeshComponent* Follower = Cast<USkeletalMeshComponent>(Part))
		{
			const FString Name = Follower->GetSkeletalMeshAsset() ? Follower->GetSkeletalMeshAsset()->GetName() : FString();
			HairMID(Follower, Name.Contains(TEXT("Long")) || Name.Contains(TEXT("Buns")) || Name.Contains(TEXT("Female")) && Name.Contains(TEXT("Eyebrows")), false);
		}
		else if (UStaticMeshComponent* Accessory = Cast<UStaticMeshComponent>(Part); Accessory && Accessory->GetStaticMesh()
			&& !Accessory->GetStaticMesh()->GetName().Contains(TEXT("Sunglasses")))
		{
			// Cap and beanie in the colour of the clothes (darker for the beanie).
			UMaterialInstanceDynamic* MID = Accessory->CreateDynamicMaterialInstance(0);
			const FLinearColor Main = WEAvatar::OutfitColors[FMath::Clamp<int32>(Look.OutfitColor, 0, 4)];
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("Tint"), Accessory->GetStaticMesh()->GetName().Contains(TEXT("Beanie")) ? Main * 0.7f : Main);
			}
		}
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Avatar %s: look %s, %d slots, body material %s"), *GetNameSafe(GetOwner()), *Look.ToString(), Slots.Num(),
		BodyMaterial ? *BodyMaterial->GetName() : TEXT("none"));
	if (!BodyMaterial)
	{
		return;
	}
	// Skin.
	const int32 Skin = FMath::Clamp<int32>(Look.Skin, 0, 4);
	const FString Gender = bFemale ? TEXT("Female") : TEXT("Male");
	BodyMaterial->SetTextureParameterValue(TEXT("SkinLight"), WEAvatar::Load<UTexture2D>(WEAvatar::AssetPath(TEXT("Skin"), FString::Printf(TEXT("T_%s_Light"), *Gender))));
	BodyMaterial->SetTextureParameterValue(TEXT("SkinDark"), WEAvatar::Load<UTexture2D>(WEAvatar::AssetPath(
		FString::Printf(TEXT("%s/Superhero_%s_FullBody/Textures"), *Gender, *Gender), bFemale ? TEXT("T_Superhero_Female_Dark_BaseColor") : TEXT("T_Superhero_Male_Dark"))));
	BodyMaterial->SetTextureParameterValue(TEXT("NormalTex"), WEAvatar::Load<UTexture2D>(WEAvatar::AssetPath(
		FString::Printf(TEXT("%s/Superhero_%s_FullBody/Textures"), *Gender, *Gender), FString::Printf(TEXT("T_Superhero_%s_Normal"), *Gender))));
	BodyMaterial->SetTextureParameterValue(TEXT("RoughTex"), WEAvatar::Load<UTexture2D>(WEAvatar::AssetPath(
		FString::Printf(TEXT("%s/Superhero_%s_FullBody/Textures"), *Gender, *Gender), FString::Printf(TEXT("T_Superhero_%s_Roughness"), *Gender))));
	BodyMaterial->SetScalarParameterValue(TEXT("SkinBlend"), WEAvatar::SkinBlend[Skin]);
	BodyMaterial->SetVectorParameterValue(TEXT("SkinTint"), WEAvatar::SkinTint[Skin]);
	// Body proportions for the clothes regions.
	const float Height = bFemale ? 178.f : 182.f;
	BodyMaterial->SetScalarParameterValue(TEXT("Height"), Height);
	BodyMaterial->SetScalarParameterValue(TEXT("FrontSign"), 1.f);
	BodyMaterial->SetScalarParameterValue(TEXT("TorsoX"), bFemale ? 18.f : 20.f);
	BodyMaterial->SetScalarParameterValue(TEXT("NeckZ"), bFemale ? 0.815f : 0.82f);
	BodyMaterial->SetScalarParameterValue(TEXT("WaistZ"), 0.56f);
	BodyMaterial->SetScalarParameterValue(TEXT("ArmZ"), 0.70f);
	// Clothes: main colour from the choice, the rest from the outfit.
	const FLinearColor Main = WEAvatar::OutfitColors[FMath::Clamp<int32>(Look.OutfitColor, 0, 4)];
	const float ShortSleeve = bFemale ? 31.f : 34.f;
	const float LongSleeve = bFemale ? 65.f : 73.f;
	FLinearColor Shirt = Main, Pants = Main, Accent(0.85f, 0.85f, 0.6f);
	float Sleeve = LongSleeve, Stripes = 0.f, Knee = 0.f, Pocket = 0.f, Zipper = 0.f;
	switch (Look.Outfit)
	{
	case 0: Stripes = 1.f; Zipper = 1.f; break;                                                                   // work overall
	case 1: Pants = FLinearColor(0.42f, 0.36f, 0.25f); Sleeve = ShortSleeve; break;                               // polo + chinos
	case 2: Pants = FLinearColor(0.10f, 0.16f, 0.30f); Pocket = 1.f; BodyMaterial->SetScalarParameterValue(TEXT("NeckZ"), bFemale ? 0.83f : 0.835f); break; // hoodie + jeans
	case 3: Pants = FLinearColor(0.17f, 0.17f, 0.18f); Sleeve = ShortSleeve; Knee = 1.f; break;                   // t-shirt + work trousers
	default: Pants = FLinearColor(0.12f, 0.12f, 0.14f); Zipper = 1.f; Accent = FLinearColor(0.03f, 0.03f, 0.03f); break; // jacket + trousers
	}
	BodyMaterial->SetVectorParameterValue(TEXT("Shirt"), Shirt);
	BodyMaterial->SetVectorParameterValue(TEXT("Pants"), Pants);
	BodyMaterial->SetVectorParameterValue(TEXT("Accent"), Accent);
	BodyMaterial->SetScalarParameterValue(TEXT("SleeveX"), Sleeve);
	BodyMaterial->SetScalarParameterValue(TEXT("Stripes"), Stripes);
	BodyMaterial->SetScalarParameterValue(TEXT("KneePads"), Knee);
	BodyMaterial->SetScalarParameterValue(TEXT("PocketBand"), Pocket);
	BodyMaterial->SetScalarParameterValue(TEXT("Zipper"), Zipper);
	BodyMaterial->SetScalarParameterValue(TEXT("PantsOn"), 1.f);
	BodyMaterial->SetScalarParameterValue(TEXT("ShirtOn"), 1.f);
	// Shoes: height of the shoe region and colour.
	static const float ShoeHeight[] = { 11.f, 13.f, 26.f };
	static const FLinearColor ShoeColor[] = { FLinearColor(0.75f, 0.75f, 0.74f), FLinearColor(0.10f, 0.07f, 0.05f), FLinearColor(0.28f, 0.17f, 0.08f) };
	const int32 Shoe = FMath::Clamp<int32>(Look.Shoes, 0, 2);
	BodyMaterial->SetScalarParameterValue(TEXT("ShoeZ"), ShoeHeight[Shoe] / Height);
	BodyMaterial->SetVectorParameterValue(TEXT("Shoe"), ShoeColor[Shoe]);
}

void UWEAvatarComponent::BuildMapping()
{
	Map.Reset();
	PelvisMap = INDEX_NONE;
	if (!Body || !Body->GetSkinnedAsset() || !Driver || !Driver->GetSkinnedAsset())
	{
		return;
	}
	const FReferenceSkeleton& T = Body->GetSkinnedAsset()->GetRefSkeleton();
	const FReferenceSkeleton& S = Driver->GetSkinnedAsset()->GetRefSkeleton();
	const TArray<FTransform> TCS = WEAvatar::RefComponentSpace(T);
	const TArray<FTransform> SCS = WEAvatar::RefComponentSpace(S);
	for (int32 t = 0; t < T.GetNum(); ++t)
	{
		const FName Name = T.GetBoneName(t);
		if (Name == TEXT("root") || Name.ToString().Contains(TEXT("leaf")))
		{
			continue;
		}
		const int32 s = S.FindBoneIndex(WEAvatar::SourceName(Name));
		if (s == INDEX_NONE)
		{
			continue;
		}
		FBoneMap& M = Map.AddDefaulted_GetRef();
		M.Name = Name;
		M.Source = s;
		M.Target = t;
		M.SourceRef = SCS[s].GetRotation();
		M.TargetRef = TCS[t].GetRotation();
		// Limbs: turn the target's rest direction (T-pose) onto the source's (A-pose) before the motion is applied.
		const FName Child = WEAvatar::DirectionChild(Name.ToString());
		const int32 tc = Child.IsNone() ? INDEX_NONE : T.FindBoneIndex(Child);
		const int32 sc = Child.IsNone() ? INDEX_NONE : S.FindBoneIndex(Child);
		if (tc != INDEX_NONE && sc != INDEX_NONE)
		{
			const FVector DirT = (TCS[tc].GetLocation() - TCS[t].GetLocation()).GetSafeNormal();
			const FVector DirS = (SCS[sc].GetLocation() - SCS[s].GetLocation()).GetSafeNormal();
			if (!DirT.IsNearlyZero() && !DirS.IsNearlyZero())
			{
				M.Align = FQuat::FindBetweenNormals(DirT, DirS);
			}
		}
		if (Name == TEXT("pelvis"))
		{
			PelvisMap = Map.Num() - 1;
			SourcePelvisRef = SCS[s].GetLocation();
			TargetPelvisRef = TCS[t].GetLocation();
			PelvisScale = SourcePelvisRef.Z > 1.f ? TargetPelvisRef.Z / SourcePelvisRef.Z : 1.f;
		}
	}
	UE_LOG(LogWorkshopEmpire, Verbose, TEXT("Avatar: %d bones mapped"), Map.Num());
}

void UWEAvatarComponent::Retarget()
{
	if (!Body || !Driver || Map.Num() == 0)
	{
		return;
	}
	const TArray<FTransform>& Source = Driver->GetComponentSpaceTransforms();
	if (Source.Num() == 0)
	{
		return;
	}
	for (int32 i = 0; i < Map.Num(); ++i)
	{
		const FBoneMap& M = Map[i];
		if (!Source.IsValidIndex(M.Source))
		{
			continue;
		}
		const FQuat Current = Source[M.Source].GetRotation();
		const FQuat Target = Current * M.SourceRef.Inverse() * M.Align * M.TargetRef;
		Body->SetBoneRotationByName(M.Name, Target.Rotator(), EBoneSpaces::ComponentSpace);
		if (i == PelvisMap)
		{
			const FVector Moved = (Source[M.Source].GetLocation() - SourcePelvisRef) * PelvisScale;
			Body->SetBoneLocationByName(M.Name, TargetPelvisRef + Moved, EBoneSpaces::ComponentSpace);
		}
	}
}

void UWEAvatarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Nobody sees it (and it casts no visible shadow there): skip the work.
	if (Body && (Body->WasRecentlyRendered(0.25f) || Body->bCastHiddenShadow))
	{
		Retarget();
	}
}
