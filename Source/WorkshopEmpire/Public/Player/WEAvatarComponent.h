#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Player/WECharacterLook.h"
#include "WEAvatarComponent.generated.h"

class UMaterialInstanceDynamic;
class UPoseableMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/** Phase 21: what the arms (and legs) do on top of the walk/idle animation - employees at work and on their break. */
UENUM()
enum class EWEWorkPose : uint8
{
	None,
	/** Something held in front with both hands. */
	Carry,
	/** Reaching into a shelf or machine (taking or putting in). */
	Reach,
	/** Working the controls of a machine. */
	Operate,
	/** A coffee mug, now and then a sip. */
	Drink,
	/** Chatting: a hand gestures now and then. */
	Talk,
	/** Sitting down. */
	Sit
};

/**
 * The visible body of a player (Phase 18c): a Quaternius base character with hair, brows, beard, clothes,
 * shoes, cap and glasses from FWECharacterLook. It is animated by the existing mannequin (the "driver" with the
 * locomotion AnimBP, kept invisible): every frame the driver's bone rotations are carried over - relative to each
 * skeleton's reference pose and with the limb directions of both rest poses aligned (A-pose vs. T-pose) - so no
 * animation assets have to be retargeted. Hair and brows follow the body bones (leader pose), accessories hang
 * on the head bone. Local only (each machine builds it from the replicated look).
 */
UCLASS(ClassGroup = (WorkshopEmpire), meta = (BlueprintSpawnableComponent))
class WORKSHOPEMPIRE_API UWEAvatarComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWEAvatarComponent();

	/** The animated mannequin to follow (it is hidden while the avatar shows). */
	void SetDriver(USkeletalMeshComponent* InDriver);
	void ApplyLook(const FWECharacterLook& InLook);
	const FWECharacterLook& GetLook() const { return Look; }
	bool HasBody() const { return Body != nullptr; }
	UPoseableMeshComponent* GetBody() const { return Body; }
	/** Same visibility rules as the driver had (first person: the owner does not see his own body). */
	void SetOwnerNoSee(bool bNoSee);
	/** Phase 20: players and employees wear the company logo on the overall (customers do not). */
	void SetShowCompanyLogo(bool bShow);
	/** Phase 20: forces the look to be rebuilt (the brand colour of work clothes changed). */
	void Rebuild() { BuildBody(); }
	/** Phase 21: arms/legs on top of the animation (blends in over a moment). */
	void SetWorkPose(EWEWorkPose InPose);
	EWEWorkPose GetWorkPose() const { return Pose; }
	/** Sit pose: seat height above the feet in cm (0 = a normal chair). */
	void SetSeatHeight(float Cm) { SeatHeight = Cm; }
	/** Tests: how far the pose has blended in (0-1). */
	float GetWorkPoseBlend() const { return PoseBlend; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Tests: number of bones carried over from the driver. */
	int32 GetMappedBoneCount() const { return Map.Num(); }
	/** Tests and shots: the garments worn (piece names). */
	TArray<FName> GetGarments() const;

protected:
	void BuildBody();
	void BuildMapping();
	void Retarget();
	void ApplyWorkPose(float DeltaTime);
	/** Turns Bone so that it points from itself towards Child along Dir (component space), Weight 0-1. */
	void AimBone(FName Bone, FName Child, const FVector& Dir, float Weight);
	EWEWorkPose Pose = EWEWorkPose::None;
	EWEWorkPose ShownPose = EWEWorkPose::None;
	float PoseBlend = 0.f;
	float PoseTime = 0.f;
	float SeatHeight = 0.f;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> HeldMug;
	void ApplyMaterials();
	USkeletalMeshComponent* MakeFollower(const TCHAR* MeshName);
	USkeletalMeshComponent* MakeFollower(USkeletalMesh* Mesh);
	/** A garment of Tools/Assets/build_clothing.py (tagged with its piece name). */
	USkeletalMeshComponent* MakeClothing(const TCHAR* Piece);
	UStaticMeshComponent* MakeAccessory(const TCHAR* Name);

	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Driver;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> Body;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Parts;
	UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> Clothes;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> HairMaterials;

	FWECharacterLook Look;
	bool bHasLook = false;
	int32 BuiltBody = -1;
	bool bOwnerNoSee = false;
	bool bShowCompanyLogo = false;

	struct FBoneMap
	{
		FName Name;
		int32 Source = INDEX_NONE;
		int32 Target = INDEX_NONE;
		FQuat SourceRef = FQuat::Identity;  // component space
		FQuat TargetRef = FQuat::Identity;  // component space
		FQuat Align = FQuat::Identity;      // turns the target's rest direction onto the source's
	};
	TArray<FBoneMap> Map;
	int32 PelvisMap = INDEX_NONE;
	FVector SourcePelvisRef = FVector::ZeroVector;
	FVector TargetPelvisRef = FVector::ZeroVector;
	float PelvisScale = 1.f;
};
