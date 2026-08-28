#include "Enemies/EnemyBase.h"

#include "AIController.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 追跡中だけ向きを合わせるので Character 側は ControllerYaw を使う
	bUseControllerRotationYaw = true;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = false;
		Move->RotationRate = FRotator(0.f, 720.f, 0.f);
		Move->MaxWalkSpeed = WalkSpeed_Idle;
	}

	GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	SetAnimState(EEnemyAnimState::Idle);
	ResumeLocomotionAnim();
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead) return;

	UpdateTargetByDistance();

	// -----------------------------
	// Target 無し：待機
	// -----------------------------
	if (!TargetPawn)
	{
		if (AnimState != EEnemyAnimState::Attack && AnimState != EEnemyAnimState::Hit)
		{
			SetAnimState(EEnemyAnimState::Idle);
			ResumeLocomotionAnim();
		}
		return;
	}

	// -----------------------------
	// 見失い
	// -----------------------------
	if (ShouldLoseTarget())
	{
		TargetPawn = nullptr;
		StopAttackLoop();
		StopChaseMove();

		SetAnimState(EEnemyAnimState::Idle);
		ResumeLocomotionAnim();
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	// -----------------------------
	// 攻撃 or 追跡
	// -----------------------------
	if (IsInAttackRange())
	{
		StopChaseMove();
		StartAttackLoop();
	}
	else
	{
		StopAttackLoop();

		// 攻撃後ディレイ中は「追跡に戻らない & 走り出さない」
		// （攻撃後の追跡が速すぎ問題のブレーキ）
		if (Now < ChaseResumeTime)
		{
			StopChaseMove();
			return;
		}

		// 攻撃ワンショット再生中に距離外へ出たら、復帰予約を解除して即追跡ループへ
		if (AnimState == EEnemyAnimState::Attack)
		{
			GetWorld()->GetTimerManager().ClearTimer(ResumeAnimTimerHandle);
			SetAnimState(EEnemyAnimState::Chase);
			ResumeLocomotionAnim();
		}
		else if (AnimState != EEnemyAnimState::Hit)
		{
			// クールダウン中は切替しない（パタパタを防ぐ）
			if (CanTransitionAnim())
			{
				SetAnimState(EEnemyAnimState::Chase);
				ResumeLocomotionAnim();
				LockAnimTransition(AnimTransitionCooldown);
			}
		}

		StartChaseMove();
	}

	// -----------------------------
	// 追跡中だけ向きを合わせる
	// -----------------------------
	if (AnimState == EEnemyAnimState::Chase && Controller)
	{
		FVector ToTarget = TargetPawn->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.IsNearlyZero())
		{
			const FRotator TargetRot = ToTarget.Rotation();
			const FRotator NewRot = FMath::RInterpTo(
				Controller->GetControlRotation(),
				TargetRot,
				DeltaSeconds,
				8.f
			);
			Controller->SetControlRotation(NewRot);
		}
	}
}

// --------------------
// Target / AI
// --------------------
void AEnemyBase::UpdateTargetByDistance()
{
	// ターゲット保持中ならここでは触らない（見失いは別関数）
	if (TargetPawn) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return;

	const float DistSq = FVector::DistSquared(Player->GetActorLocation(), GetActorLocation());
	if (DistSq <= FMath::Square(SearchRadius))
	{
		TargetPawn = Player;

		// 追跡開始時の状態
		if (CanTransitionAnim())
		{
			SetAnimState(EEnemyAnimState::Chase);
			ResumeLocomotionAnim();
			LockAnimTransition(AnimTransitionCooldown);
		}
	}
}

bool AEnemyBase::ShouldLoseTarget() const
{
	if (!TargetPawn) return true;
	return FVector::DistSquared(TargetPawn->GetActorLocation(), GetActorLocation()) >= FMath::Square(LoseTargetDistance);
}

bool AEnemyBase::IsInAttackRange() const
{
	if (!TargetPawn) return false;
	return FVector::DistSquared(TargetPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(AttackRange);
}

void AEnemyBase::StartChaseMove()
{
	if (!TargetPawn) return;

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->MoveToActor(TargetPawn, ChaseAcceptRadius);
	}
}

void AEnemyBase::StopChaseMove()
{
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
	}
}

// --------------------
// Attack
// --------------------
void AEnemyBase::StartAttackLoop()
{
	if (!TargetPawn) return;

	if (GetWorld()->GetTimerManager().IsTimerActive(AttackTimerHandle))
		return;

	AttackOnce();
	GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &AEnemyBase::AttackOnce, AttackInterval, true);
}

void AEnemyBase::StopAttackLoop()
{
	GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
}

void AEnemyBase::AttackOnce()
{
	if (bDead || !TargetPawn) return;
	if (!IsInAttackRange()) return;

	SetAnimState(EEnemyAnimState::Attack);
	PlayOneShotThenResume(AttackAnim, 0.05f);

	const float Now = GetWorld()->GetTimeSeconds();
	ChaseResumeTime = Now + AfterAttackChaseDelay;

	LockAnimTransition(AnimTransitionCooldown);

	// 攻撃アニメーションのヒットタイミングに合わせてダメージを遅延させる
	GetWorld()->GetTimerManager().SetTimer(
		AttackHitTimerHandle,
		this,
		&AEnemyBase::ApplyAttackDamage,
		AttackHitDelay,
		false
	);
}

