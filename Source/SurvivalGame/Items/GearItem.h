// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/EquippableItem.h"
#include "GearItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class SURVIVALGAME_API UGearItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UGearItem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class USkeletalMesh* Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class UMaterialInstance* MaterialInstance;

	virtual bool Equip(class ASurvivalCharacter* Character) override;
	virtual bool Unequip(class ASurvivalCharacter* Character) override;

	//DamageDefenseMultiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gear", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float DamageReduction;
	
};
