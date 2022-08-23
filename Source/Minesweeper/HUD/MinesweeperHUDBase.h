// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "MinesweeperHUDBase.generated.h"

/**
 * Base class representing Minesweeper HUD. Governs widgets logic.
 */
UCLASS()
class MINESWEEPER_API AMinesweeperHUDBase : public AHUD
{
	GENERATED_BODY()
	
public:

	AMinesweeperHUDBase();
	
	UFUNCTION(BlueprintCallable)
	void ShowNewGameMenu();

	UFUNCTION(BlueprintCallable)
	void HideNewGameMenu();

	UFUNCTION(BlueprintCallable)
	void ShowGameOver();

	UFUNCTION(BlueprintCallable)
	void HideGameOver();

	UFUNCTION(BlueprintCallable)
	void ShowGameWin();

	UFUNCTION(BlueprintCallable)
	void HideGameWin();

protected:

	/** New game widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> NewGameWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* NewGameWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bNewGameMenuVisible;

	/** Game over widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* GameOverWidget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bGameOverVisible;

	/** Game win widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> GameWinWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* GameWinWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bGameWinVisible;

	virtual void BeginPlay() override;
};
