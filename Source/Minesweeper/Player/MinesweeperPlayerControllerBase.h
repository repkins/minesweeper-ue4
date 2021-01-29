// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Minesweeper/MineGrid/MineGridBase.h"

#include "MinesweeperPlayerControllerBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerNewGameDelegate, const uint8, MapSize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTriggeredCoordsDelegate, const FIntPoint&, EnteredIntoCoords);

typedef TTuple<FIntPoint, FIntPoint> TCoordsBoundsTuple;

/**
 * This actor controls pawn movement, "visible" area of mine grid map and HUD widgets visibility
 */
UCLASS()
class MINESWEEPER_API AMinesweeperPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FOnPlayerNewGameDelegate OnPlayerNewGame;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerTriggeredCoordsDelegate OnPlayerTriggeredCoords;

	AMinesweeperPlayerControllerBase();

	UFUNCTION()
	void AddRemoveGridMapAreaCells(const FMineGridMap& MineGridMap, bool bForcedAddRemove = false);

	UFUNCTION()
	void UpdateGridMapAreaValues(const FMineGridMap& MineGridMap);

	UFUNCTION(Client, Reliable)
	void NotifyGameOver();

	UFUNCTION(Client, Reliable)
	void NotifyGameWin();

protected:
	/**
	 * The class of MineGridClass to lookup in world for representation of mine grid data in it
	 * and listening for "enter" events from.
	 */
	UPROPERTY(EditDefaultsOnly, NoClear, BlueprintReadOnly, Category = "Minesweeper|Grid")
	TSubclassOf<AMineGridBase> MineGridClass;

	/** Reference to Mine grid actor to send map updates and listen for coords entering events */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	AMineGridBase* MineGridActor;

	/** Defines the "visible" part of mine grid map */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	FMineGridMap MineGridMapArea;

	/** Number of grid map columns to be rendered by cell actors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	uint8 MapAreaMaxHalfSizeX;

	/** Number of grid map rows to be rendered by cell actors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	uint8 MapAreaMaxHalfSizeY;

	/** Player previously visited cell coords (inside or outside of grid actor) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	FIntPoint PrevPlayerRelativeGridCoords;

	/** 
	 * Store latest version of the map area. Used to compare with gamemode's map version. 
	 * Does not increment but assigns from gamemode's map version property.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|Grid")
	int32 GridMapAreaVersion;

	// 
	// UI Widgets
	// 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> NewGameWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* NewGameWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bNewGameMenuVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* GameOverWidget;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bGameOverVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper|HUD")
	TSubclassOf<class UUserWidget> GameWinWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	class UUserWidget* GameWinWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper|HUD")
	bool bGameWinVisible;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleOnTriggeredCoords(const FIntPoint& EnteredCoords);

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

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SelectNewGame(const uint8 MapSize);

	UFUNCTION(NetMulticast, Reliable)
	void ApplyAddedRemovedGridCells(const FMineGridMapChanges& GridMapChanges);

	UFUNCTION(NetMulticast, Reliable)
	void ApplyUpdatedGridCellValues(const FMineGridMapCellUpdates& GridMapChanges);

	AMineGridBase* FindMineGridActor();
	FIntPoint GetPawnRelativeLocationOfGrid(APawn* PlayerPawn, AMineGridBase* MineGrid);
};
