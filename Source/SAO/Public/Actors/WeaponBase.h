#pragma once

#include "CoreMinimal.h"
#include "Actors/VRGrabbaleActor.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "WeaponBase.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
enum class ESwingLR : uint8; 

UCLASS()
class SAO_API AWeaponBase : public AVRGrabbaleActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Effect")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> HitBox;

	//  武器先端に近いHitBox位置を基準にスイングを判定する
	FVector LastLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float Damage = 20.f;

	// 1回のスイング中にヒットしたActorを記録し、多段ヒットを防止する
	//UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> HitActors;

	bool bIsSwinging = false;

	// 開始と終了で異なる閾値を使用し、境界付近での状態切替を安定させる
	UPROPERTY(EditAnywhere, Category = "Weapon|Swing")
	float SwingStartSpeed = 50.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Swing")
	float SwingEndSpeed = 20.f;

	// 武器が敵方向へ振られているかを判定する内積の閾値（0.5で約60度以内）
	UPROPERTY(EditAnywhere, Category = "Weapon|Swing")
	float MinSwingDot = 0.5f;

	// 横方向成分が小さい場合は左右方向を確定しない
	UPROPERTY(EditAnywhere, Category = "Weapon|Swing")
	float MinSideAbsDot = 0.2f;

	UNiagaraSystem* SpawnNiagaraSystem = nullptr;

private:
	void CheckHit(float DeltaTime);
	void UpdateSwingState(float DeltaTime);

	//  敵の右方向を基準に、スイングの左右方向を計算する
	ESwingLR CalcSwingLR(const AActor* Target, const FVector& SwingDir) const;

	// HitBox基準の現在位置
	FVector GetHitPoint() const;

	void PlayHitSFX(const FVector& Location) const;


public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill")
	void BP_OnSwordSkillTriggered(); // Blueprint側でスキル演出を実装する


	// ヒット時に再生するサウンド
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	TObjectPtr<USoundBase> HitSFX;

	// ヒット音の音量・ピッチ（調整用）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	float HitSFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX")
	float HitSFXPitch = 1.0f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX|Skill")
	TObjectPtr<USoundBase> SkillStartSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX|Skill")
	float SkillStartSFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SFX|Skill")
	float SkillStartSFXPitch = 1.0f;

	// 連打で重なるのを防ぐ
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> SkillStartAudioComp;

	UFUNCTION(BlueprintCallable, Category = "SFX|Skill")
	void PlaySkillStartSFX_Attached(bool bStopIfPlaying = true);

};
