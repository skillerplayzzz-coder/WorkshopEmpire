#include "Workshop/WELogoComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/WEUIKit.h"
#include "Workshop/WEBrandComponent.h"

namespace WELogoData
{
	const TCHAR* PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* LogoMaterial = TEXT("/Game/WorkshopEmpire/Materials/M_WELogo.M_WELogo");
}

UWELogoComponent::UWELogoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;
	bAutoActivate = true;
}

void UWELogoComponent::OnRegister()
{
	Super::OnRegister();
	if (Plate || !GetOwner() || IsRunningDedicatedServer())
	{
		return;
	}
	// The plate: Engine plane (normal +Z) turned to face +X.
	Plate = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	Plate->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, WELogoData::PlaneMesh));
	Plate->SetupAttachment(this);
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plate->SetCastShadow(false);
	Plate->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Plate->SetRelativeLocation(FVector(0.1f, 0.f, 0.f));
	Plate->SetRelativeScale3D(FVector(Size / 100.f));
	Plate->RegisterComponent();
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, WELogoData::LogoMaterial))
	{
		PlateMaterial = UMaterialInstanceDynamic::Create(Base, this);
		Plate->SetMaterial(0, PlateMaterial);
	}
	if (bShowName)
	{
		NameText = NewObject<UTextRenderComponent>(GetOwner(), NAME_None, RF_Transient);
		NameText->SetupAttachment(this);
		NameText->SetHorizontalAlignment(EHTA_Center);
		NameText->SetVerticalAlignment(EVRTA_TextTop);
		const float Letters = NameHeight > 0.f ? NameHeight : Size / 3.f;
		NameText->SetWorldSize(Letters);
		NameText->SetRelativeLocation(FVector(0.2f, 0.f, -Size * 0.5f - Letters * 0.15f));
		NameText->SetCastShadow(false);
		NameText->RegisterComponent();
	}
	ShownHash = 0;
	Refresh();
}

void UWELogoComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Plate and name are separate components of the owner: they go with the logo.
	if (Plate) { Plate->DestroyComponent(); Plate = nullptr; }
	if (NameText) { NameText->DestroyComponent(); NameText = nullptr; }
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UWELogoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Refresh();
}

void UWELogoComponent::Refresh()
{
	const UWEBrandComponent* BrandComponent = UWEBrandComponent::Get(this);
	const FWECompanyBrand Brand = BrandComponent ? BrandComponent->GetBrand() : FWECompanyBrand();
	const FString Name = BrandComponent ? BrandComponent->GetDisplayName() : Brand.Name;
	const uint32 Hash = HashCombine(Brand.Hash(), GetTypeHash(Name));
	if (Hash == ShownHash)
	{
		return;
	}
	ShownHash = Hash;
	const TArray<FName>& Symbols = WEBrand::Symbols();
	const TArray<FLinearColor>& Colors = WEBrand::Colors();
	ShownSymbol = Symbols.IsValidIndex(Brand.Symbol) ? Symbols[Brand.Symbol] : NAME_None;
	const FLinearColor SymbolColor = Colors.IsValidIndex(Brand.SymbolColor) ? Colors[Brand.SymbolColor] : FLinearColor::White;
	const FLinearColor Background = Colors.IsValidIndex(Brand.Background) ? Colors[Brand.Background] : FLinearColor::Black;
	if (PlateMaterial)
	{
		if (UTexture2D* Icon = WEUI::IconTexture(ShownSymbol))
		{
			PlateMaterial->SetTextureParameterValue(TEXT("Icon"), Icon);
		}
		PlateMaterial->SetVectorParameterValue(TEXT("SymbolColor"), SymbolColor);
		PlateMaterial->SetVectorParameterValue(TEXT("Background"), Background);
	}
	if (NameText)
	{
		NameText->SetText(FText::FromString(Name).ToUpper()); // FText: also Ä, Ö, Ü
		// Readable on any wall: the background colour, or white on a dark background behind it.
		NameText->SetTextRenderColor((Background.GetLuminance() < 0.25f ? SymbolColor : Background).ToFColor(true));
	}
}

void UWELogoComponent::SetLogoOwnerNoSee(bool bNoSee)
{
	if (Plate) { Plate->SetOwnerNoSee(bNoSee); }
	if (NameText) { NameText->SetOwnerNoSee(bNoSee); }
}

FString UWELogoComponent::GetShownName() const
{
	return NameText ? NameText->Text.ToString() : FString();
}
