// Fill out your copyright notice in the Description page of Project Settings.


#include "SurviverPlayerController.h"
#include "Blueprint/UserWidget.h"
ASurviverPlayerController::ASurviverPlayerController() 
{

}

void ASurviverPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//Check our HUDOverlayClass 

	if (HUDOverlayClass) {
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if (HUDOverlay) {
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}


}
