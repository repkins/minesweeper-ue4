// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperPlayerControllerBase.h"
#include "EngineUtils.h"
#include "Blueprint/UserWidget.h"
#include "Minesweeper/GameMode/MinesweeperGameModeBase.h"
#include "Minesweeper/GameMode/MinesweeperGameStateBase.h"

AMinesweeperPlayerControllerBase::AMinesweeperPlayerControllerBase(): Super()
{
	PrimaryActorTick.bCanEverTick = true;
	bAllowTickBeforeBeginPlay = false;

	MineGridClass = AMineGridBase::StaticClass();
	bNewGameMenuVisible = false;
	bGameOverVisible = false;
	bGameWinVisible = false;

	MapAreaMaxHalfSizeX = 7;
	MapAreaMaxHalfSizeY = 4;

	PrevPlayerRelativeGridCoords = FIntPoint(-1, -1);
	GridMapAreaVersion = 0;
}

void AMinesweeperPlayerControllerBase::NotifyGameOver_Implementation()
{
	ShowGameOver();
}

void AMinesweeperPlayerControllerBase::NotifyGameWin_Implementation()
{
	ShowGameWin();
}

void AMinesweeperPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	// Creates widgets if controller is local
	if (IsLocalController())
	{
		if (NewGameWidgetClass) {
			NewGameWidget = CreateWidget<UUserWidget>(this, NewGameWidgetClass);
			if (NewGameWidget) {
				NewGameWidget->AddToViewport();
				NewGameWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		if (GameOverWidgetClass) {
			GameOverWidget = CreateWidget<UUserWidget>(this, GameOverWidgetClass);
			if (GameOverWidget) {
				GameOverWidget->AddToViewport();
				GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		if (GameWinWidgetClass) {
			GameWinWidget = CreateWidget<UUserWidget>(this, GameWinWidgetClass);
			if (GameWinWidget) {
				GameWinWidget->AddToViewport();
				GameWinWidget->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	// Finding grid actor
	MineGridActor = FindMineGridActor();
	if (MineGridActor)
	{
		// Bind to on entered coords event of grid actor
		MineGridActor->OnCharacterTriggeredCoords.AddDynamic(this, &AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords);
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MinesweeperPlayerController needs MineGrid actor placed in the level to work."));
		}
	}

	// Initializing LastCoords field
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			PrevPlayerRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);
		}
	}
}

void AMinesweeperPlayerControllerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AMinesweeperGameModeBase* MinesweeperGameMode = GetWorld()->GetAuthGameMode<AMinesweeperGameModeBase>())
	{
		const FMineGridMap& FullMineGridMap = MinesweeperGameMode->GetMineGridMap();

		// Compare grid map versions to determine if update is necessary, then proceed with update
		const uint32& FullGridMapVersion = MinesweeperGameMode->GetMineGridMapVersion();

		if (FullGridMapVersion != GridMapAreaVersion)
		{
			// Update GridMapArea values
			UpdateGridMapAreaValues(FullMineGridMap);

			// Update map version
			GridMapAreaVersion = FullGridMapVersion;
		}
		else
		{
			// Add&Remove cells of GridMapArea as necessary
			AddRemoveGridMapAreaCells(FullMineGridMap);
		}
	}

	if (AMinesweeperGameStateBase* MinesweeperState = GetWorld()->GetGameState<AMinesweeperGameStateBase>())
	{
		if (MinesweeperState->HasExplodedCell())
		{
			MineGridActor->SetGridCellExploded(MinesweeperState->GetExplodedCoords());
		}
	}
}

void AMinesweeperPlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllGridCells();
}

