
#include "Player/SurvivalCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Containers/Map.h"
#include "Materials/MaterialInstance.h"

#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "World/Pickup.h"
#include "Items/EquippableItem.h"
#include "Items/GearItem.h"


// Sets default values
ASurvivalCharacter::ASurvivalCharacter()
	: InteractionCheckFrequency(0.f), InteractionCheckDistance(450.f)
{
	PrimaryActorTick.bCanEverTick = true;

	SetupComps();
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	CarryWeight = 80.f;
	Capaciy = 20;
}

// Called when the game starts or when spawned
void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();

	// I might need to come back and as a conditional here to only do this on the first spawn in.
	for (auto& PlayerMesh : PlayerMeshes)
	{
		BareMesh.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
	
}

bool ASurvivalCharacter::IsInteracting() const
{
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}

float ASurvivalCharacter::GetRemainingInerteractTime() const
{
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}

// USE
void ASurvivalCharacter::UseItem(UItem* Item)
{
	if (!HasAuthority() && Item)
		ServerUseitem(Item);

	if (PlayerInventory && !PlayerInventory->FindItem(Item))
		return;

	if (Item)
	{
		Item->Use(this);
	}
}

void ASurvivalCharacter::ServerUseitem_Implementation(UItem* Item)
{
	UseItem(Item);
}

bool ASurvivalCharacter::ServerUseitem_Validate(UItem* Item)
{
	return true;
}

// Drop
void ASurvivalCharacter::DropItem(UItem* Item, const int32 Quantity)
{
	if (!PlayerInventory || !Item || !PlayerInventory->FindItem(Item))
		return;

	if (!HasAuthority())
	{
		ServerDropItem(Item, Quantity);
		return;
	}

	const int32 ItemQuantity = Item->GetQuantity();
	const int32 DroppedQuantity = PlayerInventory->ConsumeItem(Item, Quantity);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.bNoFail = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector SpawnLocation = GetActorLocation();
	SpawnLocation.Z = GetCapsuleComponent()->GetScaledCapsuleHalfHeight(); // Move to feet

	FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

	ensure(PickupClass);

	APickup* Pickup = GetWorld()->SpawnActor<APickup>(PickupClass, SpawnTransform, SpawnParams);
	Pickup->InitPickup(Item->GetClass(), DroppedQuantity);
	
}

bool ASurvivalCharacter::EquipItem(UEquippableItem* Item)
{
	EquippedItems.Add(Item->Slot, Item);
	OnEquippedItemsChanged.Broadcast(Item->Slot, Item);
	return true;
}

bool ASurvivalCharacter::UnequipItem(UEquippableItem* Item)
{
	if(!Item || !EquippedItems.Contains(Item->Slot) || Item != *EquippedItems.Find(Item->Slot))
		return false;

	EquippedItems.Remove(Item->Slot);
	OnEquippedItemsChanged.Broadcast(Item->Slot, nullptr);
	return true;
}

void ASurvivalCharacter::EquipGear(UGearItem* Gear)
{
	if (USkeletalMeshComponent* GearMesh = *PlayerMeshes.Find(Gear->Slot))
	{
		GearMesh->SetSkeletalMesh(Gear->Mesh);
		GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Gear->MaterialInstance);
	}
}

void ASurvivalCharacter::UnequipGear(EEquippableSlot Slot)
{
	if (USkeletalMeshComponent* PlayerMesh = *PlayerMeshes.Find(Slot))
	{
		if (USkeletalMesh* BodyMesh = *BareMesh.Find(Slot))
		{
			PlayerMesh->SetSkeletalMesh(BodyMesh);

			for (int i = 0; i < BodyMesh->GetMaterials().Num(); i++)
			{
				if (BodyMesh->GetMaterials().IsValidIndex(i))
					PlayerMesh->SetMaterial(i, BodyMesh->GetMaterials()[i].MaterialInterface);
			}
		}
		else
		{
			PlayerMesh->SetSkeletalMesh(nullptr);
		}
	}
}

void ASurvivalCharacter::ServerDropItem_Implementation(UItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}

bool ASurvivalCharacter::ServerDropItem_Validate(UItem* Item, const int32 Quantity)
{
	return true;
}

USkeletalMeshComponent* ASurvivalCharacter::GetSlotSkeletalMeshComp(const EEquippableSlot Slot)
{
	if(!PlayerMeshes.Contains(Slot))
		return nullptr;

	return *PlayerMeshes.Find(Slot);
}

void ASurvivalCharacter::MoveForward(float Val)
{
	AddMovementInput(GetActorForwardVector(), Val);
}

void ASurvivalCharacter::MoveRight(float Val)
{
	if (Val == 0)
		return;

	AddMovementInput(GetActorRightVector(), Val);

}

void ASurvivalCharacter::LookUp(float Val)
{
	if (Val == 0)
		return;
	
	AddControllerPitchInput(Val);
}

void ASurvivalCharacter::Turn(float Val)
{
	if (Val == 0)
		return;
	
	AddControllerYawInput(Val);
}

void ASurvivalCharacter::StartCrouching()
{
	Crouch();
}

void ASurvivalCharacter::StopCrouching()
{
	UnCrouch();
}

// Called every frame
void ASurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bIsIneractingOnServer = (HasAuthority() && IsInteracting());

	if(!HasAuthority() || GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
		PerformInteractionCheck();
}

