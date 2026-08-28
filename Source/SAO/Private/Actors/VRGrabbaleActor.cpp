#include "Actors/VRGrabbaleActor.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"

AVRGrabbaleActor::AVRGrabbaleActor()
{
	GrabRegion = CreateDefaultSubobject<UBoxComponent>(TEXT("GrabRegion"));
	GrabRegion->SetupAttachment(ActorMesh);
	GrabRegion->SetCollisionProfileName(TEXT("Grabbable"));

	SkillDetect = CreateDefaultSubobject<USphereComponent>(TEXT("SkillDetect"));
	SkillDetect->SetupAttachment(ActorMesh);
	SkillDetect->SetSphereRadius(25.f);

	// 検知設定（ただし最初はOFFにしておく）
	SkillDetect->SetGenerateOverlapEvents(true);
	SkillDetect->SetCollisionObjectType(ECC_WorldDynamic);
	SkillDetect->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkillDetect->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SkillDetect->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	EnableSkillDetect(false); // 初期状態ではスキル検知を無効化する
}

void AVRGrabbaleActor::EnableSkillDetect(bool bEnable)
{
	if (!SkillDetect) return;

	if (bEnable)
	{
		// 保持中のみスキル検知を有効化する
		SkillDetect->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SkillDetect->UpdateOverlaps();
	}
	else
	{
		// 非保持時はスキル検知を無効化する
		SkillDetect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkillDetect->UpdateOverlaps();
	}
}

void AVRGrabbaleActor::OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation)
{
	if (!InComponent) return;

	switch (GrabType)
	{
	case EGrabType::Free:
	case EGrabType::Snap:
		ActorMesh->SetSimulatePhysics(false);

		// 手に持ってる間はメッシュ干渉を切る（保持中はメッシュ同士の干渉を無効化する）
		ActorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (GrabType == EGrabType::Free)
		{
			bIsHeld = ActorMesh->AttachToComponent(
				InComponent,
				FAttachmentTransformRules::KeepWorldTransform
			);
		}
		else
		{
			ActorMesh->AttachToComponent(
				InComponent,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale
			);
			bIsHeld = true;
		}

		if (bIsHeld)
		{
			GrabbedBySkeletalMesh = InComponent;

			// 保持開始時にスキル検知を有効化する
			EnableSkillDetect(true);
		}
		break;

	case EGrabType::None:
	default:
		break;
	}
}

void AVRGrabbaleActor::OnRelease(USkeletalMeshComponent* InComponent)
{
	if (!InComponent) return;
	if (!bIsHeld) return;
	if (InComponent != GrabbedBySkeletalMesh) return;

	switch (GrabType)
	{
	case EGrabType::Free:
	case EGrabType::Snap:
		ActorMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

		// 手放した後に落下できるよう、物理演算とコリジョンを再有効化する
		ActorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ActorMesh->SetSimulatePhysics(true);

		bIsHeld = false;
		GrabbedBySkeletalMesh = nullptr;

		// 解放時にスキル検知を無効化する
		EnableSkillDetect(false);
		break;

	case EGrabType::None:
	default:
		break;
	}
}
