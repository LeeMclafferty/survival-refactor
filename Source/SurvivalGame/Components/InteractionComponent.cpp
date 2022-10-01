// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "Player/SurvivalCharacter.h"
#include "Widgets/InteractionWidget.h"

UInteractionComponent::UInteractionComponent()
	:InteractionTime(0.f),
	InteractionDistance(200.f),
	InteractableNameText(FText::FromString(TEXT("Interactable Object"))),
	InteractableActionText(FText::FromString(TEXT("Interact"))),
	bAllowMultipleInteractors(true)
{
	SetComponentTickEnabled(false);

	// Widget Params
	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(400, 100);
	bDrawAtDesiredSize = true;

	// Hide Widget until the player looks at it. 
	SetActive(true);
	SetHiddenInGame(true);
}

void UInteractionComponent::SetInteractableNameText(const FText& NewNameText)
{
	InteractableNameText = NewNameText;
	RefreshWidget();
}

void UInteractionComponent::SetInteractableActionText(const FText& NewNameText)
{
	InteractableActionText = NewNameText;
	RefreshWidget();
}

void UInteractionComponent::BeginFocus(ASurvivalCharacter* Character)
{
	if (!IsActive() || !GetOwner() || !Character)
		return;

	OnBeginFocus.Broadcast(Character);

	SetHiddenInGame(false);
	
	if (!GetOwner()->HasAuthority())
	{

		for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
		{
			if (UPrimitiveComponent* Primative = Cast<UPrimitiveComponent>(VisualComp))
			{
				Primative->SetRenderCustomDepth(true);
			}
		}
	}

	RefreshWidget();
}

void UInteractionComponent::EndFocus(ASurvivalCharacter* Character)
{
	if (!IsActive() || !GetOwner() || !Character)
		return;

	OnEndFocus.Broadcast(Character);

	SetHiddenInGame(true);

	if (!GetOwner()->HasAuthority())
	{
		for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
		{
			if (UPrimitiveComponent* Primative = Cast<UPrimitiveComponent>(VisualComp))
			{
				Primative->SetRenderCustomDepth(false);
			}
		}
	}
}

void UInteractionComponent::BeginInteract(ASurvivalCharacter* Character)
{
	if (!CanInteract(Character))
		return;
	
	Interactors.AddUnique(Character);
	OnBeginInteract.Broadcast(Character);
}

void UInteractionComponent::EndInteract(ASurvivalCharacter* Character)
{
	if (!Character)
		return;

	Interactors.RemoveSingle(Character);
	OnEndInteract.Broadcast(Character);
}

void UInteractionComponent::Interact(ASurvivalCharacter* Character)
{
	if (!CanInteract(Character))
		return;

	OnInteract.Broadcast(Character);
}

float UInteractionComponent::GetInteractPercentage()
{
	if (!Interactors.IsValidIndex(0))
		return 0.f;

	if (auto* Interactor = Interactors[0])
	{
		return 1.f - FMath::Abs(Interactor->GetRemainingInerteractTime() / InteractionTime);
	}

	return 0.0f;
}

void UInteractionComponent::RefreshWidget()
{
	if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject()))
		{
			InteractionWidget->UpdateInteractionWidget(this);
		}
	}
		
}

void UInteractionComponent::Deactivate()
{
	Super::Deactivate();

	for (int32 i = Interactors.Num() - 1; i >= 0; --i)
	{
		if (ASurvivalCharacter* Interactor = Interactors[i])
		{
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}

	Interactors.Empty();
}

bool UInteractionComponent::CanInteract(ASurvivalCharacter* Character) const
{
	const bool bPlayerAlreadyIntercating = !bAllowMultipleInteractors && Interactors.Num() >= 1;

	return !bPlayerAlreadyIntercating && IsActive() && GetOwner() && Character;

}
