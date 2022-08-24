// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameStateBase.h"

#include "Net/UnrealNetwork.h"

AMinesweeperGameStateBase::AMinesweeperGameStateBase(): Super()
{
	NumUndiscoveredClearCells = 0;
	LevelPassword = TEXT("");
}

int32 AMinesweeperGameStateBase::GetNumUndiscoveredClearCells()
{
	return NumUndiscoveredClearCells;
}

void AMinesweeperGameStateBase::SetNumUndiscoveredClearCells(const int32 NewNumUndiscoveredClearCells)
{
	NumUndiscoveredClearCells = NewNumUndiscoveredClearCells;
}

FString AMinesweeperGameStateBase::GetLevelPassword()
{
	return LevelPassword;
}
void AMinesweeperGameStateBase::SetLevelPassword(const FString& NewLevelPassword)
{
	LevelPassword = NewLevelPassword;
}

void AMinesweeperGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMinesweeperGameStateBase, NumUndiscoveredClearCells);
	DOREPLIFETIME(AMinesweeperGameStateBase, LevelPassword);
}
