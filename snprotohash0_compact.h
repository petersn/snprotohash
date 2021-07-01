#ifndef SNPROTOHASH0_H
#define SNPROTOHASH0_H

#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

#define SNPROTOHASH0_MAGIC_CONST0 0x93c467e37db0c7a4llu
#define SNPROTOHASH0_MAGIC_CONST1 0xd1be3f810152cb56llu

#define SNPROTOHASH0_SHUFFLE(feedback_shift) \
    for (int i = 0; i < 8; i++) \
        lanes[i] = _mm_aesdec_si128(lanes[i], lanes[(i + feedback_shift) % 8]);

#define SNPROTOHASH0_ABSORB(block) \
    for (int i = 0; i < 4; i++) \
        lanes[i + 0] = _mm_add_epi64(lanes[i + 0], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(4) \
    SNPROTOHASH0_SHUFFLE(3) \
    SNPROTOHASH0_SHUFFLE(2) \
    for (int i = 0; i < 4; i++) \
        lanes[i + 4] = _mm_add_epi64(lanes[i + 4], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(1) \
    SNPROTOHASH0_SHUFFLE(4) \
    SNPROTOHASH0_SHUFFLE(3)

static void snprotohash0(void* key, uint64_t length, void* data, void* hash_output) {
    __m128i lanes[8];
    // Initialize the state.
    for (uint64_t i = 0; i < 8; i++)
        lanes[i] = _mm_set_epi64x(i * SNPROTOHASH0_MAGIC_CONST0, i * SNPROTOHASH0_MAGIC_CONST1);
    if (key != NULL)
        for (int i = 0; i < 8; i++)
            lanes[i] = _mm_add_epi64(lanes[i], ((__m128i*) key)[i % 2]);
    // Absorb every complete 64-byte block.
    uint64_t remaining_length = length;
    while (remaining_length >= 64) {
        if (length > 4096)
            _mm_prefetch(data + 4096, _MM_HINT_T0);
        SNPROTOHASH0_ABSORB(data)
        data += 64; remaining_length -= 64;
    }
    uint8_t scratch_space[64] = {0};
    // Absorb whatever's left padded with zeros.
    if (remaining_length) {
        memcpy(scratch_space, data, remaining_length);
        SNPROTOHASH0_ABSORB(scratch_space)
        memset(scratch_space, 0, 64);
    }
    // Switch to finalization by making a modification that no input block can.
    lanes[0] = _mm_add_epi64(lanes[0],
        _mm_set_epi64x(SNPROTOHASH0_MAGIC_CONST0, SNPROTOHASH0_MAGIC_CONST1));
    // Finalize by absorbing a length block twice.
    *(uint64_t*) scratch_space = length;
    SNPROTOHASH0_ABSORB(scratch_space)
    SNPROTOHASH0_ABSORB(scratch_space)
    // Aggregate down then return.
    for (int i = 0; i < 4; i++)
        lanes[i] = _mm_add_epi64(lanes[i], lanes[i + 4]);
    for (int i = 0; i < 2; i++)
        ((__m128i*) hash_output)[i] = _mm_add_epi64(lanes[i], lanes[i + 2]);
}

#endif
