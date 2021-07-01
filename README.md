
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
#include "snprotohash0.h"

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
The `snprotohash0_file_hashing_utility.c` program reads files very naively, and seems to get ≈1.8 GiB/s.

The header `snprotohash0_compact.h` is a slightly shorter and more readable equivalent version that doesn't implement the streaming interface.

If you want the super short keyless version, it's just:

```c
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

#define SNPROTOHASH0_SHUFFLE(feedback_shift) \
    for (int i = 0; i < 8; i++) \
        lanes[i] = _mm_aesdec_si128(lanes[i], lanes[(i + feedback_shift) % 8]);

#define SNPROTOHASH0_ABSORB(block) \
    for (int i = 0; i < 4; i++) \
        lanes[i + 0] = _mm_add_epi64(lanes[i + 0], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(4) SNPROTOHASH0_SHUFFLE(3) SNPROTOHASH0_SHUFFLE(2) \
    for (int i = 0; i < 4; i++) \
        lanes[i + 4] = _mm_add_epi64(lanes[i + 4], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(1) SNPROTOHASH0_SHUFFLE(4) SNPROTOHASH0_SHUFFLE(3)

static void snprotohash0(uint64_t length, void* data, void* hash_output) {
    __m128i lanes[8];
    for (uint64_t i = 0; i < 8; i++)
        lanes[i] = _mm_set_epi64x(i * 0x93c467e37db0c7a4llu, i * 0xd1be3f810152cb56llu);
    uint64_t remaining_length = length;
    while (remaining_length >= 64) {
        SNPROTOHASH0_ABSORB(data)
        data += 64; remaining_length -= 64;
    }
    uint8_t scratch_space[64] = {0};
    if (remaining_length) {
        memcpy(scratch_space, data, remaining_length);
        SNPROTOHASH0_ABSORB(scratch_space)
        memset(scratch_space, 0, 64);
    }
    lanes[0] = _mm_add_epi64(lanes[0], _mm_set_epi64x(0x93c467e37db0c7a4llu, 0xd1be3f810152cb56llu));
    *(uint64_t*) scratch_space = length;
    SNPROTOHASH0_ABSORB(scratch_space) SNPROTOHASH0_ABSORB(scratch_space)
    for (int i = 0; i < 4; i++)
        lanes[i] = _mm_add_epi64(lanes[i], lanes[i + 4]);
    for (int i = 0; i < 2; i++)
        ((__m128i*) hash_output)[i] = _mm_add_epi64(lanes[i], lanes[i + 2]);
}
```

License
=======

Everything in this repository is released into the public domain.

