// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/GearItem.h"

#include "Player/SurvivalCharacter.h"

UGearItem::UGearItem()
	:DamageReduction(0.1f)
{
}

bool UGearItem::Equip(ASurvivalCharacter* Character)
{
	bool EquipSuccessful = Super::Equip(Character);

	if (EquipSuccessful && Character)
	{
		Character->EquipGear(this);
	}

	return EquipSuccessful;
}

bool UGearItem::Unequip(ASurvivalCharacter* Character)
{
	bool UnequipSuccessful = Super::Unequip(Character);

	if (UnequipSuccessful && Character)
	{
		Character->UnequipGear(this->Slot);
	}

	return UnequipSuccessful;
}
