# TaxiCab Game

This project was completed during my Bachelor in Computer Science at the University of Turin in the course Operating Systems.

The theme it's a simulation of a city with a taxi service.

The city map is dived into **cells**: each cell can be free or occupied (if for example there's a building). At the beginning of the game
a new random map is generated (the whole maps is implemented in shared memory).

Once the maps is generated, multiple **taxis** (represented as processes) are created and initialized in a random free cell on the map.
At the same, multiple **customers** processes are spawned randomly on a map cell.

Once everything is intialized, the master process (represented by the `main` file) starts the simulation by raising to one the shared mutex on which all the child process are waiting.

## Simulation cycle

- the simulation is started by the `master` process
- each `customer` process calls a `taxi` by putting the request (the coordinates of the cell in which he's standing and the destination cell -also random-) in a shared message queue
- in an atomic way, each `taxi` serves a request
- once a `taxi` gaines a customer request, it computes the shortest path to reach the customer using an implementation of the Dijkstra's algorithm
- once a path is obtained, the `taxi` starts to navigate to the customer
- each map cells has a `SO_CAP_MAX` limit of processes on a certain cell (configurable in the `Parameters.txt` file), if a cell on which a `taxi` needs to pass is full, it needs to wait for a slot to free (also in an atomic way)
- once the time limit is reached, the  `master` process stops the simulation, frees the occupied resources and prints some statistics