void AMinesweeperPlayerControllerBase::AddRemoveGridMapAreaCells(const FMineGridMap& MineGridMap, bool bForcedAddRemove)
{
	if (MineGridActor)
	{
		if (APawn* PlayerPawn = GetPawn())
		{
			const FIntPoint PawnRelativeGridCoords = GetPawnRelativeLocationOfGrid(PlayerPawn, MineGridActor);

			// Check if need update by determining if pawn (player) is at the same coords as before or 
			// it is forced to update, then proceed with map area update if new pawn coords is different
			if (PawnRelativeGridCoords != PrevPlayerRelativeGridCoords || bForcedAddRemove)
			{
				// Define reference to grid map dimensions
				const FIntPoint& MapGridDimensions = MineGridMap.GridDimensions;

				// Determine starting & endings coords of map area
				const FIntPoint MapAreaMaxHalfSize = FIntPoint(MapAreaMaxHalfSizeX, MapAreaMaxHalfSizeY);
				const FIntPoint MaxStartCoords = PawnRelativeGridCoords - MapAreaMaxHalfSize;
				const FIntPoint MaxEndCoords = PawnRelativeGridCoords + MapAreaMaxHalfSize + FIntPoint(1, 1);

				// Allocate max size
				const FIntPoint MapAreaMaxSize = MaxEndCoords - MaxStartCoords + FIntPoint(1, 1);
				MineGridMapArea.Cells.Reserve(MapAreaMaxSize.X * MapAreaMaxSize.Y);

				// Determine valid starting & endings coords of map "visible" area
				//const FIntPoint NewStartCoords = FIntPoint(FMath::Clamp(MaxStartCoords.X, 0, MapGridDimensions.X), FMath::Clamp(MaxStartCoords.Y, 0, MapGridDimensions.Y));
				//const FIntPoint NewEndCoords = FIntPoint(FMath::Clamp(MaxEndCoords.X, -1, MapGridDimensions.X - 1), FMath::Clamp(MaxEndCoords.Y, -1, MapGridDimensions.Y - 1));

				FIntPoint NewStartCoords;
				NewStartCoords.X = MaxStartCoords.X >= 0 ? MaxStartCoords.X : FMath::Min(0, MaxEndCoords.X + 1);
				NewStartCoords.Y = MaxStartCoords.Y >= 0 ? MaxStartCoords.Y : FMath::Min(0, MaxEndCoords.Y + 1);

				FIntPoint NewEndCoords;
				NewEndCoords.X = MaxEndCoords.X <= MapGridDimensions.X - 1 ? MaxEndCoords.X : FMath::Max(MapGridDimensions.X - 1, MaxStartCoords.X - 1);
				NewEndCoords.Y = MaxEndCoords.Y <= MapGridDimensions.Y - 1 ? MaxEndCoords.Y : FMath::Max(MapGridDimensions.Y - 1, MaxStartCoords.Y - 1);

				// Because start and end coords are inclusive, we need to increment by 1
				const FIntPoint MapAreaSize = NewEndCoords - NewStartCoords + FIntPoint(1, 1);

				const FIntPoint OldStartCoords = MineGridMapArea.StartCoords;
				const FIntPoint OldEndCoords = MineGridMapArea.EndCoords;

				TArray<TCoordsBoundsTuple> AdditiveBounds;
				TArray<TCoordsBoundsTuple> SubtractiveBounds;
				AdditiveBounds.Reserve(4);
				SubtractiveBounds.Reserve(4);

				// Top-left difference
				const FIntPoint LeftTopDiff = OldStartCoords - NewStartCoords;

				// Left side
				if (LeftTopDiff.X > 0)
				{
					// Additive
					const FIntPoint LeftStartCoords(NewStartCoords.X, NewStartCoords.Y);
					const FIntPoint LeftEndCoords(NewStartCoords.X + LeftTopDiff.X - 1, NewEndCoords.Y);
					AdditiveBounds.Emplace(LeftStartCoords, LeftEndCoords);
				}
				else if (LeftTopDiff.X < 0)
				{
					// Subtractive
					const FIntPoint LeftStartCoords(OldStartCoords.X, OldStartCoords.Y);
					const FIntPoint LeftEndCoords(OldStartCoords.X - LeftTopDiff.X - 1, OldEndCoords.Y);
					SubtractiveBounds.Emplace(LeftStartCoords, LeftEndCoords);
				}

				// Top side
				if (LeftTopDiff.Y > 0)
				{
					// Additive
					const FIntPoint TopStartCoords(NewStartCoords.X, NewStartCoords.Y);
					const FIntPoint TopEndCoords(NewEndCoords.X, NewStartCoords.Y + LeftTopDiff.Y - 1);
					AdditiveBounds.Emplace(TopStartCoords, TopEndCoords);
				}
				else if (LeftTopDiff.Y < 0)
				{
					// Subtractive
					const FIntPoint TopStartCoords(OldStartCoords.X, OldStartCoords.Y);
					const FIntPoint TopEndCoords(OldEndCoords.X, OldStartCoords.Y - LeftTopDiff.Y - 1);
					SubtractiveBounds.Emplace(TopStartCoords, TopEndCoords);
				}

				// Bottom-right difference
				const FIntPoint RightBottomDiff = NewEndCoords - OldEndCoords;

				// Right side
				if (RightBottomDiff.X > 0)
				{
					// Additive
					const FIntPoint RightStartCoords(NewEndCoords.X - RightBottomDiff.X + 1, NewStartCoords.Y);
					const FIntPoint RightEndCoords(NewEndCoords.X, NewEndCoords.Y);
					AdditiveBounds.Emplace(RightStartCoords, RightEndCoords);
				}
				else if (RightBottomDiff.X < 0)
				{
					// Subtractive
					const FIntPoint RightStartCoords(OldEndCoords.X + RightBottomDiff.X + 1, OldStartCoords.Y);
					const FIntPoint RightEndCoords(OldEndCoords.X, OldEndCoords.Y);
					SubtractiveBounds.Emplace(RightStartCoords, RightEndCoords);
				}

				// Bottom side
				if (RightBottomDiff.Y > 0)
				{
					// Additive
					const FIntPoint BottomStartCoords(NewStartCoords.X, NewEndCoords.Y - RightBottomDiff.Y + 1);
					const FIntPoint BottomEndCoords(NewEndCoords.X, NewEndCoords.Y);
					AdditiveBounds.Emplace(BottomStartCoords, BottomEndCoords);
				}
				else if (RightBottomDiff.Y < 0)
				{
					// Subtractive
					const FIntPoint BottomStartCoords(OldStartCoords.X, OldEndCoords.Y + RightBottomDiff.Y + 1);
					const FIntPoint BottomEndCoords(OldEndCoords.X, OldEndCoords.Y);
					SubtractiveBounds.Emplace(BottomStartCoords, BottomEndCoords);
				}

				// Define removed & added cell containers
				FMineGridMapChanges GridMapChanges;
				GridMapChanges.NewGridDimensions = MapAreaSize;

				// Start with subtractive bounds first
				for (const TCoordsBoundsTuple& SideBoundsTuple : SubtractiveBounds)
				{
					if (AdditiveBounds.Contains(SideBoundsTuple))
					{
						continue;
					}

					FIntPoint StartCoords = SideBoundsTuple.Get<0>(),
						EndCoords = SideBoundsTuple.Get<1>();

					for (int32 Y = StartCoords.Y; Y <= EndCoords.Y; ++Y)
					{
						for (int32 X = StartCoords.X; X <= EndCoords.X; ++X)
						{
							FIntPoint Coords(X, Y);
							MineGridMapArea.Cells.Remove(Coords);

							GridMapChanges.RemovedGridMapCells.AddUnique(Coords);
						}
					}
				}

				// Then with additive bounds
				for (const TCoordsBoundsTuple& SideBoundsTuple : AdditiveBounds)
				{
					if (SubtractiveBounds.Contains(SideBoundsTuple))
					{
						continue;
					}

					FIntPoint StartCoords = SideBoundsTuple.Get<0>(),
						EndCoords = SideBoundsTuple.Get<1>();

					for (int32 Y = StartCoords.Y; Y <= EndCoords.Y; ++Y)
					{
						for (int32 X = StartCoords.X; X <= EndCoords.X; ++X)
						{
							FIntPoint Coords(X, Y);
							EMineGridMapCell CellValue = MineGridMap.Cells[Coords];
							MineGridMapArea.Cells.Emplace(Coords, CellValue);

							if (!GridMapChanges.AddedGridMapCellCoords.Contains(Coords))
              {
                GridMapChanges.AddedGridMapCellCoords.Add(Coords);
                GridMapChanges.AddedGridMapCellValues.Add(CellValue);
							}
						}
					}
				}

				MineGridMapArea.GridDimensions = MapAreaSize;
				MineGridMapArea.StartCoords = NewStartCoords;
				MineGridMapArea.EndCoords = NewEndCoords;

				// Finally apply changes
				ApplyAddedRemovedGridCells(GridMapChanges);

				// Update prev player coords
				PrevPlayerRelativeGridCoords = PawnRelativeGridCoords;
			}
		}
	}
}