// --------------------
// Animation / State
// --------------------
bool AEnemyBase::CanTransitionAnim() const
{
	return GetWorld() && (GetWorld()->GetTimeSeconds() >= NextAnimTransitionTime);
}

void AEnemyBase::LockAnimTransition(float Seconds)
{
	if (!GetWorld()) return;
	NextAnimTransitionTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Seconds);
}

void AEnemyBase::SetAnimState(EEnemyAnimState NewState)
{
	AnimState = NewState;
	ApplyMoveSpeedForState();
}

void AEnemyBase::ApplyMoveSpeedForState()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move) return;

	switch (AnimState)
	{
	case EEnemyAnimState::Idle:   Move->MaxWalkSpeed = WalkSpeed_Idle;   break;
	case EEnemyAnimState::Chase:  Move->MaxWalkSpeed = WalkSpeed_Chase;  break;
	case EEnemyAnimState::Attack: Move->MaxWalkSpeed = WalkSpeed_Attack; break;
	case EEnemyAnimState::Hit:    Move->MaxWalkSpeed = WalkSpeed_Hit;    break;
	default: break;
	}
}

void AEnemyBase::PlayLoopIfChanged(UAnimSequenceBase* Seq)
{
	if (!Seq || !GetMesh()) return;
	if (CurrentLoopAnim == Seq) return;

	CurrentLoopAnim = Seq;

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->SetAnimation(Seq);
	GetMesh()->Play(true);
}

void AEnemyBase::PlayOneShotThenResume(UAnimSequenceBase* Seq, float MinHoldSeconds)
{
	if (!Seq || !GetMesh()) return;

	CurrentLoopAnim = nullptr;

	// 連続ヒット等で復帰予約が残っていたら解除
	GetWorld()->GetTimerManager().ClearTimer(ResumeAnimTimerHandle);

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->SetAnimation(Seq);
	GetMesh()->Play(false);

	const float Len = Seq->GetPlayLength();
	const float Delay = FMath::Max(Len, MinHoldSeconds);

	GetWorld()->GetTimerManager().SetTimer(
		ResumeAnimTimerHandle,
		this,
		&AEnemyBase::ResumeLocomotionAnim,
		Delay,
		false
	);
}

void AEnemyBase::ResumeLocomotionAnim()
{
	if (bDead) return;

	// 状況に応じてループを戻す
	if (TargetPawn && !IsInAttackRange())
	{
		SetAnimState(EEnemyAnimState::Chase);
		PlayLoopIfChanged(ChaseAnim ? ChaseAnim : IdleAnim);
	}
	else
	{
		SetAnimState(EEnemyAnimState::Idle);
		PlayLoopIfChanged(IdleAnim ? IdleAnim : ChaseAnim);
	}
}

// --------------------
// Damage
// --------------------
float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		Die();
		return DamageAmount;
	}

	// 汎用ダメージは左右情報が無いので適当に片方
	SetAnimState(EEnemyAnimState::Hit);
	PlayOneShotThenResume(HitAnim_Left ? HitAnim_Left : HitAnim_Right, 0.05f);
	LockAnimTransition(AnimTransitionCooldown);

	return DamageAmount;
}

void AEnemyBase::ApplySwordHit(float DamageAmount, ESwingLR SwingDir, AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f) return;

	CurrentHealth -= DamageAmount;

	if (CurrentHealth <= 0.f)
	{
		Die();
		return;
	}

	SetAnimState(EEnemyAnimState::Hit);

	UAnimSequenceBase* HitSeq = nullptr;
	if (SwingDir == ESwingLR::LeftToRight)       HitSeq = HitAnim_Left;
	else if (SwingDir == ESwingLR::RightToLeft) HitSeq = HitAnim_Right;
	else                                        HitSeq = (HitAnim_Left ? HitAnim_Left : HitAnim_Right);

	PlayOneShotThenResume(HitSeq, 0.05f);
	LockAnimTransition(AnimTransitionCooldown);
}

// --------------------
// Death
// --------------------
void AEnemyBase::Die()
{
	if (bDead) return;
	bDead = true;

	SetAnimState(EEnemyAnimState::Dead);

	StopAttackLoop();
	StopChaseMove();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
	}

	// 死亡アニメ
	if (DeathAnim && GetMesh())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->SetAnimation(DeathAnim);
		GetMesh()->Play(false);
	}

	float Delay = DestroyDelay;
	if (DeathAnim)
	{
		Delay = FMath::Max(Delay, DeathAnim->GetPlayLength());
	}

	GetWorld()->GetTimerManager().SetTimer(DestroyTimerHandle, this, &AEnemyBase::DestroySelf, Delay, false);
}

void AEnemyBase::DestroySelf()
{
	Destroy();
}

void AEnemyBase::ApplyAttackDamage()
{
	if (bDead || !TargetPawn) return;

	// ダメージ適用時にも攻撃範囲内か再確認する
	if (!IsInAttackRange()) return;

	UGameplayStatics::ApplyDamage(
		TargetPawn,
		AttackDamage,
		GetController(),
		this,
		nullptr
	);
}
