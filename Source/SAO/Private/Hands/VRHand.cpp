
#include "Hands/VRHand.h"
#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/SphereComponent.h"
#include "Characters/VRCharacterBase.h" 
#include "Camera/CameraComponent.h" 


// コンポーネントの初期設定
AVRHand::AVRHand()
{
 	// PCデバッグ時の手位置更新に使用するためTickを有効化する。
	PrimaryActorTick.bCanEverTick = true;
	MotionController = CreateDefaultSubobject<UMotionControllerComponent>("MotionController");
	SetRootComponent(MotionController);

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HandMesh");
	HandMesh->SetupAttachment(MotionController);

	widgetInteractionComponent = CreateDefaultSubobject<UWidgetInteractionComponent>("WidgetInteraction");
	widgetInteractionComponent->SetupAttachment(HandMesh);

	GrabSphere = CreateDefaultSubobject<USphereComponent>("GrabSphere");
	GrabSphere->SetupAttachment(HandMesh);
}

void AVRHand::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	switch (HandType)
	{
	case EControllerHand::Left:
		MotionController->MotionSource = "Left";
		break;
	case EControllerHand::Right:
		MotionController->MotionSource = "Right";
		break;

	default:
		break;
	}
}

// ゲーム開始時の初期化
void AVRHand::BeginPlay()
{
	Super::BeginPlay();

	
}

void AVRHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVRHand::InitializeOffset()
{
	OwnerCharacter = Cast<AVRCharacterBase>(GetOwner());
	
	if(HandType != EControllerHand::Left && HandType != EControllerHand::Right)
	{
		if(GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 4.554f, FColor::Red, FString::Printf(TEXT("Class %s: Wrong Hand"), *GetClass()->GetName()));
		}
	}

	//  PCデバッグ用の前後オフセットを初期化する
	PC_ForwardOffset = 0.f;

	// 手ごとの初期位置を決める
	if (HandType == EControllerHand::Left)
	{
		PC_BaseHandOffset = FVector(0.f, 0.f, 0.f);
	}
	else if (HandType == EControllerHand::Right)
	{
		PC_BaseHandOffset = FVector(0.f, 0.f, 0.f);
	}
}

void AVRHand::DynamicOffset(float DeltaTime)
{
	if (!bPCDebugEnabled || !OwnerCharacter) return;
	if (OwnerCharacter->bIsVRMode) return;

	// 前後操作（左手のみ）
	if (bMoveForwardByMouse && HandType == EControllerHand::Left)
	{
		const float Speed = 30.f;
		PC_ForwardOffset += MouseYInput * Speed * DeltaTime;
	}

	UCameraComponent* Cam = OwnerCharacter->GetCamera();
	if (!Cam) return;

	const FTransform CamTF = Cam->GetComponentTransform();

	//  基準 + 動的オフセット
	FVector Target =
		CamTF.TransformPosition(PC_BaseHandOffset) +
		CamTF.GetRotation().GetForwardVector() * PC_ForwardOffset;

	SetActorLocation(Target);
}


void AVRHand::GrabObject()
{
	TArray<AActor*> OverlappingActors;
	GrabSphere->GetOverlappingActors(OverlappingActors);

	if(!OverlappingActors.IsEmpty())
	{
		AActor* FirstActorUnderCollision = OverlappingActors[0];
		if(FirstActorUnderCollision)
		{
			// 具体的な武器クラスに依存せず、インターフェース経由で共通の掴み操作を行う
			CurrentlyGrabbedActor = TScriptInterface<IInteractInterface>(FirstActorUnderCollision);
			if(CurrentlyGrabbedActor)
			{
				CurrentlyGrabbedActor->OnGrab(HandMesh, HandMesh->GetComponentLocation());
			}
		}
	}
}

void AVRHand::ReleaseObject()
{
	if(CurrentlyGrabbedActor)
	{
		CurrentlyGrabbedActor->OnRelease(HandMesh);
		CurrentlyGrabbedActor = nullptr;
	}
}

void AVRHand::PC_SetMoveForwardByMouse(bool bEnable)
{
	// 左手だけ許可
	if (HandType != EControllerHand::Left) return;

	bMoveForwardByMouse = bEnable;

	if (OwnerCharacter)
	{
		// 手操作中は視点を止める
		OwnerCharacter->bLook = !bEnable;
	}
}

void AVRHand::PC_SetMouseY(float Value)
{
	if (HandType != EControllerHand::Left) return;

	MouseYInput = Value;
}