void AMinesweeperPlayerControllerBase::ClearAllGridCells()
{
  FMineGridMapChanges GridMapChanges;
  GridMapChanges.NewGridDimensions = FIntPoint::ZeroValue;

	for (auto CellToRemove : MineGridMapArea.Cells)
	{
		const FIntPoint CoordsToRemoveAt = CellToRemove.Key;

		GridMapChanges.RemovedGridMapCells.AddUnique(CoordsToRemoveAt);
	}

	ApplyAddedRemovedGridCells(GridMapChanges);
}

void AMinesweeperPlayerControllerBase::UpdateGridMapAreaValues(const FMineGridMap& MineGridMap)
{
	FMineGridMapCellUpdates CellsUpdate;

	for (TPair<FIntPoint, EMineGridMapCell>& CoordsCellEntry : MineGridMapArea.Cells)
	{
		const FIntPoint Coords = CoordsCellEntry.Key;
		const EMineGridMapCell NewCellValue = MineGridMap.Cells[Coords];

		if (CoordsCellEntry.Value != NewCellValue)
		{
			CoordsCellEntry.Value = NewCellValue;

			CellsUpdate.UpdatedGridMapCellCoords.Emplace(Coords);
			CellsUpdate.UpdatedGridMapCellValues.Emplace(NewCellValue);
		}
	}

	ApplyUpdatedGridCellValues(CellsUpdate);
}

