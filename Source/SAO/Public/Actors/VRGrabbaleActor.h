#pragma once

#include "CoreMinimal.h"
#include "Actors/VRActor.h"
#include "Interface/InteractInterface.h"
#include "VRGrabbaleActor.generated.h"

class UBoxComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EGrabType : uint8
{
	Free,
	Snap,
	None
};

UCLASS()
class SAO_API AVRGrabbaleActor : public AVRActor, public IInteractInterface
{
	GENERATED_BODY()

public:
	AVRGrabbaleActor();

	UFUNCTION(BlueprintCallable, Category = "Grab")
	bool IsHeld() const { return bIsHeld; } // 任意（デバッグ用）

protected:
	virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation);
	virtual void OnRelease(USkeletalMeshComponent* InComponent);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBoxComponent> GrabRegion;

	// 検知専用：持ってる時だけ有効
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<USphereComponent> SkillDetect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup")
	EGrabType GrabType = EGrabType::Free;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Setup")
	bool bIsHeld = false;

private:
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> GrabbedBySkeletalMesh = nullptr;

private:
	void EnableSkillDetect(bool bEnable);
};
