// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameModeBase.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Minesweeper/Player/MinesweeperPlayerControllerBase.h"
#include "MinesweeperGameStateBase.h"

const FIntPoint AMinesweeperGameModeBase::DefaultCellCoords(-1, -1);

AMinesweeperGameModeBase::AMinesweeperGameModeBase(): Super()
{
	MinesweeperBackend = CreateDefaultSubobject<UMinesweeperBackendComponent>(TEXT("MinesweeperBackendComponent"));

	// Setting defaults
	LastSentCoordsToOpen = AMinesweeperGameModeBase::DefaultCellCoords;
	MineGridMapVersion = 0;
	bIsGameOver = false;
	bTriggeredCellProcessing = false;

	// Setting up backend delegate handlers
	MinesweeperBackend->OnMapOk.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnMapOk);
	MinesweeperBackend->OnOpenOk.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnOpenOk);
	MinesweeperBackend->OnOpenGameOver.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnOpenGameOver);
	MinesweeperBackend->OnOpenYouWinDelegate.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnOpenYouWin);
	MinesweeperBackend->OnNewGameOk.AddDynamic(this, &AMinesweeperGameModeBase::HandleOnNewGameOk);
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
	}
}

void AMinesweeperGameModeBase::HandleOnNewGameOk()
{
}

void AMinesweeperGameModeBase::HandleOnMapOk(const FMineGridMap& NewMineGridMap)
{
	MineGridMapVersion += 1;
}

void AMinesweeperGameModeBase::HandleOnOpenOk()
{
	HandleOnOpen();
}

void AMinesweeperGameModeBase::HandleOnOpenGameOver()
{
	bIsGameOver = true;

	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->SetExplodedCell(LastSentCoordsToOpen);
	}

	for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (auto MinespeeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
		{
			MinespeeperPlayer->NotifyGameOver();
		}
	}

	HandleOnOpen();
}

void AMinesweeperGameModeBase::HandleOnOpenYouWin(const FString& LevelPassword)
{
	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->SetLevelPassword(LevelPassword);
	}

	for (FConstPlayerControllerIterator PlayerIt = GetWorld()->GetPlayerControllerIterator(); PlayerIt; ++PlayerIt)
	{
		if (auto MinespeeperPlayer = Cast<AMinesweeperPlayerControllerBase>(*PlayerIt))
		{
			MinespeeperPlayer->NotifyGameWin();
		}
	}

	HandleOnOpen();
}

void AMinesweeperGameModeBase::HandleOnOpen()
{
	MinesweeperBackend->SendMapCommand();

	if (bIsGameOver)
	{
		RemainingCellsToProcess.Empty();
	}

	FIntPoint NextTriggeredCoords;
	if (RemainingCellsToProcess.Dequeue(NextTriggeredCoords))
	{
		LastSentCoordsToOpen = NextTriggeredCoords;
		MinesweeperBackend->SendOpenCellCommand(NextTriggeredCoords);
	}
	else
	{
		bTriggeredCellProcessing = false;
	}
}

void AMinesweeperGameModeBase::HandleOnPlayerNewGame(const uint8 MapSize)
{
	bIsGameOver = false;
	MineGridMapVersion = 0;

	if (AMinesweeperGameStateBase* MinesweeperState = GetGameState<AMinesweeperGameStateBase>())
	{
		MinesweeperState->UnsetExplodedCell();
	}

	MinesweeperBackend->SendNewGameCommand(MapSize);
	MinesweeperBackend->SendMapCommand();
}

void AMinesweeperGameModeBase::HandleOnPlayerTriggeredCoords(const FIntPoint& EnteredCoords)
{
	if (!bIsGameOver)
	{
		if (!bTriggeredCellProcessing)
		{
			LastSentCoordsToOpen = EnteredCoords;

			MinesweeperBackend->SendOpenCellCommand(EnteredCoords);
			bTriggeredCellProcessing = true;
		}
		else
		{
			RemainingCellsToProcess.Enqueue(EnteredCoords);
		}
	}
}
