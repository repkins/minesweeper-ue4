// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "Minesweeper/Backend/MinesweeperBackendComponent.h"
#include "Minesweeper/MineGrid/MineGridBase.h"

#include "MinesweeperGameModeBase.generated.h"

/**
 * Defines the minesweeper mode and controls it's gameplay according by Minesweeper backend 
 * using it's backend integration component.
 */
UCLASS()
class MINESWEEPER_API AMinesweeperGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	static const FIntPoint DefaultCellCoords;

	AMinesweeperGameModeBase();

	FORCEINLINE const FMineGridMap& GetMineGridMap() { return MinesweeperBackend->GetLastReceivedMineGridMap(); }

	FORCEINLINE const int32 GetMineGridMapVersion() { return MineGridMapVersion; }

protected:

	/**
	 * The separate integration component to use to communicate with backend
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	UMinesweeperBackendComponent* MinesweeperBackend;

	/**
	 * Store last sent coordinates with the "open" command to the backend
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	FIntPoint LastSentCoordsToOpen;

	/**
	 * Stores latest version of received grid map from backend
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	int32 MineGridMapVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	bool bIsGameOver;

	/**
	 * The following is necessary to ensure processing only one triggered cell at a time by backend.
	 */
	bool bTriggeredCellProcessing;
	TQueue<FIntPoint> RemainingCellsToProcess;

	virtual void BeginPlay() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION()
	void HandleOnNewGameOk();
	UFUNCTION()
	void HandleOnMapOk(const FMineGridMap& NewMineGridMap);
	UFUNCTION()
	void HandleOnOpenOk();
	UFUNCTION()
	void HandleOnOpenGameOver();
	UFUNCTION()
	void HandleOnOpenYouWin(const FString& LevelPassword);
	void HandleOnOpen();

	UFUNCTION()
	void HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords);
	UFUNCTION()
	void HandleOnPlayerNewGame(const uint8 MapSize);

};
