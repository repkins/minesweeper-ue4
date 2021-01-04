// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameStateBase.h"

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
