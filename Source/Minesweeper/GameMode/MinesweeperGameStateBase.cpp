// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameStateBase.h"

#include "Net/UnrealNetwork.h"

AMinesweeperGameStateBase::AMinesweeperGameStateBase(): Super()
{
	bHasExplodedCell = false;
}

FString AMinesweeperGameStateBase::GetLevelPassword()
{
	return LevelPassword;
}

bool AMinesweeperGameStateBase::HasExplodedCell()
{
	return bHasExplodedCell;
}

FIntPoint AMinesweeperGameStateBase::GetExplodedCoords()
{
	return ExplodedCoords;
}

void AMinesweeperGameStateBase::SetLevelPassword(const FString& NewLevelPassword)
{
	LevelPassword = NewLevelPassword;
}

void AMinesweeperGameStateBase::SetExplodedCell(const FIntPoint& NewExplodedCoords)
{
	ExplodedCoords = NewExplodedCoords;
	bHasExplodedCell = true;
}

void AMinesweeperGameStateBase::UnsetExplodedCell()
{
	bHasExplodedCell = false;
}

void AMinesweeperGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMinesweeperGameStateBase, bHasExplodedCell);
	DOREPLIFETIME(AMinesweeperGameStateBase, ExplodedCoords);

}
