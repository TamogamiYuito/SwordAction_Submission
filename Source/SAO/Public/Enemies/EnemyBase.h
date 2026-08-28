#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class UAnimSequenceBase;

UENUM(BlueprintType)
enum class ESwingLR : uint8
{
	LeftToRight,
	RightToLeft,
	Unknown
};

UENUM(BlueprintType)
enum class EEnemyAnimState : uint8
{
	Idle,
	Chase,
	Attack,
	Hit,
	Dead
};

UCLASS()
class SAO_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 飛び道具など、攻撃方向を持たない汎用ダメージ用
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

public:
	// 武器からスイング方向付きでダメージを適用する
	UFUNCTION(BlueprintCallable, Category = "Enemy|Damage")
	void ApplySwordHit(float DamageAmount, ESwingLR SwingDir, AActor* DamageCauser);

protected:
	// --------------------
	// Status
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Status")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Status")
	float CurrentHealth = 0.f;

	// --------------------
	// Search / AI
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float SearchRadius = 1500.f;     // cm（半径）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float LoseTargetDistance = 2500.f; // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float AttackRange = 150.f;       // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float ChaseAcceptRadius = 80.f;  // cm

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float AttackInterval = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float AttackDamage = 10.f;

	// 攻撃開始からヒットまでの時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Attack")
	float AttackHitDelay = 0.35f;

	FTimerHandle AttackHitTimerHandle;

	// 実際にダメージを入れる関数
	void ApplyAttackDamage();

	// --------------------
	// Timing（ここがタイミング制御）
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Timing")
	float AfterAttackChaseDelay = 0.35f; // 攻撃後、追跡に戻るまでの待ち

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Timing")
	float AnimTransitionCooldown = 0.15f; // 状態/アニメの切替連打を抑える

	// --------------------
	// Move Speed（状態別）
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
	float WalkSpeed_Idle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
	float WalkSpeed_Chase = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
	float WalkSpeed_Attack = 0.f; // 攻撃中は止めたいなら0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Move")
	float WalkSpeed_Hit = 0.f;

	// --------------------
	// Animation（BPでAnimSequence設定）
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Locomotion")
	TObjectPtr<UAnimSequenceBase> IdleAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Locomotion")
	TObjectPtr<UAnimSequenceBase> ChaseAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Combat")
	TObjectPtr<UAnimSequenceBase> AttackAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Hit")
	TObjectPtr<UAnimSequenceBase> HitAnim_Left = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Hit")
	TObjectPtr<UAnimSequenceBase> HitAnim_Right = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Death")
	TObjectPtr<UAnimSequenceBase> DeathAnim = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation|Death")
	float DestroyDelay = 2.0f;

private:
	// --------------------
	// Runtime
	// --------------------
	bool bDead = false;

	UPROPERTY()
	TObjectPtr<APawn> TargetPawn = nullptr;

	EEnemyAnimState AnimState = EEnemyAnimState::Idle;

	// ループアニメは同じのを毎Tickセットしない
	UPROPERTY()
	TObjectPtr<UAnimSequenceBase> CurrentLoopAnim = nullptr;

	FTimerHandle AttackTimerHandle;
	FTimerHandle ResumeAnimTimerHandle;
	FTimerHandle DestroyTimerHandle;

	// 「攻撃後しばらく追跡へ戻さない」ための時刻
	float ChaseResumeTime = 0.f;

	// 状態/アニメ切替のクールダウン時刻
	float NextAnimTransitionTime = 0.f;

private:
	// --------------------
	// AI helpers
	// --------------------
	void UpdateTargetByDistance();
	bool ShouldLoseTarget() const;
	bool IsInAttackRange() const;

	void StartChaseMove();
	void StopChaseMove();

	void StartAttackLoop();
	void StopAttackLoop();
	void AttackOnce();

	// --------------------
	// Animation helpers
	// --------------------
	bool CanTransitionAnim() const;
	void LockAnimTransition(float Seconds);

	void SetAnimState(EEnemyAnimState NewState);
	void ApplyMoveSpeedForState();

	void PlayLoopIfChanged(UAnimSequenceBase* Seq);
	void PlayOneShotThenResume(UAnimSequenceBase* Seq, float MinHoldSeconds = 0.f);
	void ResumeLocomotionAnim();

	// --------------------
	// Death
	// --------------------
	void Die();
	void DestroySelf();
};
