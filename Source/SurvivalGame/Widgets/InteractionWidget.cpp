// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InteractionWidget.h"
#include "Components/InteractionComponent.h"

void UInteractionWidget::UpdateInteractionWidget(UInteractionComponent* InteractionComponent)
{
	if (!InteractionComponent)
		return;

	OwningInteractionComponent = InteractionComponent;
	OnUpdatedInteractionWidget();
}


