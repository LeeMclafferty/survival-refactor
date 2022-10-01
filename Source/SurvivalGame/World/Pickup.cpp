// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Pickup.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "Player/SurvivalCharacter.h"
#include "Items/Item.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"

APickup::APickup()
{
	PrimaryActorTick.bCanEverTick = true;
	
	SetReplicates(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	SetRootComponent(PickupMesh);

	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction Component"));
	InteractionComponent->InteractionTime = 0.5f;
	InteractionComponent->InteractionDistance = 200.f;
	InteractionComponent->InteractableNameText = FText::FromString(TEXT("Pickup"));
	InteractionComponent->InteractableActionText = FText::FromString(TEXT("Take"));
	InteractionComponent->OnInteract.AddDynamic(this, &APickup::OnTakePickup);
	InteractionComponent->SetupAttachment(RootComponent);
}

void APickup::InitPickup(const TSubclassOf<class UItem> ItemClass, const int32 Quantity)
{
	if (HasAuthority() && ItemClass && Quantity > 0)
	{
		Item = NewObject<UItem>(this, ItemClass);
		Item->SetQuantity(Quantity);

		OnRep_Item();
		Item->MarkDirtyForReplication();
	}
}

// Called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && ItemTemplate && bNetStartup)
		InitPickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
	
	if (!bNetStartup)
		AlignWithGround();

	if (Item)
		Item->MarkDirtyForReplication();

	
}

void APickup::OnRep_Item()
{
	if (!Item)
		return;

	PickupMesh->SetStaticMesh(Item->PickupMesh);
	InteractionComponent->InteractableNameText = Item->ItemDisplayName;

	Item->OnItemModified.AddDynamic(this, &APickup::OnItemModified);
	
	if(InteractionComponent)
		InteractionComponent->RefreshWidget();
}

void APickup::OnItemModified()
{
	if (InteractionComponent)
		InteractionComponent->RefreshWidget();

}

void APickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickup, Item);
}

bool APickup::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteToActorChannel = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	if (Item && Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
		bWroteToActorChannel = Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);

	return bWroteToActorChannel;
}

#if WITH_EDITOR
void APickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChagnedEvent)
{
	Super::PostEditChangeProperty(PropertyChagnedEvent);

	FName PropertyName = (PropertyChagnedEvent.Property != nullptr) ? PropertyChagnedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(APickup, ItemTemplate) && ItemTemplate)
		PickupMesh->SetStaticMesh(ItemTemplate->PickupMesh);

}
#endif

void APickup::OnTakePickup(ASurvivalCharacter* Taker)
{
	if (!Taker  || IsPendingKillPending() || !Item)
		return;

	const FItemAddResult AddResult = Taker->PlayerInventory->TryAddItem(Item);

	UE_LOG(LogTemp, Warning, TEXT("AddResult: %i"),AddResult.ActualAmountGiven);

	if (AddResult.ActualAmountGiven < Item->GetQuantity())
	{
		Item->SetQuantity(Item->GetQuantity() - AddResult.ActualAmountGiven);
	}
	else if (AddResult.ActualAmountGiven >= Item->GetQuantity())
	{
		Destroy();
	}


}

