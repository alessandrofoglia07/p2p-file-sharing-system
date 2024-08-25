<h1 align="center">
  <br>
  Peer-to-Peer File Sharing System
</h1>

<h4 align="center">Peer-to-Peer File Sharing System written in C</h4>

<p align="center">
  <a href="#key-features">Key Features</a> •
  <a href="#how-to-use">How To Use</a> •
  <a href="#license">License</a>
</p>

## Key Features

- **Distributed Hash Table (DHT) Implementation**: Utilizes
  the [Chord](https://en.wikipedia.org/wiki/Chord_(peer-to-peer)) protocol for efficient and scalable
  peer-to-peer
  file lookup and storage.

- **Dynamic Ring Formation**: Supports both the creation of new rings and joining existing ones, allowing nodes to
  connect
  and disconnect without disrupting the system.

- **File Storage and Retrieval**: Enables decentralized file storage and retrieval with automatic assignment of files to
  responsible nodes based on their hashed identifiers.

- **Efficient Successor and Predecessor Management**: Periodic stabilization routines ensure consistent and correct
  successor and predecessor pointers, maintaining the integrity of the ring.

- **Fault Tolerance**: Nodes periodically send heartbeat messages to detect and handle node failures, ensuring high
  availability and reliability.

- **Threaded Node Management**:
  Utilizes [multi-threading](https://en.wikipedia.org/wiki/Multithreading_(computer_architecture)) to manage node
  activities, such as handling incoming
  connections,
  stabilizing the ring, and processing user commands concurrently.

- **Command Line Interface**: Provides a simple and intuitive command-line interface for node management, file
  operations,
  and network communication.

- **SHA-1 Based Hashing**: Uses [SHA-1](https://en.wikipedia.org/wiki/SHA-1) hashing for generating unique file
  identifiers and node IDs, ensuring efficient
  and
  collision-resistant key distribution.

- **Graceful Shutdown**: Ensures all active threads and connections are properly terminated upon exit, preserving the
  system's stability and preventing data loss.

## How To Use

To clone and run this application, you'll need [Git](https://git-scm.com) and [CMake](https://cmake.org/)
installed on your computer. From your command line:

```bash
# Clone this repository
$ git clone https://github.com/alessandrofoglia07/p2p-file-sharing-system

# Go into the repository
$ cd p2p-file-sharing-system

# Build the program
$ cmake --build ./cmake-build-debug --target p2p_file_sharing_system -- -j 6

# Run the program (entry_ip and entry_port are optional, used for joining an existing ring)
$ ./build/p2p_file_sharing_system <ip> <port> <entry_ip> <entry_port>
```

## License

MIT

---

> GitHub [@alessandrofoglia07](https://github.com/alessandrofoglia07) &nbsp;&middot;&nbsp;
> StackOverflow [@Alexxino](https://stackoverflow.com/users/21306952/alexxino) &nbsp;&middot;&nbsp;
> NPM [alessandrofoglia07](https://www.npmjs.com/~alessandrofoglia07)
