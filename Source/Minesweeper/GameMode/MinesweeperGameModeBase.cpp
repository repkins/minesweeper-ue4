// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameModeBase.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"
#include "MinesweeperGameStateBase.h"

const FIntPoint AMinesweeperGameModeBase::DefaultCellCoords(-1, -1);

AMinesweeperGameModeBase::AMinesweeperGameModeBase(): Super()
{
	// Setting defaults
	MineGridMapVersion = 0;
	bIsGameOver = false;
}

void AMinesweeperGameModeBase::BeginPlay()
{
	Super::BeginPlay();
}

void AMinesweeperGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AMinesweeperPlayerControllerBase* NewMinesweeperPlayer = Cast<AMinesweeperPlayerControllerBase>(NewPlayer))
	{
		NewMinesweeperPlayer->OnPlayerNewGame.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnPlayerNewGame);
		NewMinesweeperPlayer->OnPlayerTriggeredCoords.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnPlayerTriggeredCoords);

		// Automatically assign first player as lobby leader
		if (!IsValid(LobbyLeader))
		{
			LobbyLeader = NewMinesweeperPlayer;
			NewMinesweeperPlayer->SetIsLobbyLeader(true);
		}
		else
		{
			NewMinesweeperPlayer->SetIsLobbyLeader(false);
		}
	}
}

void AMinesweeperGameModeBase::HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords)
{
	if (!bIsGameOver && RemainingClearCellCount > 0)
	{
		OpenCell(EnteredCoords);

		if (RemainingClearCellCount == 0)
		{
			for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
			{
				if (auto MinesweeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
				{
					MinesweeperPlayer->NotifyGameWin();
				}
			}
		}
	}
}

void AMinesweeperGameModeBase::OpenCell(const FIntPoint& EnteredCoords)
{
	if (ActualMinesHidden.Contains(EnteredCoords))
	{
		MineGridMap.Cells.Emplace(EnteredCoords, EMineGridMapCell::MGMC_Revealed);

		bIsGameOver = true;

		if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
		{
			MinesweeperState->SetExplodedCell(EnteredCoords);
		}

		for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
		{
			if (auto MinesweeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
			{
				MinesweeperPlayer->NotifyGameOver();
			}
		}
	}
	else
	{
		// 
		// Use of queue to automatically open nearby cells if opened cell with no mines around it
		//

		TQueue<FIntPoint> RemainingCells;
		RemainingCells.Enqueue(EnteredCoords);

		FIntPoint RemainingCellCoords;

		TArray<FIntPoint> SurroundingCellCoordsList;
		SurroundingCellCoordsList.Reserve(8);
		while (RemainingCells.Dequeue(RemainingCellCoords))
		{
			if (MineGridMap.Cells[RemainingCellCoords] != EMineGridMapCell::MGMC_Undiscovered)
			{
				continue;
			}

			FIntPoint StartValidSurroundingCoords(FMath::Clamp(RemainingCellCoords.X - 1, MineGridMap.StartCoords.X, MineGridMap.EndCoords.X),
				FMath::Clamp(RemainingCellCoords.Y - 1, MineGridMap.StartCoords.Y, MineGridMap.EndCoords.Y));
			FIntPoint EndValidSurroundingCoords(FMath::Clamp(RemainingCellCoords.X + 1, MineGridMap.StartCoords.X, MineGridMap.EndCoords.X),
				FMath::Clamp(RemainingCellCoords.Y + 1, MineGridMap.StartCoords.Y, MineGridMap.EndCoords.Y));

			SurroundingCellCoordsList.Reset();

			uint8 MinesCount = 0;
			for (int32 Y = StartValidSurroundingCoords.Y; Y <= EndValidSurroundingCoords.Y; Y++)
			{
				for (int32 X = StartValidSurroundingCoords.X; X <= EndValidSurroundingCoords.X; X++)
				{
					FIntPoint CellCoords(X, Y);
					if (CellCoords == RemainingCellCoords)
					{
						continue;
					}

					if (ActualMinesHidden.Contains(CellCoords))
					{
						MinesCount++;
					}

					SurroundingCellCoordsList.Add(CellCoords);
				}
			}

			MineGridMap.Cells.Emplace(RemainingCellCoords, (EMineGridMapCell)MinesCount);
			RemainingClearCellCount -= 1;

			if (MinesCount == 0)
			{
				for (FIntPoint SurroundingCellCoords : SurroundingCellCoordsList)
				{
					RemainingCells.Enqueue(SurroundingCellCoords);
				}
			}
		}
	}

	MineGridMapVersion += 1;
}

void AMinesweeperGameModeBase::HandleOnPlayerNewGame(const uint8 MapSize)
{
	bIsGameOver = false;

	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->UnsetExplodedCell();
	}

	GenerateNewMap(MapSize);
	MineGridMapVersion = 0;

	for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (auto MinesweeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
		{
			// Force add/remove cells
			MinesweeperPlayer->AddRemoveGridMapAreaCells(MineGridMap, true);

			// Update GridMapArea values
			MinesweeperPlayer->UpdateGridMapAreaValues(MineGridMap);

			// Notify clients that game is started
			MinesweeperPlayer->NotifyGameStarted();
		}
	}
}

void AMinesweeperGameModeBase::GenerateNewMap(const uint8 MapSize)
{
	FIntPoint BaseDimensions(5, 4);
	int32 Scale = FMath::FloorToInt(FMath::Exp2(MapSize));

	MineGridMap.GridDimensions = BaseDimensions * Scale;
	MineGridMap.Cells.Empty(MineGridMap.GridDimensions.X * MineGridMap.GridDimensions.Y);

	MineGridMap.StartCoords = FIntPoint::ZeroValue;
	MineGridMap.EndCoords = MineGridMap.GridDimensions - 1;

	ActualMinesHidden.Reset();
	for (int32 Y = MineGridMap.StartCoords.Y; Y <= MineGridMap.EndCoords.Y; Y++)
	{
		for (int32 X = MineGridMap.StartCoords.X; X <= MineGridMap.EndCoords.X; X++)
		{
			FIntPoint CellCoords(X, Y);
			MineGridMap.Cells.Emplace(CellCoords, EMineGridMapCell::MGMC_Undiscovered);

			if (FMath::RandRange(0, 5) == 0)
			{
				ActualMinesHidden.Emplace(CellCoords);
			}
		}
	}

	RemainingClearCellCount = MineGridMap.Cells.Num() - ActualMinesHidden.Num();
}
