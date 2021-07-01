
Experimental fast hash functions
================================

**A collection of single-header fast hash functions.**

Build the corresponding hashing utilities with:
```
./build.sh
```

snprotohash0
------------

snprotohash0 is a keyed hash function, taking in a 256-bit key, and producing a 256-bit output.
The goal is 256-bit security as a cryptographic hash function.

To use:
```c
#include "snprotohash.h"

// Simple interface.
// The NULL argument is an optional pointer to a 256-bit (32-byte) key for generating MACs.
const char* data = "Hello, world.";
char hash_result[32];
snprotohash0(NULL, strlen(data), data, hash_result);

// Streaming interface.
snprotohash0_t ctx;
snprotohash0_init(&ctx, "optional key for generating MACs");
snprotohash0_update(&ctx, strlen(data), data);
snprotohash0_update(&ctx, strlen(data), data);
...
snprotohash0_end(&ctx, hash_result);
```

Runs at ≈3.1 GiB/s on my laptop (i7-10750H @ 2.60GHz), with 130 cycles for a short (one byte) message.
(Compare to SipHash-2-4, which gets ~1.5 GiB/s on my laptop, and takes about 30 cycles for a one byte message.)
The `snprotohash0` program reads files very naively, and seems to get ≈1.8 GiB/s.

The header `snprotohash_compact.h` is a slightly shorter and more readable equivalent version that doesn't implement the streaming interface.

License
=======

Everything in this repository is released into the public domain.

