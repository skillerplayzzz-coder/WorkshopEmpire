#include "Player/WEAvatarComponent.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WELog.h"
#include "Workshop/WELogoComponent.h"

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
		// 0.19.1: the everyday body (build_clothing.py slims the superhero body; the clothes are cut from it).
		const TCHAR* Slim = bFemale ? TEXT("SK_Female_Body") : TEXT("SK_Male_Body");
		if (USkeletalMesh* Everyday = Load<USkeletalMesh>(FString::Printf(TEXT("%s/Clothing/%s.%s"), Root, Slim, Slim)))
		{
			return Everyday;
		}
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
	/** Garments of an outfit (Tools/Assets/build_clothing.py) and of the shoes. */
	const TCHAR* const OutfitPieces[][2] = { { TEXT("Overall"), nullptr }, { TEXT("Polo"), TEXT("Chinos") }, { TEXT("Hoodie"), TEXT("Jeans") },
		{ TEXT("TShirt"), TEXT("WorkPants") }, { TEXT("Jacket"), TEXT("Trousers") } };
	const TCHAR* const ShoePieces[] = { TEXT("Sneakers"), TEXT("WorkShoes"), TEXT("Boots") };

	USkeletalMesh* ClothingMesh(bool bFemale, const TCHAR* Piece)
	{
		const FString Name = FString::Printf(TEXT("SK_%s_%s"), bFemale ? TEXT("Female") : TEXT("Male"), Piece);
		return Load<USkeletalMesh>(FString::Printf(TEXT("%s/Clothing/%s.%s"), Root, *Name, *Name));
	}

	/** Colours of the three slots (main, accent, trim) of a garment; the main colour of tops is the chosen one. */
	struct FClothColors
	{
		FLinearColor Slot[3];
		float Pattern = 0.f;  // 0 plain, 1 denim twill, 2 knit rib
	};

	FClothColors ClothColors(const FString& Piece, const FLinearColor& Main)
	{
		const FLinearColor Metal(0.55f, 0.55f, 0.52f), Leather(0.09f, 0.055f, 0.03f), Black(0.03f, 0.03f, 0.03f);
		if (Piece == TEXT("Overall"))   { return { { Main, FLinearColor(0.9f, 0.88f, 0.55f), FLinearColor(0.06f, 0.06f, 0.06f) }, 0.f }; }
		if (Piece == TEXT("Polo"))      { return { { Main, Main * 0.82f, FLinearColor(0.85f, 0.84f, 0.8f) }, 0.f }; }
		if (Piece == TEXT("Chinos"))    { return { { FLinearColor(0.42f, 0.36f, 0.25f), Leather, Metal }, 1.f }; }
		if (Piece == TEXT("Hoodie"))    { return { { Main, Main * 0.78f, Main }, 2.f }; }
		if (Piece == TEXT("Jeans"))     { return { { FLinearColor(0.09f, 0.15f, 0.30f), FLinearColor(0.55f, 0.38f, 0.12f), Leather }, 1.f }; }
		if (Piece == TEXT("TShirt"))    { return { { Main, Main * 0.8f, Main }, 0.f }; }
		if (Piece == TEXT("WorkPants")) { return { { FLinearColor(0.17f, 0.17f, 0.18f), FLinearColor(0.11f, 0.11f, 0.12f), Black }, 1.f }; }
		if (Piece == TEXT("Jacket"))    { return { { Main, FLinearColor(0.04f, 0.04f, 0.04f), Main * 0.65f }, 0.f }; }
		if (Piece == TEXT("Trousers"))  { return { { FLinearColor(0.12f, 0.12f, 0.14f), Black, Metal }, 0.f }; }
		if (Piece == TEXT("Sneakers"))  { return { { FLinearColor(0.75f, 0.75f, 0.74f), FLinearColor(0.9f, 0.9f, 0.88f), FLinearColor(0.85f, 0.85f, 0.85f) }, 0.f }; }
		if (Piece == TEXT("WorkShoes")) { return { { FLinearColor(0.10f, 0.07f, 0.05f), FLinearColor(0.02f, 0.02f, 0.02f), FLinearColor(0.07f, 0.05f, 0.035f) }, 0.f }; }
		if (Piece == TEXT("Boots"))     { return { { FLinearColor(0.28f, 0.17f, 0.08f), FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.12f, 0.08f, 0.04f) }, 0.f }; }
		return { { Main, Main, Main }, 0.f };
	}

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
		else if (UWELogoComponent* Logo = Cast<UWELogoComponent>(Part))
		{
			Logo->SetLogoOwnerNoSee(bNoSee);
		}
	}
}

