// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#pragma once

#include <immintrin.h>

#include "types.h"

#include <chrono>
#include <cstring>
#include <string>
#include <algorithm>

// Here comes some SIMD functions replaced by some easy-to-use names
// In most cases, as cpus for home are fully advanced
// therefore, in most cases, only avx512 and avx2 (a bit slower than avx512) are recommended
#if defined(__AVX512F__) && defined(__AVX512BW__)
using vec_t = __m512i;
#define BIT_ALIGNMENT 512
#define vec_load(a) _mm512_load_si512(a)
#define vec_store(a, b) _mm512_store_si512(a, b)
#define vec_add_16(a, b) _mm512_add_epi16(a, b)
#define vec_sub_16(a, b) _mm512_sub_epi16(a, b)
#define vec_mul_16(a, b) _mm512_mullo_epi16(a, b)
#define vec_zero() _mm512_setzero_epi32()
#define vec_setzero() _mm512_setzero_si512()
#define vec_set_16(a) _mm512_set1_epi16(a)
#define vec_set_32(a) _mm512_set1_epi32(a)
#define vec_max_16(a, b) _mm512_max_epi16(a, b)
#define vec_max_32(a, b) _mm512_max_epi32(a, b)
#define vec_min_32(a, b) _mm512_min_epi32(a, b)
#define vec_cvt_8(a) _mm512_cvtepi32_epi8(a)
#define vec_srai_16(a, b) _mm512_srai_epi16(a, b)
#define vec_packs_16(a, b) _mm512_packs_epi16(a, b)
#define REG_NUM 16
#define CHUNK_SIZE 32
#define vec_nnz(a) _mm512_cmpgt_epi32_mask(a, _mm512_setzero_si512())

#elif defined(__AVX2__) || defined(__AVX__)
using vec_t = __m256i;
#define BIT_ALIGNMENT 256
#define vec_load(a) _mm256_load_si256(a)
#define vec_store(a, b) _mm256_store_si256(a, b)
#define vec_add_16(a, b) _mm256_add_epi16(a, b)
#define vec_sub_16(a, b) _mm256_sub_epi16(a, b)
#define vec_mul_16(a, b) _mm256_mullo_epi16(a, b)
#define vec_zero() _mm256_setzero_si256()
#define vec_setzero() _mm256_setzero_si256()
#define vec_set_16(a) _mm256_set1_epi16(a)
#define vec_set_32(a) _mm256_set1_epi32(a)
#define vec_max_16(a, b) _mm256_max_epi16(a, b)
#define vec_max_32(a, b) _mm256_max_epi32(a, b)
#define vec_min_32(a, b) _mm256_min_epi32(a, b)
#define vec_cvt_8(a) _mm256_cvtepi32_epi8(a)
#define vec_srai_16(a, b) _mm256_srai_epi16(a, b)
#define vec_packs_16(a, b) _mm256_packs_epi16(a, b)
#define REG_NUM 16
#define CHUNK_SIZE 32
#define vec_nnz(a) _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(a, _mm256_setzero_si256())))
#endif

// Seraphina NNUE Format: Seraphina-[SHA256 first 12 digits]-version.nnue
#define Seraphina_NNUE "S6.nnue"

// 3072 -> 16 -> 32 -> 1 NNUE Structure
#define KING_BUCKETS 32
#define FEATURES (KING_BUCKETS * 12 * 64)
#define HIDDEN 1536
#define L1 (HIDDEN * 2)
#define L2 16
#define L3 32
#define OUTPUT 1

#define NNUE_SIZE ((FEATURES * HIDDEN + HIDDEN) * sizeof(int16_t) + (L1 * L2 + L2 * L3 + L3) * sizeof(int8_t) + (L2 + L3 + 1) * sizeof(int32_t))

#ifdef DUAL_NNUE
#define Seraphina_NNUE_SMALL "Seraphina-s-v1.nnue"

#ifdef _MSC_VER
const unsigned char        gNNUESmallData[1] = { 0x0 };
const unsigned char* const gNNUESmallEnd = &gEmbeddedNNUESmallData[1];
const unsigned int         gNNUESmallSize = 1;
#else
INCBIN(NNUESmall, Seraphina_NNUE_SMALL);
#endif

// NNUE Dual NNUE 128 -> 16 -> 32 -> 1
#define HIDDEN_SMALL 64
#define L1_SMALL (HIDDEN_SMALL * 2)
#endif

class Board;

namespace Seraphina
{
	namespace NNUE
	{
        struct Delta
        {
            int16_t added[32];
            int16_t removed[32];
            int a, r;
        };

        struct alignas(64) Accumulator
        {
            int16_t acc[2][HIDDEN];
            bool computed[Color::NO_COLOR];

            bool RequireRefresh(Move& move, int pov);
            bool updatable(Accumulator* acc, Move& move, int pov);

            void add_accumulator(int16_t* input, int16_t* output, Delta* delta);
        };

        // Finny Table
        struct alignas(64) AccumulatorRefreshTableEntry
        {
            Accumulator acc;
            U64 colorpiece[Color::NO_COLOR][PieceList::NO_PIECE + 1];
        };

        struct alignas(64) AccumulatorRefreshTable
        {
            AccumulatorRefreshTableEntry table[SQ_NUM];

            void ApplyRefresh(Board& board, int pov);
            void ResetTable();
        };

        void* aligned_malloc(size_t size, size_t alignment);

        void init();
        void load_external(std::string& path);
        void update_accumulator(Board& board, const Move& move, int pov);
        void reset_accumulator(Board& board, int pov);
        void L1_forward(int8_t* input, uint8_t* output);
        void L2_forward(uint8_t* input, uint8_t* output);
        void L3_forward(uint8_t* input, int32_t* output);
        int forward(Accumulator& acc, Color pov);
        int predict(Board& board);
	}
}