// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperHUDBase.h"
#include "Blueprint/UserWidget.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"

AMinesweeperHUDBase::AMinesweeperHUDBase(): Super()
{
	bNewGameMenuVisible = false;
	bGameOverVisible = false;
	bGameWinVisible = false;
}

void AMinesweeperHUDBase::BeginPlay()
{
	Super::BeginPlay();

	// 
	// Create and add widgets
	// 

	if (NewGameWidgetClass) {
		NewGameWidget = CreateWidget<UUserWidget>(PlayerOwner, NewGameWidgetClass);
		if (NewGameWidget) {
			NewGameWidget->AddToViewport();
			NewGameWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (GameOverWidgetClass) {
		GameOverWidget = CreateWidget<UUserWidget>(PlayerOwner, GameOverWidgetClass);
		if (GameOverWidget) {
			GameOverWidget->AddToViewport();
			GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (GameWinWidgetClass) {
		GameWinWidget = CreateWidget<UUserWidget>(PlayerOwner, GameWinWidgetClass);
		if (GameWinWidget) {
			GameWinWidget->AddToViewport();
			GameWinWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AMinesweeperHUDBase::ShowNewGameMenu()
{
	if (NewGameWidget && PlayerOwner)
	{
		NewGameWidget->SetVisibility(ESlateVisibility::Visible);
		PlayerOwner->SetInputMode(FInputModeGameAndUI());
		PlayerOwner->SetShowMouseCursor(true);
		bNewGameMenuVisible = true;
	}
}

void AMinesweeperHUDBase::HideNewGameMenu()
{
	if (NewGameWidget && PlayerOwner)
	{
		NewGameWidget->SetVisibility(ESlateVisibility::Hidden);
		PlayerOwner->SetInputMode(FInputModeGameOnly());
		PlayerOwner->SetShowMouseCursor(false);
		bNewGameMenuVisible = false;
	}
}

void AMinesweeperHUDBase::ShowGameOver()
{
	if (GameOverWidget && PlayerOwner)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Visible);
		PlayerOwner->SetInputMode(FInputModeGameAndUI());
		PlayerOwner->SetShowMouseCursor(true);
		bGameOverVisible = true;
	}
}

void AMinesweeperHUDBase::HideGameOver()
{
	if (GameOverWidget && PlayerOwner)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
		PlayerOwner->SetInputMode(FInputModeGameOnly());
		PlayerOwner->SetShowMouseCursor(false);
		bGameOverVisible = false;
	}
}

void AMinesweeperHUDBase::ShowGameWin()
{
	if (GameWinWidget && PlayerOwner)
	{
		GameWinWidget->SetVisibility(ESlateVisibility::Visible);
		PlayerOwner->SetInputMode(FInputModeGameAndUI());
		PlayerOwner->SetShowMouseCursor(true);
		bGameWinVisible = true;
	}
}

void AMinesweeperHUDBase::HideGameWin()
{
	if (GameWinWidget && PlayerOwner)
	{
		GameWinWidget->SetVisibility(ESlateVisibility::Hidden);
		PlayerOwner->SetInputMode(FInputModeGameOnly());
		PlayerOwner->SetShowMouseCursor(false);
		bGameWinVisible = false;
	}
}
