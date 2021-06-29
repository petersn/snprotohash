#ifndef SNPROTOHASH0_H
#define SNPROTOHASH0_H

#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

#define SNPROTOHASH0_BLOCK_SIZE   64
#define SNPROTOHASH0_MAGIC_CONST0 0x93c467e37db0c7a4llu
#define SNPROTOHASH0_MAGIC_CONST1 0xd1be3f810152cb56llu

#define SNPROTOHASH0_SHUFFLE(feedback_shift) \
    for (int i = 0; i < 8; i++) \
        sh->lanes[i] = _mm_aesdec_si128(sh->lanes[i], sh->lanes[(i + feedback_shift) % 8]);

#define SNPROTOHASH0_ABSORB(block) \
    for (int i = 0; i < 4; i++) \
        sh->lanes[i + 0] = _mm_add_epi64(sh->lanes[i + 0], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(4) \
    SNPROTOHASH0_SHUFFLE(3) \
    SNPROTOHASH0_SHUFFLE(2) \
    for (int i = 0; i < 4; i++) \
        sh->lanes[i + 4] = _mm_add_epi64(sh->lanes[i + 4], ((__m128i*) (block))[i]); \
    SNPROTOHASH0_SHUFFLE(1) \
    SNPROTOHASH0_SHUFFLE(4) \
    SNPROTOHASH0_SHUFFLE(3)

typedef struct snprotohash0_t {
    __m128i lanes[8];
    uint8_t scratch_space[SNPROTOHASH0_BLOCK_SIZE];
    uint64_t length;
} snprotohash0_t;

// Takes in a 256-bit (32-byte) key.
static void snprotohash0_init(snprotohash0_t* sh, void* key) {
    for (uint64_t i = 0; i < 8; i++)
        sh->lanes[i] = _mm_set_epi64x(i * SNPROTOHASH0_MAGIC_CONST0, i * SNPROTOHASH0_MAGIC_CONST1);
    if (key != NULL)
        for (int i = 0; i < 8; i++)
            sh->lanes[i] = _mm_add_epi64(sh->lanes[i], ((__m128i*) key)[i % 2]);
    sh->length = 0;
}

static void snprotohash0_update(snprotohash0_t* sh, uint64_t length, void* data) {
    uint64_t scratch_fill = sh->length % SNPROTOHASH0_BLOCK_SIZE;
    sh->length += length;
    // Deal with any residual partial block from last time.
    if (scratch_fill) {
        if (scratch_fill + length < SNPROTOHASH0_BLOCK_SIZE) {
            memcpy(sh->scratch_space + scratch_fill, data, length);
            return;
        }
        memcpy(sh->scratch_space + scratch_fill, data, SNPROTOHASH0_BLOCK_SIZE - scratch_fill);
        SNPROTOHASH0_ABSORB(sh->scratch_space)
        data += SNPROTOHASH0_BLOCK_SIZE - scratch_fill;
        length -= SNPROTOHASH0_BLOCK_SIZE - scratch_fill;
    }
    // Hash any complete blocks.
    while (length >= SNPROTOHASH0_BLOCK_SIZE) {
        if (length > 4096)
            _mm_prefetch(data + 4096, _MM_HINT_T0);
        SNPROTOHASH0_ABSORB(data)
        data += SNPROTOHASH0_BLOCK_SIZE;
        length -= SNPROTOHASH0_BLOCK_SIZE;
    }
    // Stash whatever is left over for next time.
    memcpy(sh->scratch_space, data, length);
}

// Returns a 256-bit (32-byte) hash.
static void snprotohash0_end(snprotohash0_t* sh, void* hash_output) {
    // Absorb a padded block, if necessary.
    uint64_t scratch_fill = sh->length % SNPROTOHASH0_BLOCK_SIZE;
    if (scratch_fill) {
        memset(sh->scratch_space + scratch_fill, 0, SNPROTOHASH0_BLOCK_SIZE - scratch_fill);
        SNPROTOHASH0_ABSORB(sh->scratch_space)
    }
    // Switch to finalization by making a modification that no input block can.
    sh->lanes[0] = _mm_add_epi64(sh->lanes[0],
        _mm_set_epi64x(SNPROTOHASH0_MAGIC_CONST0, SNPROTOHASH0_MAGIC_CONST1));
    // Finalize by absorbing a length block twice.
    memset(sh->scratch_space, 0, 64);
    *(uint64_t*) sh->scratch_space = sh->length;
    SNPROTOHASH0_ABSORB(sh->scratch_space)
    SNPROTOHASH0_ABSORB(sh->scratch_space)
    // Aggregate down then return.
    for (int i = 0; i < 4; i++)
        sh->lanes[i] = _mm_add_epi64(sh->lanes[i], sh->lanes[i + 4]);
    for (int i = 0; i < 2; i++)
        ((__m128i*) hash_output)[i] = _mm_add_epi64(sh->lanes[i], sh->lanes[i + 2]);
}

static void snprotohash0(void* key, uint64_t length, void* data, void* hash_output) {
    snprotohash0_t sh;
    snprotohash0_init(&sh, key);
    snprotohash0_update(&sh, length, data);
    snprotohash0_end(&sh, hash_output);
}

#endif
