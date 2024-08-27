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