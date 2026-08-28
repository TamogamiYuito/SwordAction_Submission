#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRActor.generated.h"

UCLASS()
class SAO_API AVRActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVRActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> ActorMesh;

public:	

};
