// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "Minesweeper/Includes/MineGridMap.h"
#include "Minesweeper/MineGrid/MineGridBase.h"

#include "MinesweeperGameModeBase.generated.h"

/**
 * Defines the minesweeper mode and responsable for course of matches.
 */
UCLASS()
class MINESWEEPER_API AMinesweeperGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	static const FIntPoint DefaultCellCoords;

	AMinesweeperGameModeBase();

	FORCEINLINE virtual const FMineGridMap& GetMineGridMap() { return MineGridMap; }

	FORCEINLINE const int32 GetMineGridMapVersion() { return MineGridMapVersion; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	TSet<FIntPoint> ActualMinesHidden;

	/**
	 * Current version of mine grid map
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	FMineGridMap MineGridMap;

	/**
	 * Stores latest version number of grid map. Used for update determination.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	int32 MineGridMapVersion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minesweeper")
	class AMinesweeperPlayerControllerBase* LobbyLeader;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	int32 RemainingClearCellCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minesweeper")
	bool bIsGameOver;

	virtual void BeginPlay() override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	UFUNCTION()
	virtual void HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords);
	UFUNCTION()
	virtual void HandleOnPlayerNewGame(const uint8 MapSize);

	virtual void GenerateNewMap(const uint8 MapSize);
	virtual void OpenCell(const FIntPoint& EnteredCoords);
};
