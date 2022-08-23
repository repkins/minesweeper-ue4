// Fill out your copyright notice in the Description page of Project Settings.


#include "MinesweeperGameStateBase.h"

#include "Net/UnrealNetwork.h"

AMinesweeperGameStateBase::AMinesweeperGameStateBase(): Super()
{

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

	DOREPLIFETIME(AMinesweeperGameStateBase, LevelPassword);
}
