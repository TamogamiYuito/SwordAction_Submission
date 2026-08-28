#include "Characters/VRCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Actors/VRGrabbaleActor.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Actors/WeaponBase.h"

AVRCharacterBase::AVRCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Origin = CreateDefaultSubobject<USceneComponent>(TEXT("Origin"));
	Origin->SetupAttachment(GetCapsuleComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Origin);


	// SkillBox（プレイヤー固定）
	SkillBox = CreateDefaultSubobject<USphereComponent>(TEXT("SkillBox"));
	SkillBox->SetupAttachment(GetCapsuleComponent());
	SkillBox->SetSphereRadius(120.f);

	SkillBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SkillBox->SetCollisionObjectType(ECC_WorldDynamic);
	SkillBox->SetCollisionResponseToAllChannels(ECR_Ignore);

	// WorldDynamicとして設定された武器とのオーバーラップを検知する
	SkillBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	SkillBox->SetGenerateOverlapEvents(true);

	CurrentWeapon = nullptr;
	WeaponInBox = nullptr;
}

void AVRCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;

	Camera->PostProcessSettings.bOverride_ColorSaturation = true;
	Camera->PostProcessSettings.bOverride_ColorGain = true;


	if (SkillBox)
	{
		SkillBox->OnComponentBeginOverlap.AddDynamic(this, &AVRCharacterBase::OnSkillBoxBeginOverlap);
		SkillBox->OnComponentEndOverlap.AddDynamic(this, &AVRCharacterBase::OnSkillBoxEndOverlap);
	}
}

void AVRCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GEngine->ForceGarbageCollection(true);
}

void AVRCharacterBase::CharacterMove(const FVector2D& MoveInput)
{
	if (MoveInput.IsNearlyZero()) return;

	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MoveInput.Y);
	AddMovementInput(Right, MoveInput.X);
}

void AVRCharacterBase::CharacterLook(const FVector2D& LookInput)
{
	if (!bLook) return;

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AVRCharacterBase::SetCurrentWeapon(AWeaponBase* NewWeapon)
{
	CurrentWeapon = NewWeapon;
}

void AVRCharacterBase::OnSkillBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AWeaponBase* Weapon = Cast<AWeaponBase>(OtherActor);
	if (!IsValid(Weapon)) return;

	WeaponInBox = Weapon;

	// UI用
	OnWeaponInBoxChanged(true);
}

void AVRCharacterBase::OnSkillBoxEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{

	AWeaponBase* Weapon = Cast<AWeaponBase>(OtherActor);
	if (!IsValid(Weapon)) return;

	// 今保持してる武器が出た場合だけクリア
	if (WeaponInBox.Get() == Weapon)
	{
		WeaponInBox = nullptr;
		OnWeaponInBoxChanged(false);
	}
}

void AVRCharacterBase::TryTriggerSwordSkill()
{
	AWeaponBase* InBox = WeaponInBox.Get();
	if (!IsValid(InBox)) return;

	AVRGrabbaleActor* Grabbable = Cast<AVRGrabbaleActor>(InBox);
	if (!IsValid(Grabbable) || !Grabbable->IsHeld())
		return;

	InBox->BP_OnSwordSkillTriggered();
}


float AVRCharacterBase::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead) return 0.f;
	if (DamageAmount <= 0.f) return 0.f;

	CurrentHealth -= DamageAmount;

	// 0未満にしない
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);


	float HealthRatio = CurrentHealth / MaxHealth;

	// 0HPに近づくほど赤くする
	float DamageIntensity = 1.f - HealthRatio;

	// 徐々に強くしたいなら Clamp
	DamageIntensity = FMath::Clamp(DamageIntensity, 0.f, 1.f);

	// 赤みを増やす（Rだけ強める）
	Camera->PostProcessSettings.ColorGain = FVector4(
		1.f + DamageIntensity * 1.5f,  // R
		1.f - DamageIntensity * 0.5f,  // G
		1.f - DamageIntensity * 0.5f,  // B
		1.f
	);


	// 必要に応じてBlueprintイベントからUIを更新できる

	if (CurrentHealth <= 0.f)
	{
		GameOver();
	}


	return DamageAmount;
}

void AVRCharacterBase::GameOver()
{
	if (bIsDead) return;
	bIsDead = true;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;


	// 入力停止
	DisableInput(PC);

	// 移動停止
	GetCharacterMovement()->DisableMovement();

	// 敵からの追加ダメージを防ぐ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 軽いスロー演出（VRでも酔いにくい）
	UGameplayStatics::SetGlobalTimeDilation(this, 0.2f);

	// ゲームオーバーUIはBlueprint側で表示する
}
