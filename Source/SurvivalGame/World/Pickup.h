// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class SURVIVALGAME_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();

	// non-replicated template that will be used to construced a replicated item.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	class UItem* ItemTemplate;

	void InitPickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity);

	UFUNCTION(BlueprintImplementableEvent)
	void AlignWithGround();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, ReplicatedUsing = OnRep_Item)
	class UItem* Item;

	UFUNCTION()
	void OnRep_Item();

	UFUNCTION()
	void OnItemModified();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChagnedEvent) override;
#endif

	UFUNCTION()
	void OnTakePickup(class ASurvivalCharacter* Taker);

	UPROPERTY(EditAnywhere, Category = "Components")
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UInteractionComponent* InteractionComponent;
};