void AMinesweeperPlayerControllerBase::HandleOnTriggeredCoords(const FIntPoint& EnteredCoords)
{
	OnPlayerTriggeredCoords.Broadcast(EnteredCoords);
}

void AMinesweeperPlayerControllerBase::ShowNewGameMenu()
{
	if (NewGameWidget)
	{
		NewGameWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
		bNewGameMenuVisible = true;
	}
}

void AMinesweeperPlayerControllerBase::HideNewGameMenu()
{
	if (NewGameWidget)
	{
		NewGameWidget->SetVisibility(ESlateVisibility::Hidden);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		bNewGameMenuVisible = false;
	}
}

void AMinesweeperPlayerControllerBase::ShowGameOver()
{
	if (GameOverWidget)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
		bGameOverVisible = true;
	}
}

void AMinesweeperPlayerControllerBase::HideGameOver()
{
	if (GameOverWidget)
	{
		GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		bGameOverVisible = false;
	}
}

void AMinesweeperPlayerControllerBase::ShowGameWin()
{
	if (GameWinWidget)
	{
		GameWinWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputMode(FInputModeGameAndUI());
		SetShowMouseCursor(true);
		bGameWinVisible = true;
	}
}

void AMinesweeperPlayerControllerBase::HideGameWin()
{
	if (GameWinWidget)
	{
		GameWinWidget->SetVisibility(ESlateVisibility::Hidden);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		bGameWinVisible = false;
	}
}

void AMinesweeperPlayerControllerBase::SelectNewGame_Implementation(const uint8 MapSize)
{
	OnPlayerNewGame.Broadcast(MapSize);
}

void AMinesweeperPlayerControllerBase::ApplyAddedRemovedGridCells_Implementation(const FMineGridMapChanges& GridMapChanges)
{
	if (MineGridActor)
	{
		MineGridActor->AddOrRemoveGridCells(GridMapChanges);
	}
}

void AMinesweeperPlayerControllerBase::ApplyUpdatedGridCellValues_Implementation(const FMineGridMapCellUpdates& UpdatedCells)
{
	if (MineGridActor)
	{
		MineGridActor->UpdateCellValues(UpdatedCells);
	}
}

AMineGridBase* AMinesweeperPlayerControllerBase::FindMineGridActor()
{
	UWorld* World = GetWorld();
	for (TActorIterator<AMineGridBase> ActorIt(World, MineGridClass); ActorIt; ++ActorIt)
	{
		AMineGridBase* MineGrid = *ActorIt;
		if (MineGrid)
		{
			return MineGrid;
		}
	}
	return nullptr;
}

FIntPoint AMinesweeperPlayerControllerBase::GetPawnRelativeLocationOfGrid(APawn* PlayerPawn, AMineGridBase* MineGrid)
{
	// Get relative grid coords of player pawn
	const FVector PawnAbsoluteLocation = PlayerPawn->GetActorLocation();
	const FVector MineGridLocation = MineGrid->GetActorLocation();
	const float CellSize = MineGrid->GetCellSize();

	const FVector PawnRelativeLocation = PawnAbsoluteLocation - MineGridLocation;

	int32 PawnRelativeLocationX = FMath::TruncToInt(PawnRelativeLocation.X);
	int32 PawnRelativeLocationY = FMath::TruncToInt(PawnRelativeLocation.Y);

	FIntPoint PawnRelativeGridCoords = FIntPoint(PawnRelativeLocationX, PawnRelativeLocationY) / CellSize;

	return PawnRelativeGridCoords;
}
