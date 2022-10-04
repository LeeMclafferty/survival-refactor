// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/FoodItem.h"

#define LOCTEXT_NAMESPACE "FoodItem"

UFoodItem::UFoodItem()
	:HealAmount(20.f)
{
	UseActionText = LOCTEXT("ItemUseAction", "Consume");
}

void UFoodItem::Use(ASurvivalCharacter* Character)
{
	//TODO: Heal Character

	UE_LOG(LogTemp, Warning, TEXT("Nom Nom"));
}

#undef LOCTEXT_NAMESPACE
