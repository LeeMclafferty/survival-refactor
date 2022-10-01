// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_Common UMETA(DisplayName = "Common"),
	IR_Uncommon UMETA(DisplayName = "Uncommon"),
	IR_Rare UMETA(DisplayName = "Rare"),
	IR_VeryRare UMETA(DisplayName = "Very Rare"),
	IR_Legendary UMETA(DisplayName = "Legendary")
};

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SURVIVALGAME_API UItem : public UObject
{

	GENERATED_BODY()

protected:

	virtual void GetLifetimeReplicatedProps(TArray< class FLifetimeProperty >& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:

	UItem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText UseActionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UTexture2D* Thumbnail;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UStaticMesh* PickupMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float Weight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bIsStackable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	int32 MaxStackSize;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TSubclassOf<class UItemToolTip> ItemToolTip;

	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bIsStackable))
	int32 Quantity;

	UPROPERTY()
	class UInventoryComponent* OwningInventory;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; }

	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const; 

	UPROPERTY()
	int32 RepKey;

	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

	virtual void Use(class ASurvivalCharacter* Character);
	virtual void AddToInventory(class UInventoryComponent* Inventory);

	void SetQuantity(const int32 NewQuantity);

	void MarkDirtyForReplication();

	UFUNCTION()
	void OnRep_Quantity();

	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; }
	
};
