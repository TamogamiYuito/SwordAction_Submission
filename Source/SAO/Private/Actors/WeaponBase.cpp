#include "Actors/WeaponBase.h"

#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "Enemies/EnemyBase.h" // ESwingLR と AEnemyBase

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(RootComponent);

	HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBox->SetCollisionObjectType(ECC_WorldDynamic);
	HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootComponent);
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(ActorMesh))
	{
		HitBox->AttachToComponent(
			ActorMesh,
			FAttachmentTransformRules::KeepRelativeTransform
		);

		NiagaraComponent->AttachToComponent(
			ActorMesh,
			FAttachmentTransformRules::KeepRelativeTransform
		);
	}
	else
	{
		UE_LOG(LogTemp, Fatal,
			TEXT("ActorMesh is null. AVRActor must CreateDefaultSubobject ActorMesh."));
	}

	LastLocation = GetHitPoint();
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateSwingState(DeltaTime);

	if (bIsSwinging)
	{
		CheckHit(DeltaTime);
	}

	LastLocation = GetHitPoint();
}

FVector AWeaponBase::GetHitPoint() const
{
	if (HitBox)
	{
		return HitBox->GetComponentLocation();
	}
	return GetActorLocation();
}

void AWeaponBase::UpdateSwingState(float DeltaTime)
{
	const FVector CurrentLocation = GetHitPoint();

	// DeltaTimeが極端に小さい場合の0除算を防止する
	const float SafeDT = FMath::Max(DeltaTime, 0.0001f);
	const float Speed = FVector::Dist(CurrentLocation, LastLocation) / SafeDT;

	// スイング開始
	if (!bIsSwinging && Speed > SwingStartSpeed)
	{
		bIsSwinging = true;
		HitActors.Empty();
		UE_LOG(LogTemp, Log, TEXT("Swing Start"));
	}

	// スイング終了
	if (bIsSwinging && Speed < SwingEndSpeed)
	{
		bIsSwinging = false;
		HitActors.Empty();
		UE_LOG(LogTemp, Log, TEXT("Swing End"));
	}
}

void AWeaponBase::PlayHitSFX(const FVector& Location) const
{
	if (!HitSFX) return;

	UGameplayStatics::PlaySoundAtLocation(
		this,
		HitSFX,
		Location,
		HitSFXVolume,
		HitSFXPitch
	);
}

void AWeaponBase::PlaySkillStartSFX_Attached(bool bStopIfPlaying)
{
	if (!SkillStartSFX || !ActorMesh) return;

	// 再生中の音を停止し、連続入力による音の重複を防ぐ
	if (bStopIfPlaying && SkillStartAudioComp && SkillStartAudioComp->IsPlaying())
	{
		SkillStartAudioComp->Stop();
		SkillStartAudioComp = nullptr;
	}

	// 武器メッシュに追従するようアタッチして再生する
	SkillStartAudioComp = UGameplayStatics::SpawnSoundAttached(
		SkillStartSFX,
		ActorMesh,                    // 武器メッシュに追従させて再生する
		NAME_None,
		FVector::ZeroVector,
		EAttachLocation::KeepRelativeOffset,
		true,                         // bStopWhenAttachedToDestroyed
		SkillStartSFXVolume,
		SkillStartSFXPitch
	);
}


ESwingLR AWeaponBase::CalcSwingLR(const AActor* Target, const FVector& SwingDir) const
{
	if (!Target) return ESwingLR::Unknown;
	if (SwingDir.IsNearlyZero()) return ESwingLR::Unknown;

	const FVector EnemyRight = Target->GetActorRightVector();
	const float Side = FVector::DotProduct(SwingDir, EnemyRight);

	if (FMath::Abs(Side) < MinSideAbsDot)
	{
		return ESwingLR::Unknown;
	}

	// Side>0: 敵の右方向へ動いた = 左→右スイング
	return (Side > 0.f) ? ESwingLR::LeftToRight : ESwingLR::RightToLeft;
}

void AWeaponBase::CheckHit(float DeltaTime)
{
	const FVector CurrentLocation = GetHitPoint();
	const FVector Delta = (CurrentLocation - LastLocation);

	// ほぼ動いてないなら無視
	if (Delta.SizeSquared() < 1.0f)
		return;

	const FVector SwingDir = Delta.GetSafeNormal();

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = true;

	// フレーム間の軌跡を線分判定し、高速移動時のすり抜けを軽減する
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		LastLocation,
		CurrentLocation,
		ECC_Pawn,
		Params
	);

	if (!bHit)
		return;

	AActor* HitActor = Hit.GetActor();
	if (!IsValid(HitActor))
		return;

	// 同一スイング中に同じ敵へ複数回ダメージが入ることを防ぐ
	if (HitActors.Contains(HitActor))
		return;

	// 武器の移動方向と敵方向の内積から、有効な攻撃方向か判定する
	const FVector ToEnemyDir = (HitActor->GetActorLocation() - LastLocation).GetSafeNormal();
	// 内積を利用し、敵に向かって振られた攻撃だけを有効にする
	const float Dot = FVector::DotProduct(SwingDir, ToEnemyDir);

	if (Dot < MinSwingDot)
		return;

	// 有効なヒットとして確定する
	HitActors.Add(HitActor);

	// ヒット音（ImpactPoint優先、ダメなら Location）
	const FVector SfxLoc =
		(!Hit.ImpactPoint.IsNearlyZero()) ? Hit.ImpactPoint :
		(!Hit.Location.IsNearlyZero()) ? Hit.Location :
		HitActor->GetActorLocation();

	PlayHitSFX(SfxLoc);

	//  左→右 / 右→左 判定（敵基準）
	const ESwingLR SwingLR = CalcSwingLR(HitActor, SwingDir);

	// 敵にはスイング方向も渡し、左右別のリアクション再生に利用する
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(HitActor))
	{
		Enemy->ApplySwordHit(Damage, SwingLR, this);
	}
	else
	{
		UGameplayStatics::ApplyDamage(HitActor, Damage, nullptr, this, nullptr);
	}

	const TCHAR* LRText =
		(SwingLR == ESwingLR::LeftToRight) ? TEXT("L->R") :
		(SwingLR == ESwingLR::RightToLeft) ? TEXT("R->L") :
		TEXT("Unknown");

	UE_LOG(LogTemp, Warning,
		TEXT("Valid Slash Hit: %s  Dot=%.2f  LR=%s"),
		*HitActor->GetName(), Dot, LRText);
}
