// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

// Called to update the UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemsAdded UMETA(DisplayName = "No items added"),
	IAR_SomeItemsAdded UMETA(DisplayName = "Some items add"),
	IAR_AllItemsAdded UMETA(DisplayName = "All items Added")
};

USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()

public:

	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), ActualAmountGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), ActualAmountGiven(InQuantityAdded) {};

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountToGive;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 ActualAmountGiven;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult Result;

	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ErrorText;

	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	static FItemAddResult AddedSome(const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText)
	{
		FItemAddResult AddSomeResult(InItemQuantity, ActualAmountGiven);
		AddSomeResult.Result = EItemAddResult::IAR_SomeItemsAdded;
		AddSomeResult.ErrorText = ErrorText;

		return AddSomeResult;
	}

	static FItemAddResult AddedAll(const int32 InItemQuantity)
	{
		FItemAddResult AddAllResult(InItemQuantity, InItemQuantity);
		AddAllResult.Result = EItemAddResult::IAR_AllItemsAdded;

		return AddAllResult;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UItem;

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItem(class UItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItemOfClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity);

	int32 ConsumeItem(class UItem* Item);
	int32 ConsumeItem(class UItem* Item, const int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(class UItem* Item);
	

	/* Item navigation */
	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	bool HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity = 1) const;

	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	UItem* FindItem(class UItem* Item) const;

	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	UItem* FindItemByClass(TSubclassOf<class UItem> ItemClass) const;

	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	TArray<UItem*>FindItemsByClass(TSubclassOf<class UItem> ItemClass) const;
	/* Item navigation */


	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	float GetCurrentWeight() const;

	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	void SetWeightCapacity(const float NewWeightCapacity);

	UFUNCTION(BlueprintCallable, Category = "InventoryNavigation")
	void SetCapacity(const int32 NewCapacity);
	

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE int32 GetCapacity() const { return Capacity;  }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE TArray<class UItem*> GetItems() const { return Items; }

	// Server does not need to refresh ui, it just stores the data.
	UFUNCTION(Client, Reliable)
	void ClientRefreshInventory();

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

protected:
	// Called when the game starts 
	virtual void BeginPlay() override;	

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	float WeightCapacity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0, ClampMax = 200))
	int32 Capacity;

	UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, Category = "Inventory")
	TArray<class UItem*> Items; 

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

private:

	UFUNCTION()
	void OnRep_Items();

	UPROPERTY()
	int32 ReplicatedItemsKey;

	FItemAddResult TryAddItem_Internal(class UItem* Item);

	// Do not call Items.Add() directly. This function handles replication.
	UItem* AddItem(class UItem* Item);

};
