#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

// Unreal Engineのインターフェース型として使用する。
UINTERFACE(MinimalAPI)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SAO_API IInteractInterface
{
	GENERATED_BODY()

	// 掴めるオブジェクトが実装する共通操作を定義する。
public:

	virtual void OnGrab(USkeletalMeshComponent* InComponent, const FVector& GrabLocation) = 0;
	virtual void OnRelease(USkeletalMeshComponent* InComponent) = 0;

};
