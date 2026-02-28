# LANChat
## Overview
Fully decentralized peer-to-peer chat application for local area networks. 
No central server required - peers discover each other automatically via UDP 
broadcast and establish direct TCP connections for messaging.

Built using RT-RK FSM (Finite State Machine) library for robust protocol 
state management.

### Architecture

### Two-FSM System
1. **UDP Discovery FSM** (1 instance per client)
   - Broadcasts ALIVE messages to discover peers
   - Responds to discovery with OK messages  
   - Manages 15-second discovery timeout
   - Dispatches TCP connection requests to pool

2. **TCP Connection FSM** (pool of 10 instances)
   - Handles individual peer-to-peer connections
   - Supports up to 10 simultaneous connections
   - Role assignment: ALIVE sender = client, OK sender = server

### Threading Model

**Main Thread:**
- Initializes Winsock
- Creates SystemThread
- Keeps process alive (infinite loop)

**SystemThread:**
- Runs FSM kernel event loop
- Processes FSM messages sequentially
- Manages timers and state transitions

**UdpListenerThread:**
- Receives UDP broadcasts on port 8080
- Converts network packets to FSM messages
- Runs continuously in background

**ConsoleInputThread:**
- Reads user input from stdin
- Broadcasts messages to all active TCP connections
- Started lazily on first connection

**ConnectionWorkerThread** (×10 max):
- Manages individual TCP connection lifecycle
- Server mode: bind() → listen() → accept()
- Client mode: connect() with 5 retry attempts (500ms delays)
- Runs send/receive loop until disconnection
### Port Assignment
TCP ports are deterministically calculated from username (implemented mainly for local testing)

### State Machines

**UDP Discovery FSM States:**
- `LOGIN`: Initial state, username entry
- `IDLE`: Operational state, handles discovery messages, doesn't connect, spawns for each individual connection

**TCP Connection FSM States:**
- `IDLE`: Available in pool, waiting for connection request
- `CONNECTED`: Active TCP connection with peer

