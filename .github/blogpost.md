## Introduction

_Before talking about my experience, I wanted to specify that this post can be read as both a **report of my personal experience** and a **tutorial for implementing the Chord protocol in C**. Enjoy your read!_

I recently created a Peer-to-Peer file sharing system (you can find the full repo [here](https://github.com/alessandrofoglia07/p2p-file-sharing-system) and I'd really appreciate if you gave it a ⭐ :D) in C and I wanted to share some details about why I started this project, how I managed to implement it and how it made me a better coder overall.

## The idea

I'm a high-school student, a Web Developer and specifically a Cloud computing enthusiast. For quite some time now I've begun approaching to the C language, in order to really understand how computers work at the _low level_ and how to work with them effectively. For this reason, I decided to start diving into some low level _networking_ projects (I also created a [HTTP protocol implementation](https://github.com/alessandrofoglia07/minimalist-http-server) and a simple [chat app](https://github.com/alessandrofoglia07/chat-application-c) in C) and surely this one was the hardest of them all!

Initially, I began implementing a distributed network of unordered peers as I was able to do with the knowledge I got by coding the aforesaid projects. Soon, however, I realized that the approach I unknowingly chose was extremely inefficient and wouldn't work for large networks. Therefore I started to search for a more optimized solution and I stumbled upon Distributed Hash Tables (DHTs) and the Chord algorithm. As a professional-high-schooler, the first thing I did was asking ChatGPT "_What are the benefits of implementing a DHT into a p2p application?_"... it answered me with big words like "_decentralization_", "_fault tolerance_", "_efficiency_" and "_robustness_"; however it also mentioned a downside: "_complexity in implementation_". And I can say, as a student with little to no ability in implementing even simple data structures, that it was in fact telling the truth 😅.

## The actual implementation

### DHTs

So, exactly... _what is a distributed hash table_?<br> As the name says, a distributed hash table (DHT) is a [hash table](https://www.geeksforgeeks.org/hash-table-data-structure/) which is distributed across various nodes in the network. By utilizing [consistent hashing](https://en.wikipedia.org/wiki/Consistent_hashing), any node can lookup the location (e.g. IP address and port) of the node that contains the value of a certain key (then contact it and get the value).

### Introduction to Chord algorithm

Then, how does Chord effectively create a DHT?<br> [Chord](https://en.wikipedia.org/wiki/Chord_(peer-to-peer)) (an algorithm introduced in 2001 as one of the four original DHT protocols) creates a ring shaped network in which each node has a specific position based on an ID, generated through an hash function (I personally used the [SHA-1](https://en.wikipedia.org/wiki/SHA-1) function). Each node has a successor and a predecessor: the successor is the next node in the ring going in clockwise direction, the predecessor is counter-clockwise. The maximum amount of nodes in the ring is 2<sup>M</sup> where M is the amount of bits in the ID (if you're using the SHA-1 function, M is 160).

```
        Hash(Key) = 125
           ↓
    n1 ---------- n2 
  (100)          (200)
    |              |
    |              |
    |              |
    n4 ---------- n3
  (400)          (300)
```

The final aim of the system is to implement a `find_successor(hash(key))` function to lookup the location of a key in the DHT. The function would return the location of the node (peer) responsible for that key.

```
    Hash(Key) FindSuccessor(Hash(Key))
        ↓         ↓
    n1 ---------- n2 
  (100)          (200)
    |              |
    |              |
    |              |
    n4 ---------- n3
  (400)          (300)
```

_Credits for these cool illustrations go to [@arush15june](https://github.com/arush15june)_
For lookup, Chord algorithm suggests what's called a _finger table_, a table containing M nodes' data, to avoid a linear search in favor of a faster method.

### My Own Node/Data implementation

_From this point, I will display the pseudocode and, below, a simplified version of my C implementation_

Specifically, here's how I structured my `Node`:

```c
typedef struct Node {
    uint8_t id[20]; // SHA-1 hash
    char ip[16]; // e.g. 127.0.0.1
    int port; // e.g. 5000
    struct Node *successor; // node's successor
    struct Node *predecessor; // node's predecessor
    struct Node *finger[M]; // array of nodes (finger table)
    struct FileEntry *files; // files which the node is responsible for
    struct FileEntry *uploaded_files; // files uploaded by the node
    int sockfd; // file descriptor of node's socket
} Node;
```

and here's the `FileEntry` struct:

```c
typedef struct FileEntry {
    uint8_t id[20]; // hash key of the file (used for lookup)
    char filename[256];
    char filepath[512];
    char owner_ip[16];
    int owner_port;
    struct FileEntry *next;
} FileEntry;
```

In my implementation, each node only stores file metadata, in order for a peer looking for a file to download it directly from the node who decided to share it originally. Here's how I structured my lookup flow:

```
- Node (A) requests file to responsible node (B)
- B responds with FileEntry object (sends it back to A)
- A contacts C (at file.owner_ip:file.owner_port) and downloads the file.
```

### The `find_successor` Function

As I previously said, the most important function of the algorithm is the `find_successor` function. The function initially checks if the responsible node is the local one, by comparing its ID to its successor's ID and to the given ID. If it isn't, the node starts to search the local finger table for the highest predecessor of ID.

Here's the code for the `find_successor` function:

```
n.find_successor(id)
    // It is a half closed interval.
    if id ∈ (n, successor] then
        return successor
    else
        // forward the query around the circle
        n0 := closest_preceding_node(id)
        return n0.find_successor(id)

n.closest_preceding_node(id)
    for i = m downto 1 do
        if (finger[i] ∈ (n, id)) then
            return finger[i]
    return n
```

```c
// checks if id is in the interval (a, b)
int is_in_interval(const uint8_t *id, const uint8_t *a, const uint8_t *b) {
    if (memcmp(a, b, HASH_SIZE) < 0) {
        return memcmp(id, a, HASH_SIZE) > 0 && memcmp(id, b, HASH_SIZE) < 0;
    }
    return memcmp(id, a, HASH_SIZE) > 0 || memcmp(id, b, HASH_SIZE) < 0;
}

Node *find_successor(Node *n, const uint8_t *id) {
    if (memcmp(id, n->id, HASH_SIZE) > 0 && memcmp(id, n->successor->id, HASH_SIZE) <= 0) {
        return n->successor;
    }

    const Node *n0 = closest_preceding_node(n, id);

    // Avoid sending message to itself
    if (n0 == n) {
        return n->successor;
    }

    // send a FIND_SUCCESSOR message to n0
    send_message(n, n0->ip, n0->port, "FIND_SUCCESSOR");

    // receive the successor's info
    const Node response = receive_message();

    Node *successor = n->successor;
    memcpy(successor->id, response->id, HASH_SIZE);
    strcpy(successor->ip, response->ip);
    successor->port = response->port;

    return successor;
}

Node *closest_preceding_node(Node *n, const uint8_t *id) {
    // search the finger table in reverse order to find the closest node
    for (int i = M - 1; i >= 0; i--) {
        if (is_in_interval(n->finger[i]->id, n->id, id)) {
            return n->finger[i];
        }
    }
    return n;
}
```

### Create/Join a Network

A node can create a new ring by setting its predecessor to nil/NULL and its successor to itself, like this:

```
n.create()
    predecessor := nil
    successor := n
```

```c
void create_ring(Node *n) {
    n->predecessor = NULL;
    n->successor = n;
}
```

If a node wants to join an existing ring it needs to know the location of at least another node in that network.

```
n.join(n0)
    predecessor := nil
    successor := n0.find_successor(n)
```

```c
void join_ring(Node *n, const char *existing_ip, const int existing_port) {
    // send a JOIN message to the existing node
    send_message(n, existing_ip, existing_port, "JOIN");

    // the existing node will respond with the new node's successor
    Message response = receive_message();

    // create a new node for the successor
    Node *successor = create_node(response->ip, response->port);

    // set the new node's successor to the one returned by the existing node
    n->predecessor = NULL;
    n->successor = successor;
}
```

### The Stabilization Process

At this point, the algorithm stumbles upon a problem. We need a way to ensure that the structure of the network remains the same, even after a node joins or leaves. The network has to be dynamically updated to let nodes join the network at will and stale nodes be removed from the network and stabilize the ring again. We can solve this through a stabilization routine. We need to regularly verify that:

- the current successor is still its successor and no new node has joined in between them;
- the predecessor is still alive;
- the finger table is updated.

We can do this with the functions `fix_fingers()`, `stabilize()`, `notify()` and `check_predecessor()`:

#### `fix_fingers()`

```
// called periodically. refreshes finger table entries.
// next stores the index of the finger to fix
n.fix_fingers()
    next := next + 1
    if next > m then
        next := 1
    finger[next] := find_successor(n+2next-1);
```

```c
void fix_fingers(Node *n, int *next) {
    *next = (*next + 1) % M;
    Node *finger = find_successor(n, n->finger[*next]->id);
    n->finger[*next] = finger;
}
```
