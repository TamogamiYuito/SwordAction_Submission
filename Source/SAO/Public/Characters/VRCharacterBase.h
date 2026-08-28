#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacterBase.generated.h"

class UCameraComponent;
class USphereComponent;
class AWeaponBase;
struct FInputActionValue;

UCLASS()
class SAO_API AVRCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AVRCharacterBase();

public:
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bIsVRMode = false;

	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bLook = true;

	UCameraComponent* GetCamera() const { return Camera; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Origin;

	// =========================
	// SkillBox（プレイヤー固定）
	// =========================
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<USphereComponent> SkillBox;

	// 箱内に入っている武器（弱参照）
	// 武器が破棄された場合に無効参照を保持しないよう弱参照で管理する
	UPROPERTY()
	TWeakObjectPtr<AWeaponBase> WeaponInBox;


public:
	// 現在保持している武器
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<AWeaponBase> CurrentWeapon;

	// 入力イベントから呼び出し、剣スキルの発動条件を確認する
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void TryTriggerSwordSkill();

	// Blueprintから現在の武器を設定する
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetCurrentWeapon(AWeaponBase* NewWeapon);

	// 武器の範囲内状態をBlueprintへ通知する
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill")
	void OnWeaponInBoxChanged(bool bHasWeaponInBox);

	// Movement / Look
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void CharacterMove(const FVector2D& MoveInput);

	UFUNCTION(BlueprintCallable, Category = "Look")
	void CharacterLook(const FVector2D& LookInput);


private:
	UFUNCTION()
	void OnSkillBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSkillBoxEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);


protected:

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Status")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Status")
	float CurrentHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Player|Status")
	bool bIsDead = false;

private:
	void GameOver();
};