void ASurvivalCharacter::PerformInteractionCheck()
{
	FVector EyeLoc;
	FRotator EyeRot;
	FHitResult TraceHit;

	if (!GetController())
		return;

	GetController()->GetPlayerViewPoint(EyeLoc, EyeRot);
	FVector TraceStart = EyeLoc;
	FVector TraceEnd = (EyeRot.Vector() * InteractionCheckDistance) + TraceStart;
	
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	GetController()->GetPlayerViewPoint(EyeLoc, EyeRot);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (!GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)) {
		CouldntFindInteractable();
		return;
	}
	else if (!TraceHit.GetActor()) {
		CouldntFindInteractable();
		return;
	}
	else if (UInteractionComponent* InteractionComp = Cast<UInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass())))
	{
		float DistanceToInteractable = (TraceStart - TraceHit.ImpactPoint).Size();

		if (DistanceToInteractable > InteractionComp->InteractionDistance) {
			CouldntFindInteractable();
			return;
		}
		
		FoundInteractable(InteractionComp);
	}
}

void ASurvivalCharacter::CouldntFindInteractable()
{
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact)) 
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}
	
	if (!GetInteractable())
		return;

	GetInteractable()->EndFocus(this);

	if (InteractionData.bInteractHeld)
	{
		EndInteract();
	}

	InteractionData.ViewedInteractionComponent = nullptr;
}

void ASurvivalCharacter::FoundInteractable(UInteractionComponent* Interactable)
{
	EndInteract();

	if(!Interactable)
		return;

	if (GetInteractable())
	{
		GetInteractable()->EndFocus(this);
	}

	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->BeginFocus(this);
	

}

void ASurvivalCharacter::BeginInteract()
{
	if(!HasAuthority())
		ServerBeginInteract();

	if (HasAuthority())
		PerformInteractionCheck();

	InteractionData.bInteractHeld = true;

	if (!GetInteractable())
		return;

	GetInteractable()->BeginInteract(this);

	if (FMath::IsNearlyZero(GetInteractable()->InteractionTime))
	{
		Interact();
	}
	else
	{
		GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &ASurvivalCharacter::Interact, GetInteractable()->InteractionTime, false);
	}
}

void ASurvivalCharacter::EndInteract()
{
	if (!HasAuthority())
		ServerEndInteract();

	InteractionData.bInteractHeld = false;

	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	if (!GetInteractable())
		return;

	GetInteractable()->EndInteract(this);

}

void ASurvivalCharacter::ServerBeginInteract_Implementation()
{
	BeginInteract();
}

void ASurvivalCharacter::ServerEndInteract_Implementation()
{
	EndInteract();
}

bool ASurvivalCharacter::ServerBeginInteract_Validate()
{
	return true;
}

bool ASurvivalCharacter::ServerEndInteract_Validate()
{
	return true;
}

void ASurvivalCharacter::Interact()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	
	if (!GetInteractable())
		return;

	GetInteractable()->Interact(this);
}

// Called to bind functionality to input
void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASurvivalCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ASurvivalCharacter::EndInteract);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASurvivalCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASurvivalCharacter::StopCrouching);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASurvivalCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASurvivalCharacter::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &ASurvivalCharacter::LookUp);
	PlayerInputComponent->BindAxis("Turn", this, &ASurvivalCharacter::Turn);
}

void ASurvivalCharacter::SetupComps()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(GetMesh(), TEXT("CameraSocket"));
	CameraComponent->bUsePawnControlRotation = true;

	HelmetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HelmetMesh");
	HelmetMesh->SetupAttachment(GetMesh());
	HelmetMesh->SetMasterPoseComponent(GetMesh());

	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ChestMesh");
	ChestMesh->SetupAttachment(GetMesh());
	ChestMesh->SetMasterPoseComponent(GetMesh());

	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("LegsMesh");
	LegsMesh->SetupAttachment(GetMesh());
	LegsMesh->SetMasterPoseComponent(GetMesh());

	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("FeetMesh");
	FeetMesh->SetupAttachment(GetMesh());
	FeetMesh->SetMasterPoseComponent(GetMesh());

	VestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("VestMesh");
	VestMesh->SetupAttachment(GetMesh());
	VestMesh->SetMasterPoseComponent(GetMesh());

	HandsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HandsMesh");
	HandsMesh->SetupAttachment(GetMesh());
	HandsMesh->SetMasterPoseComponent(GetMesh());

	BackpackMesh = CreateDefaultSubobject<USkeletalMeshComponent>("BackpackMesh");
	BackpackMesh->SetupAttachment(GetMesh());
	BackpackMesh->SetMasterPoseComponent(GetMesh());

	GetMesh()->SetOwnerNoSee(true);

	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Player Inventory"));
	PlayerInventory->SetCapacity(Capaciy);
	PlayerInventory->SetWeightCapacity(CarryWeight);

	HelmetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Helmet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh")));
	ChestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Chest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
	LegsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Legs, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
	FeetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Feet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
	VestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Vest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VestMesh")));
	HandsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Hands, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));
	BackpackMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Backpack, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh")));

	for (auto& PlayerMesh : PlayerMeshes)
	{
		USkeletalMeshComponent* MeshComp = PlayerMesh.Value;
		MeshComp->SetupAttachment(GetMesh());
		MeshComp->SetMasterPoseComponent(GetMesh());
	}

	//Add the head last so that it is not attached to the its self in the for loop since the head is set to the spot that GetMesh() returns.
	PlayerMeshes.Add(EEquippableSlot::EIS_Head, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMesh")));
}

void ASurvivalCharacter::DrawLookDebug()
{
	FVector EyeLoc;
	FRotator EyeRot;
	FHitResult TraceHit;

	if (!GetController())
		return;

	GetController()->GetPlayerViewPoint(EyeLoc, EyeRot);
	FVector TraceStart = EyeLoc;
	FVector TraceEnd = (EyeRot.Vector() * InteractionCheckDistance) + TraceStart;

	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, -1.f, 0, 5.f);
}



