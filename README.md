# LANChat
## Decentralized LAN Chat (FSM-based)

This project implements a decentralized LAN chat application where all clients communicate directly, without a broker or central server.

## Concept

Each client discovers other active clients in the local subnet using UDP discovery.

After discovery, clients establish direct TCP connections with each other.

For every TCP connection, a separate worker thread is created.

Each worker thread owns one FSM instance, which controls the protocol logic for that connection.

FSMs handle connection setup, message exchange, and un/graceful shutdown.

Messages are broadcast to all connected peers in the subnet and displayed with sender identification.

## Architecture

### Main thread

Parses command-line parameters (client name)

Handles UDP discovery

Accepts incoming TCP connections

Spawns worker threads

### Worker thread

Manages one TCP connection

Runs a dedicated FSM instance

## FSM

Implements protocol states and transitions

Reacts to events such as message received, send request, and disconnect

One FSM instance per TCP connection, they represent the state of the thread itself, not the client 

FSM communicates with the network via win system support

