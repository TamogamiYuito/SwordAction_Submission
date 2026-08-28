
#include "Actors/VRActor.h"

AVRActor::AVRActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ActorMesh = CreateDefaultSubobject<UStaticMeshComponent>("ActorMesh");
	SetRootComponent(ActorMesh);
}

void AVRActor::BeginPlay()
{
	Super::BeginPlay();
	
}