void UWEAvatarComponent::SetShowCompanyLogo(bool bShow)
{
	if (bShowCompanyLogo != bShow)
	{
		bShowCompanyLogo = bShow;
		if (bHasLook)
		{
			BuildBody();
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

USkeletalMeshComponent* UWEAvatarComponent::MakeClothing(const TCHAR* Piece)
{
	USkeletalMesh* Mesh = Piece ? WEAvatar::ClothingMesh(Look.Body == 1, Piece) : nullptr;
	USkeletalMeshComponent* C = MakeFollower(Mesh);
	if (C)
	{
		C->ComponentTags.Add(FName(Piece));
		Clothes.Add(C);
	}
	return C;
}

TArray<FName> UWEAvatarComponent::GetGarments() const
{
	TArray<FName> Out;
	for (const USkeletalMeshComponent* Cloth : Clothes)
	{
		if (Cloth && Cloth->ComponentTags.Num() > 0)
		{
			Out.Add(Cloth->ComponentTags[0]);
		}
	}
	return Out;
}

USkeletalMeshComponent* UWEAvatarComponent::MakeFollower(const TCHAR* MeshName)
{
	return MakeFollower(MeshName ? WEAvatar::HairMesh(MeshName) : nullptr);
}

USkeletalMeshComponent* UWEAvatarComponent::MakeFollower(USkeletalMesh* Mesh)
{
	if (!Mesh || !Body)
	{
		return nullptr;
	}
	USkeletalMeshComponent* C = NewObject<USkeletalMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	C->SetSkeletalMesh(Mesh);
	C->SetupAttachment(Body);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetOwnerNoSee(bOwnerNoSee);
	C->bCastHiddenShadow = Body->bCastHiddenShadow;
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
	Clothes.Reset();
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
	const WELook::FFace Face = WELook::GetFace(Look);
	MakeFollower(Face.bFineBrows ? TEXT("Eyebrows_Female") : TEXT("Eyebrows_Regular"));
	if (Face.Beard && !bFemale)
	{
		MakeFollower(Face.Beard);
	}
	// Phase 19: real clothes over the body (the painted clothes of the body material stay underneath).
	for (const TCHAR* Piece : WEAvatar::OutfitPieces[FMath::Clamp<int32>(Look.Outfit, 0, UE_ARRAY_COUNT(WEAvatar::OutfitPieces) - 1)])
	{
		MakeClothing(Piece);
	}
	MakeClothing(WEAvatar::ShoePieces[FMath::Clamp<int32>(Look.Shoes, 0, UE_ARRAY_COUNT(WEAvatar::ShoePieces) - 1)]);
	// Phase 20: company logo on the chest of the work overall.
	if (bShowCompanyLogo && Look.Outfit == 0 && Body && Body->GetSkinnedAsset())
	{
		const FReferenceSkeleton& Ref = Body->GetSkinnedAsset()->GetRefSkeleton();
		const FName Bone = Ref.FindBoneIndex(TEXT("spine_03")) != INDEX_NONE ? FName(TEXT("spine_03")) : FName(TEXT("spine_02"));
		const int32 BoneIndex = Ref.FindBoneIndex(Bone);
		if (BoneIndex != INDEX_NONE)
		{
			const TArray<FTransform> CS = WEAvatar::RefComponentSpace(Ref);
			// Body space (T-pose, looking along +Y): on the chest opposite the pocket, on the overall's surface.
			const FTransform InBody(FRotator(0.f, 90.f, 0.f), bFemale ? FVector(-7.f, 14.1f, 126.f) : FVector(-7.5f, 13.6f, 131.f));
			UWELogoComponent* Logo = NewObject<UWELogoComponent>(GetOwner(), NAME_None, RF_Transient);
			Logo->Size = 8.f;
			Logo->bShowName = false;
			Logo->SetupAttachment(Body, Bone);
			Logo->RegisterComponent();
			Logo->SetRelativeTransform(InBody * CS[BoneIndex].Inverse());
			Logo->SetLogoOwnerNoSee(bOwnerNoSee);
			Parts.Add(Logo);
		}
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
		if (USkeletalMeshComponent* Follower = Cast<USkeletalMeshComponent>(Part); Follower && !Clothes.Contains(Follower))
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
	// Clothes: one fabric instance per slot.
	UMaterialInterface* ClothBase = WEAvatar::Load<UMaterialInterface>(WEAvatar::AssetPath(TEXT(""), TEXT("M_WECloth")).Replace(TEXT("//"), TEXT("/")));
	const FLinearColor MainColor = WEAvatar::OutfitColors[FMath::Clamp<int32>(Look.OutfitColor, 0, 4)];
	for (USkeletalMeshComponent* Cloth : Clothes)
	{
		if (!Cloth || !ClothBase || Cloth->ComponentTags.Num() == 0)
		{
			continue;
		}
		const WEAvatar::FClothColors Colors = WEAvatar::ClothColors(Cloth->ComponentTags[0].ToString(), MainColor);
		for (int32 i = 0; i < Cloth->GetNumMaterials() && i < 3; ++i)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ClothBase, this);
			MID->SetVectorParameterValue(TEXT("Tint"), Colors.Slot[i]);
			MID->SetScalarParameterValue(TEXT("Pattern"), i == 0 ? Colors.Pattern : 0.f);
			// Buckles and reflective stripes shine a little.
			const bool bShiny = Colors.Slot[i].Equals(FLinearColor(0.55f, 0.55f, 0.52f)) || Colors.Slot[i].Equals(FLinearColor(0.9f, 0.88f, 0.55f));
			MID->SetScalarParameterValue(TEXT("Roughness"), bShiny ? 0.35f : 0.86f);
			MID->SetScalarParameterValue(TEXT("Metallic"), Colors.Slot[i].Equals(FLinearColor(0.55f, 0.55f, 0.52f)) ? 0.9f : 0.f);
			Cloth->SetMaterial(i, MID);
		}
	}
	UE_LOG(LogWorkshopEmpire, Display, TEXT("Avatar %s: look %s, %d slots, body material %s, %d garments"), *GetNameSafe(GetOwner()), *Look.ToString(), Slots.Num(),
		BodyMaterial ? *BodyMaterial->GetName() : TEXT("none"), Clothes.Num());
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
	// Face details (Phase 19): eye height of each body, stubble in the hair colour, freckles, make-up.
	const WELook::FFace Face = WELook::GetFace(Look);
	BodyMaterial->SetScalarParameterValue(TEXT("EyeZ"), bFemale ? 163.4f : 168.0f);
	BodyMaterial->SetScalarParameterValue(TEXT("LipOffset"), bFemale ? 6.6f : 7.0f);
	BodyMaterial->SetScalarParameterValue(TEXT("Stubble"), Face.bStubble ? 1.f : 0.f);
	BodyMaterial->SetScalarParameterValue(TEXT("Freckles"), Face.bFreckles ? 1.f : 0.f);
	BodyMaterial->SetScalarParameterValue(TEXT("Makeup"), Face.bMakeup ? 1.f : 0.f);
	BodyMaterial->SetVectorParameterValue(TEXT("StubbleColor"), HairColor * 0.8f);
	BodyMaterial->SetVectorParameterValue(TEXT("LipColor"), FLinearColor(0.42f, 0.05f, 0.08f));
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
		ApplyWorkPose(DeltaTime);
	}
}

void UWEAvatarComponent::SetWorkPose(EWEWorkPose InPose)
{
	Pose = InPose;
}

void UWEAvatarComponent::AimBone(FName Bone, FName Child, const FVector& Dir, float Weight)
{
	if (!Body || Weight <= 0.f || Body->GetBoneIndex(Bone) == INDEX_NONE || Body->GetBoneIndex(Child) == INDEX_NONE)
	{
		return;
	}
	const FVector A = Body->GetBoneLocationByName(Bone, EBoneSpaces::ComponentSpace);
	const FVector C = Body->GetBoneLocationByName(Child, EBoneSpaces::ComponentSpace);
	const FVector Current = (C - A).GetSafeNormal();
	const FVector Wanted = Dir.GetSafeNormal();
	if (Current.IsNearlyZero() || Wanted.IsNearlyZero())
	{
		return;
	}
	const FQuat Delta = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Current, Wanted), Weight);
	const FQuat Rot = Body->GetBoneRotationByName(Bone, EBoneSpaces::ComponentSpace).Quaternion();
	Body->SetBoneRotationByName(Bone, (Delta * Rot).Rotator(), EBoneSpaces::ComponentSpace);
}

