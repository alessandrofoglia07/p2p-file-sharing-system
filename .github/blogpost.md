## Introduction

_Before talking about my experience, I wanted to specify that this post can be read as both
a **report of my personal experience** and a **tutorial for implementing the Chord protocol in C**. Enjoy your read!_

I recently created a Peer-to-Peer file sharing system (you can find the full
repo [here](https://github.com/alessandrofoglia07/p2p-file-sharing-system) and I'd really appreciate if you gave it a
⭐ :D) in C and I wanted to share some details about why I started this project, how I managed to implement it and how it
made me a better coder overall.

## The idea

I'm a high-school student, a Web Developer and specifically a Cloud computing enthusiast. For quite some time now I've
begun approaching to the C language, in order to really understand how computers work at the _low level_ and how to work
with them effectively. For this reason, I decided to start diving into some low level _networking_ projects (I also
created a [HTTP protocol implementation](https://github.com/alessandrofoglia07/minimalist-http-server) and a
simple [chat app](https://github.com/alessandrofoglia07/chat-application-c) in C) and surely this one was the hardest of
them all!

Initially, I began implementing a distributed network of unordered peers as I was able to do with the knowledge I got by
coding the aforesaid projects. Soon, however, I realized that the approach I unknowingly chose was extremely inefficient
and wouldn't work for large networks. Therefore I started to search for a more optimized solution and I stumbled upon
Distributed Hash Tables (DHTs) and the Chord algorithm. As a professional-high-schooler, the first thing I did was
asking ChatGPT "_What are the benefits of implementing a DHT into a p2p application?_"... it answered me with big words
like "_decentralization_", "_fault tolerance_", "_efficiency_" and "_robustness_"; however it also mentioned a
downside: "_complexity in implementation_". And I can say, as a student with little to no ability in implementing even
simple data structures, that it was in fact telling the truth 😅.

## The actual implementation

### DHTs

So, exactly.. _what is a distributed hash table_?
As the name says, a distributed hash table (DHT) is
a [hash table](https://www.geeksforgeeks.org/hash-table-data-structure/) which is distributed across various nodes in
the network. By utilizing [consistent hashing](https://en.wikipedia.org/wiki/Consistent_hashing), any node can lookup
the location (e.g. IP address and port) of the node that contains the value of a certain key (then contact it and get
the value).

### Introduction to Chord algorithm

Then, how does Chord effectively create a DHT?
[Chord](https://en.wikipedia.org/wiki/Chord_(peer-to-peer)) (an algorithm introduced in 2001 as one of the four original
DHT protocols) creates a ring shaped network in which each node has a specific position based on an ID, generated
through an hash function (I personally used the [SHA-1](https://en.wikipedia.org/wiki/SHA-1) function). Each node has a
successor and a predecessor: the successor is the next node in the ring going in clockwise direction, the predecessor is
counter-clockwise. The maximum amount of nodes in the ring is 2<sup>M</sup> where M is the amount of bits in the ID (if
you're using the SHA-1 function, M is 160).

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

The final aim of the system is to implement a `find_successor(hash(key))` function to lookup the location of a key in
the DHT. The function would return the location of the node (peer) responsible for that key.

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
For lookup, Chord algorithm suggests what's called a _finger table_, a table containing M nodes' data, to avoid a linear
search in favor of a faster method.

### My Own Node/Data implementation

_From this point, I will display both the pseudocode and my C implementation_

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

In my implementation, each node only stores file metadata, in order for a peer looking for a file to download it
directly from the node who decided to share it originally. Here's how I structured my lookup flow:

```
- Node (A) requests file to responsible node (B)
- B responds with FileEntry object (sends it back to A)
- A contacts C (at file.owner_ip:file.owner_port) and downloads the file.
```