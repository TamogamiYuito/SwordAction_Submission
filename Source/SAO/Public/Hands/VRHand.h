#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractInterface.h"
#include "VRHand.generated.h"


class UMotionControllerComponent;
class USkeletalMeshComponent;
class UWidgetInteractionComponent;
class USphereComponent;
class AVRCharacterBase;

UCLASS()
class SAO_API AVRHand : public AActor
{
	GENERATED_BODY()
	
public:	
	// コンポーネントの初期設定 for this actor's properties
	AVRHand();

	UPROPERTY()
	TObjectPtr<AVRCharacterBase> OwnerCharacter;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void GrabObject();

	UFUNCTION(BlueprintCallable)
	void ReleaseObject();

#pragma region Components

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMotionControllerComponent> MotionController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hands")
	TObjectPtr<USkeletalMeshComponent> HandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hands")
	TObjectPtr<UWidgetInteractionComponent> widgetInteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Hands")
	TObjectPtr<USphereComponent> GrabSphere;

#pragma endregion


#pragma region HandData

	// 左右どちらの手として使用するかを設定する
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Hands|HandData")
	EControllerHand HandType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Hands|HandData")
	bool bMirrorAnimation;

#pragma endregion


#pragma region PC_Debug

	// PC上でVR操作を確認するためのデバッグ設定
	UPROPERTY(EditAnywhere, Category = "Debug|PC")
	bool bPCDebugEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "Debug|PC")
	FVector PC_HandOffset = FVector(40.f, 20.f, -10.f);

	bool bMoveForwardByMouse = false;
	float MouseYInput = 0.f;

	// 初期位置（手ごとに違う）
	UPROPERTY(EditDefaultsOnly, Category = "Debug|PC")
	FVector PC_BaseHandOffset = FVector::ZeroVector;

	// マウス操作で変わる分
	float PC_ForwardOffset = 0.f;


#pragma endregion


public:

	UFUNCTION(BlueprintCallable, Category = "Debug|PC")
	void PC_SetMoveForwardByMouse(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Debug|PC")
	void PC_SetMouseY(float Value);


	UFUNCTION(BlueprintCallable)
	void InitializeOffset();

	UFUNCTION(BlueprintCallable)
	void DynamicOffset(float DeltaTime);

private:
	TScriptInterface<IInteractInterface> CurrentlyGrabbedActor;
};
