// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "Items/Item.h"

#define LOCTEXT_NAMESPACE "Inventory"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	SetIsReplicated(true);
}

FItemAddResult UInventoryComponent::TryAddItem(UItem* Item)
{
	return TryAddItem_Internal(Item);
}

FItemAddResult UInventoryComponent::TryAddItemOfClass(TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	UItem* Item = NewObject<UItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);

	return TryAddItem_Internal(Item);
}

int32 UInventoryComponent::ConsumeItem(UItem* Item)
{
	if (!Item)
		return 0;

	return ConsumeItem(Item, Item->Quantity);
}

int32 UInventoryComponent::ConsumeItem(UItem* Item, const int32 Quantity)
{
	if (!GetOwner()->HasAuthority() || !Item)
		return 0;

	const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

	ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

	Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

	if (Item->GetQuantity() <= 0)
	{
		RemoveItem(Item);
	}
	else
	{
		ClientRefreshInventory();
	}

	return RemoveQuantity;
}

bool UInventoryComponent::RemoveItem(class UItem* Item)
{
	if (!GetOwner()->HasAuthority() || !Item)
		return false;

	Items.RemoveSingle(Item);
	ReplicatedItemsKey++;

	return true;
}

bool UInventoryComponent::HasItem(TSubclassOf<class UItem> ItemClass, const int32 Quantity) const
{

	if (UItem* ItemToFind = FindItemByClass(ItemClass))
	{
		return ItemToFind->GetQuantity() >= Quantity;
	}

	return false;
}

UItem* UInventoryComponent::FindItem(UItem* Item) const
{
	if (!Item)
		return nullptr;

	for (auto& InventoryItem : Items)
	{
		if (InventoryItem && InventoryItem->GetClass() == Item->GetClass())
		{
			return InventoryItem;
		}
	}

	return nullptr;
}

UItem* UInventoryComponent::FindItemByClass(TSubclassOf<class UItem> ItemClass) const
{

	for (auto& InventoryItem : Items)
	{
		if (InventoryItem && InventoryItem->GetClass() == ItemClass)
		{
			return InventoryItem;
		}
	}
	return nullptr;
}

TArray<UItem*> UInventoryComponent::FindItemsByClass(TSubclassOf<class UItem> ItemClass) const
{
	TArray<UItem*> ItemsOfClass;

	for (auto& InventoryItem : Items)
	{
		if (InventoryItem && InventoryItem->GetClass()->IsChildOf(ItemClass))
		{
			ItemsOfClass.Add(InventoryItem);
		}
	}

	return ItemsOfClass;
}

float UInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : Items)
	{
		if (Item)
		{
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity)
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetCapacity(const int32 NewCapacity)
{
	Capacity = NewCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::ClientRefreshInventory_Implementation()
{
	OnInventoryUpdated.Broadcast();
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Items);
}

bool UInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteToActorChannel = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (!Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
		return false;

	for (auto& Item : Items)
	{
		if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
		{
			bWroteToActorChannel = Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
		}
	}


	return bWroteToActorChannel;
}

void UInventoryComponent::OnRep_Items()
{
	OnInventoryUpdated.Broadcast();
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(UItem* Item)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("202"));
		return FItemAddResult::AddedNone(-1, LOCTEXT("IsNotServerText", "Clients cannot add items."));
	}
	
	const int32 AddAmount = Item->GetQuantity();

	if (Items.Num() + 1 > GetCapacity())
	{
		UE_LOG(LogTemp, Warning, TEXT("210"));
		return FItemAddResult::AddedNone(0, LOCTEXT("InventoryCapacityFullText", "Inventory is full."));
	}

	if (!FMath::IsNearlyZero(Item->Weight))
	{
		if (GetCurrentWeight() + Item->Weight > GetWeightCapacity())
		{
			UE_LOG(LogTemp, Warning, TEXT("218"));
			return FItemAddResult::AddedNone(0, LOCTEXT("InventoryTooMuchWeightText", "Carrying Too Much Weight."));
		}
	}

	if (Item->bIsStackable)
	{
		ensure(Item->GetQuantity() <= Item->MaxStackSize);

		if (UItem* ExistingItem = FindItem(Item))
		{
			if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize)
			{
				const int32 CapacityMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();
				int32 ActualAddAmount = FMath::Min(AddAmount, CapacityMaxAddAmount);

				FText ErrorText = LOCTEXT("InventoryErrorText", "Couldn't add all items to inventory.");

				if (FMath::IsNearlyZero(Item->Weight))
				{
					const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);
					ActualAddAmount = FMath::Min(ActualAddAmount, WeightMaxAddAmount);

					if (ActualAddAmount < AddAmount)
					{
						ErrorText = FText::Format(LOCTEXT("InventoryTooMuchWeightText", "Too much weight, couldn't add all {ItemName} to inventory"), Item->ItemDisplayName);
					}
				}
				else if (ActualAddAmount < AddAmount)
				{
					ErrorText = FText::Format(LOCTEXT("InventoryCapacityFullText", "Inventory is full, couldn't add all {ItemName} to inventory"), Item->ItemDisplayName);
				}
				
				if (ActualAddAmount <= 0)
				{
					UE_LOG(LogTemp, Warning, TEXT("246"));
					return FItemAddResult::AddedNone(0, LOCTEXT("InventoryErrorText", "Unable to add item to inventory."));
				}

				ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount);

				ensure(ExistingItem->GetQuantity() <= ExistingItem->MaxStackSize);

				if (ActualAddAmount < AddAmount)
				{
					UE_LOG(LogTemp, Warning, TEXT("253"));
					return FItemAddResult::AddedSome(AddAmount, ActualAddAmount, ErrorText);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("257"));
					return FItemAddResult::AddedAll(AddAmount);
				}
			}
			else
			{ 
				UE_LOG(LogTemp, Warning, TEXT("262"));
				return FItemAddResult::AddedNone(AddAmount, FText::Format(LOCTEXT("InventoryFullStackText", "{ItemName}'s stack is already full."), Item->ItemDisplayName));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("267"));
			AddItem(Item);
			return FItemAddResult::AddedAll(AddAmount);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NotStackable"));
		ensure(Item->GetQuantity() == 1);

		AddItem(Item);

		return FItemAddResult::AddedAll(AddAmount);
	}
} 

UItem* UInventoryComponent::AddItem(UItem* Item)
{
	if(!GetOwner() || !GetOwner()->HasAuthority())
		return nullptr;

	// Reconstruct object so that this inventory component is guarenteed to be the owner.
	UItem* NewItem = NewObject<UItem>(GetOwner(), Item->GetClass());
	NewItem->SetQuantity(Item->GetQuantity());
	NewItem->OwningInventory = this;
	NewItem->AddToInventory(this);

	Items.Add(NewItem);
	NewItem->MarkDirtyForReplication();

	return NewItem;
}

#undef LOCTEXT_NAMESPACE