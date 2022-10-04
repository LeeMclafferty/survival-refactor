
#include "Player/SurvivalCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Items/Item.h"
#include "World/Pickup.h"


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

void ASurvivalCharacter::ServerDropItem_Implementation(UItem* Item, const int32 Quantity)
{
	DropItem(Item, Quantity);
}

bool ASurvivalCharacter::ServerDropItem_Validate(UItem* Item, const int32 Quantity)
{
	return true;
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



