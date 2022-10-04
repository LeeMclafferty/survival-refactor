// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SurvivalCharacter.generated.h"

USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

public:
	FInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.f;
		bInteractHeld = false;
	}

	UPROPERTY()
	class UInteractionComponent* ViewedInteractionComponent;

	UPROPERTY()
	float LastInteractionCheckTime;

	UPROPERTY()
	bool bInteractHeld;
};

UCLASS()
class SURVIVALGAME_API ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASurvivalCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* PlayerInventory;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UCameraComponent* CameraComponent;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HelmetMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* ChestMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* LegsMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* FeetMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* VestMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HandsMesh;
	
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* BackpackMesh;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionCheckDistance;

	void PerformInteractionCheck();

	void CouldntFindInteractable();
	void FoundInteractable(class UInteractionComponent* Interactable);

	void BeginInteract();
	void EndInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginInteract();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndInteract();

	void Interact();

	UPROPERTY()
	FInteractionData InteractionData;

	FORCEINLINE class UInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent;  }

	FTimerHandle TimerHandle_Interact;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	float CarryWeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Capaciy;
public:

	bool IsInteracting() const;

	float GetRemainingInerteractTime() const;

	UFUNCTION(BlueprintCallable, Category = "Items")
	void UseItem(class UItem* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseitem(class UItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(class UItem* Item, const int32 Quantity);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropItem(class UItem* Item, const int32 Quantity);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class APickup> PickupClass;

protected:
	void MoveForward(float Val);
	void MoveRight(float Val);

	void LookUp(float Val);
	void Turn(float Val);

	void StartCrouching();
	void StopCrouching();

public:	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


private:

	void SetupComps();

	void DrawLookDebug();
};
