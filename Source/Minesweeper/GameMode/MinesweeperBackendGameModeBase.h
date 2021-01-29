// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinesweeperGameModeBase.h"

#include "Minesweeper/Backend/MinesweeperBackendComponent.h"
#include "Minesweeper/MineGrid/MineGridBase.h"

#include "MinesweeperBackendGameModeBase.generated.h"

/**
 * Defines the minesweeper mode and controls it's gameplay according by Minesweeper backend
 * using it's backend integration component.
 */
UCLASS()
class MINESWEEPER_API AMinesweeperBackendGameModeBase : public AMinesweeperGameModeBase
{
	GENERATED_BODY()

public:

	AMinesweeperBackendGameModeBase();

	FORCEINLINE const FMineGridMap& GetMineGridMap() override { return MinesweeperBackend->GetLastReceivedMineGridMap(); }

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

	void HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords) override;
	void HandleOnPlayerNewGame(const uint8 MapSize) override;

	void GenerateNewMap(const uint8 MapSize) override;
};
