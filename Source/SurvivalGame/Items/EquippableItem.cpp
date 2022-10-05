// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/EquippableItem.h"
#include "Net/UnrealNetwork.h"

#include "Player/SurvivalCharacter.h"
#include "Components/InventoryComponent.h"

#define LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem()
	:bIsEquipped(false)
{
	bIsStackable = false;
	UseActionText= LOCTEXT("ItemUseActionText", "Equip");
}

void UEquippableItem::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippableItem, bIsEquipped);
}

void UEquippableItem::Use(ASurvivalCharacter* Character)
{
	if ( !Character || !Character->HasAuthority())
		return;

	if (Character->GetEquippedItems().Contains(Slot) && !bIsEquipped)
	{
		UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot);
		AlreadyEquippedItem->SetEquipped(false);
	}

	SetEquipped(!IsEquipped());

}

bool UEquippableItem::Equip(ASurvivalCharacter* Character)
{
	if(!Character)
		return false;

	return Character->EquipItem(this);
}

bool UEquippableItem::Unequip(ASurvivalCharacter* Character)
{
	if (!Character)
		return false;

	return Character->UnequipItem(this);
}

bool UEquippableItem::ShouldShowInInventory() const
{
	return !bIsEquipped;
}

void UEquippableItem::SetEquipped(bool bNewEquipped)
{
	bIsEquipped = bNewEquipped;
	EquipStatusChanged();
	MarkDirtyForReplication();
}

void UEquippableItem::EquipStatusChanged()
{
	if (ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(GetOuter()))
	{
		if (bIsEquipped)
		{
			Equip(Character);
		}
		else
		{
			Unequip(Character);
		}
	}

	OnItemModified.Broadcast();
}

#undef LOCTEXT_NAMESPACE