void UWEAvatarComponent::ApplyWorkPose(float DeltaTime)
{
	// Blend out of the old pose before blending into a new one.
	if (Pose != ShownPose)
	{
		PoseBlend = FMath::FInterpConstantTo(PoseBlend, 0.f, DeltaTime, 4.f);
		if (PoseBlend <= 0.f)
		{
			ShownPose = Pose;
			PoseTime = 0.f;
		}
	}
	else if (ShownPose != EWEWorkPose::None)
	{
		PoseBlend = FMath::FInterpConstantTo(PoseBlend, 1.f, DeltaTime, 3.f);
	}
	PoseTime += DeltaTime;
	if (HeldMug)
	{
		HeldMug->SetVisibility(ShownPose == EWEWorkPose::Drink && PoseBlend > 0.3f);
	}
	if (!Body || ShownPose == EWEWorkPose::None || PoseBlend <= 0.f || !GetOwner())
	{
		return;
	}
	const FTransform& CT = Body->GetComponentTransform();
	const FVector Fwd = CT.InverseTransformVectorNoScale(GetOwner()->GetActorForwardVector()).GetSafeNormal();
	const FVector Up = CT.InverseTransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	const FVector Right = CT.InverseTransformVectorNoScale(GetOwner()->GetActorRightVector()).GetSafeNormal();
	const float W = FMath::SmoothStep(0.f, 1.f, PoseBlend);
	const float T = PoseTime;
	// Arms: Side -1 = left, +1 = right ("outwards" is Right * Side).
	auto Arm = [this, W](float Side, const FVector& Upper, const FVector& Lower)
	{
		const TCHAR* S = Side < 0.f ? TEXT("_l") : TEXT("_r");
		AimBone(*FString::Printf(TEXT("upperarm%s"), S), *FString::Printf(TEXT("lowerarm%s"), S), Upper, W);
		AimBone(*FString::Printf(TEXT("lowerarm%s"), S), *FString::Printf(TEXT("hand%s"), S), Lower, W);
	};
	switch (ShownPose)
	{
	case EWEWorkPose::Carry:
		for (const float Side : { -1.f, 1.f })
		{
			Arm(Side, Fwd * 0.5f - Up * 0.85f + Right * Side * 0.12f, Fwd * 1.f + Up * 0.25f - Right * Side * 0.35f);
		}
		break;
	case EWEWorkPose::Reach:
		for (const float Side : { -1.f, 1.f })
		{
			const float Bob = 0.15f * FMath::Sin(T * 5.f + Side);
			Arm(Side, Fwd * 0.9f - Up * (0.45f + Bob) + Right * Side * 0.1f, Fwd * 1.f - Up * 0.35f - Right * Side * 0.2f);
		}
		break;
	case EWEWorkPose::Operate:
		for (const float Side : { -1.f, 1.f })
		{
			// Hands on the controls, moving a little (pressing, turning).
			const float Move = FMath::Sin(T * 2.6f + (Side > 0.f ? 0.f : 1.7f));
			Arm(Side, Fwd * 0.7f - Up * 0.7f + Right * Side * 0.15f, Fwd * 1.f - Up * (0.1f + 0.15f * Move) + Right * Side * 0.25f * Move);
		}
		break;
	case EWEWorkPose::Drink:
	{
		// Mug in the right hand; every few seconds a sip.
		const float Cycle = FMath::Fmod(T, 4.5f);
		const float Sip = Cycle < 2.f ? FMath::SmoothStep(0.f, 0.6f, Cycle) * (1.f - FMath::SmoothStep(1.4f, 2.f, Cycle)) : 0.f;
		const FVector LowerRest = Fwd * 1.f + Up * 0.15f - Right * 0.2f;
		const FVector LowerSip = Up * 1.f + Fwd * 0.3f - Right * 0.35f;
		Arm(1.f, Fwd * (0.3f + 0.2f * Sip) - Up * 0.9f + Right * 0.15f, FMath::Lerp(LowerRest, LowerSip, Sip));
		break;
	}
	case EWEWorkPose::Talk:
	{
		const float Gesture = FMath::Max(0.f, FMath::Sin(T * 1.7f)) * FMath::Max(0.f, FMath::Sin(T * 0.6f));
		Arm(1.f, Fwd * 0.2f - Up * 1.f + Right * 0.15f, Fwd * 1.f + Up * (0.1f + 0.6f * Gesture) + Right * 0.2f * Gesture);
		break;
	}
	case EWEWorkPose::Sit:
	{
		// Lower the hips to seat height, thighs forward, shins down, hands on the knees.
		const FVector Pelvis = Body->GetBoneLocationByName(TEXT("pelvis"), EBoneSpaces::ComponentSpace);
		// (Seat height known: the hips just above it, a bar stool lowers them only a little.)
		const float Drop = SeatHeight > 0.f ? FMath::Clamp(FVector::DotProduct(Pelvis, Up) - SeatHeight - 4.f, 5.f, 55.f) : 42.f;
		Body->SetBoneLocationByName(TEXT("pelvis"), Pelvis - Up * Drop * W, EBoneSpaces::ComponentSpace);
		for (const TCHAR* S : { TEXT("_l"), TEXT("_r") })
		{
			AimBone(*FString::Printf(TEXT("thigh%s"), S), *FString::Printf(TEXT("calf%s"), S), Fwd * 1.f - Up * 0.12f, W);
			AimBone(*FString::Printf(TEXT("calf%s"), S), *FString::Printf(TEXT("foot%s"), S), -Up * 1.f + Fwd * 0.12f, W);
		}
		for (const float Side : { -1.f, 1.f })
		{
			Arm(Side, Fwd * 0.4f - Up * 0.9f, Fwd * 1.f - Up * 0.3f - Right * Side * 0.1f);
		}
		break;
	}
	default:
		break;
	}
	// The mug sits in the right hand.
	if (ShownPose == EWEWorkPose::Drink)
	{
		if (!HeldMug)
		{
			HeldMug = NewObject<UStaticMeshComponent>(GetOwner());
			HeldMug->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
			HeldMug->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/WorkshopEmpire/Materials/Props/MI_Prop_PlasticWhite.MI_Prop_PlasticWhite"), nullptr, LOAD_Quiet | LOAD_NoWarn));
			HeldMug->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HeldMug->SetCastShadow(false);
			HeldMug->SetupAttachment(Body, TEXT("hand_r"));
			HeldMug->RegisterComponent();
			HeldMug->SetRelativeScale3D(FVector(0.09f, 0.09f, 0.1f));
			HeldMug->SetRelativeLocation(FVector(4.f, 3.f, 0.f));
		}
	}
}
