// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Item.h"
#include "Net/UnrealNetwork.h"

#include "Components/InventoryComponent.h"

#define LOCTEXT_NAMESPACE "Item"

UItem::UItem()
	:ItemDisplayName(LOCTEXT("ItemName", "Item")),
	UseActionText(LOCTEXT("ItemUseActionText", "Use")),
	Weight(0.f),
	bIsStackable(true),
	MaxStackSize(2),
	Quantity(1),
	RepKey(0)
{

}

bool UItem::ShouldShowInInventory() const
{
	return true;
}

void UItem::Use(ASurvivalCharacter* Character)
{
}

void UItem::AddToInventory(UInventoryComponent* Inventory)
{
}

void UItem::SetQuantity(const int32 NewQuantity)
{
	if (NewQuantity == Quantity)
		return;
		
	Quantity = FMath::Clamp(NewQuantity, 0, bIsStackable ? MaxStackSize : 1);
	MarkDirtyForReplication();
}

void UItem::MarkDirtyForReplication()
{
	RepKey++;

	if (OwningInventory)
		++OwningInventory->ReplicatedItemsKey;
}

void UItem::OnRep_Quantity()
{
	OnItemModified.Broadcast();
}

void UItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItem, Quantity);
}

bool UItem::IsSupportedForNetworking() const
{
	return true;
}

#if WITH_EDITOR
void UItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UItem, Quantity))
	{
		Quantity = FMath::Clamp(Quantity, 1, bIsStackable ? MaxStackSize : 1);
	}
		
}
#endif


#undef LOCTEXT_NAMESPACE