# Minesweeper (multiplayer-enabled)

Developed with Unreal Engine 4

# Prerequisites

1. Unreal Engine 4.26
2. Windows 10 x64

# How-to play

1. Start the game (1 up to 3 clients), either of: one client as listenserver, or run server separately
2. Any client can open "main" menu by invoking "New Game" action (default is one of "ESC" and "Q" keyboard keys) and can click "Resume" to close menu or click "Quit" to exit his own instance of client
3. Lobby leader (first connected client) can select one of possible map sizes in "main" menu by clicking on one of "New #" buttons to start a new match
5. All clients can start opening cells by moving their possesed characters onto them using movement keyboard keys:
    - "W" for forward direction
    - "S" for backward direction
    - "A" for left direction
    - "D" for right direction
6. All clients can also make their characters to jump over cells, untriggering them, by using "Jump" action (default is "Space" keyboard key)
7. Lobby leader can start new match at any time by using the same "New Game" action to open "main" menu.
8. All clients continue opening cells until one of the following:
    - mines in untriggered cells became revealed and showing "You Win" HUD overlay for all clients;
    - or until last triggered cell become reddish color and revealed and showing "Game Over" HUD overlay for all clients
9. In either of two game ends all clients depending of lobby role within session have the following options by clicking on related buttons:
    - lobby leader: start a new game (opens "main" menu)
    - all clients: quit

# Solution Description
    
Solution architecture is networking ready, designed with networking in mind. It features added replication, different kinds of RPCs (remote procedure calls), authority checks. All match progress logic is contained in it's own derived authoritive-priviled game mode class, inheritating accessibility only by one having server role (listenserver or dedicated) to prevent cheating. It also contains RPC-enabled cells remote-streaming system, where only server knows about every cell state for every client and clients does not store state of cells outside of clients viewports.

Implemented the following units:
1. `MinesweeperGameMode` class is resposible for match control. Communicates only with PlayerControllers and updates values in GameState about match state.
2. `MinesweeperPlayerController` class is responsible of controlling the character, contains action bindings to methods, communicates with `GameMode` for match and cell opening updates. Uses `MineGrid` actor as a representation container of cells, telling him what exactly to represent and listening of "cell triggering" events.
3. `MineGrid` class is a representational container of cells which defines the root location of cells in the game world. Spawns or destroys cell actors in response of player controller, responds to player controller about character triggering coordinates by listening to cells.
4. `MineGridCell` class is a physical representation of a single cell visible in the viewport and interactable with them by player. Contains trigger box to listen for character overlapping events to initiate cell triggering event broadcast chain (up through `MineGrid` and `PlayerController` until `GameMode`). Responds to cell value updates by it's `MineGrid` container actor to update visual representation of it.

Grid, Grid Cells, GameMode and PlayerController core logic are implemented natively in C++ with the possibility in blueprints to:
- change property values or references to assets;
- invoking native methods;
- listening to Blueprint events, overriding native implemention of them.
- 
