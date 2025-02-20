// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "incbin/incbin.h"
#include "bitboard.h"
#include "movegen.h"
#include "nnue.h"

#include <vector>
#include <fstream>
#include <assert.h>

#ifdef _MSC_VER
unsigned char        gNNUEData[1] = { 0x0 };
unsigned char*       gNNUEEnd = &gNNUEData[1];
unsigned int         gNNUESize = 1;
#else
INCBIN(NNUE, Seraphina_NNUE);
#endif

#if defined(__AVX512F__) && defined(__AVX512BW__)
void vec_add_dpbusd_epi32(vec_t* acc, vec_t a, vec_t b)
{
#if defined (VNNI512)
	acc = _mm512_dpbusd_epi32(acc, a, b);
#else
	// Multiply a * b and accumulate neighbouring outputs into int16 values
	vec_t product0 = _mm512_maddubs_epi16(a, b);

	// Multiply product0 by 1 (idempotent) and accumulate neighbouring outputs into int32 values
	vec_t one = vec_set_16(1);
	product0 = _mm512_madd_epi16(product0, one);

	// Add to the main int32 accumulator.
	*acc = _mm512_add_epi32(*acc, product0);
#endif
};

void vec_add_dpbusd_epi32x2(vec_t* acc, vec_t a0, vec_t b0, vec_t a1, vec_t b1)
{
	vec_t product0 = _mm512_maddubs_epi16(a0, b0);
	vec_t product1 = _mm512_maddubs_epi16(a1, b1);

	product0 = _mm512_madd_epi16(vec_add_16(product0, product1), vec_set_16(1));
	*acc = _mm512_add_epi32(*acc, product0);
}

__m128i m512_haddx4(vec_t sum0, vec_t sum1, vec_t sum2, vec_t sum3, __m128i bias)
{
	vec_t v0 = _mm512_unpacklo_epi32(sum0, sum1);
	vec_t v1 = _mm512_unpackhi_epi32(sum0, sum1);
	vec_t v2 = _mm512_unpacklo_epi32(sum2, sum3);
	vec_t v3 = _mm512_unpackhi_epi32(sum2, sum3);

	vec_t va1 = _mm512_add_epi32(v0, v1);
	vec_t va2 = _mm512_add_epi32(v2, v3);

	vec_t vslo = _mm512_unpacklo_epi64(va1, va2);
	vec_t vshi = _mm512_unpackhi_epi64(va1, va2);

	vec_t vs = _mm512_add_epi32(vslo, vshi);

	__m256i vs256lo = _mm512_castsi512_si256(vs);
	__m256i vs256hi = _mm512_extracti64x4_epi64(vs, 1);

	__m128i vs128lo = _mm256_castsi256_si128(vs256lo);
	__m128i vs128hi = _mm256_extracti128_si256(vs256hi, 1);

	return _mm_add_epi32(_mm_add_epi32(vs128lo, vs128hi), bias);
}

static int m512_hadd(__m512i sum, int bias)
{
	return _mm512_reduce_add_epi32(sum) + bias;
}

#elif defined(__AVX2__) || defined(__AVX__)
void vec_add_dpbusd_epi32(vec_t* acc, vec_t a, vec_t b)
{
#if defined (VNNI256)
	acc = _mm256_dpbusd_epi32(acc, a, b);
#else
	// Multiply a * b and accumulate neighbouring outputs into int16 values
	vec_t product0 = _mm256_maddubs_epi16(a, b);

	// Multiply product0 by 1 (idempotent) and accumulate neighbouring outputs into int32 values
	vec_t one = vec_set_16(1);
	product0 = _mm256_madd_epi16(product0, one);

	// Add to the main int32 accumulator.
	*acc = _mm256_add_epi32(*acc, product0);
#endif
}

void vec_add_dpbusd_epi32x2(vec_t* acc, vec_t a0, vec_t b0, vec_t a1, vec_t b1)
{
	vec_t product0 = _mm256_maddubs_epi16(a0, b0);
	vec_t product1 = _mm256_maddubs_epi16(a1, b1);

	product0 = _mm256_madd_epi16(vec_add_16(product0, product1), vec_set_16(1));
	*acc = _mm256_add_epi32(*acc, product0);
}

__m128i m256_haddx4(vec_t sum0, vec_t sum1, vec_t sum2, vec_t sum3, __m128i bias)
{
	sum0 = _mm256_hadd_epi32(sum0, sum1);
	sum2 = _mm256_hadd_epi32(sum2, sum3);

	sum0 = _mm256_hadd_epi32(sum0, sum2);

	__m128i sum128lo = _mm256_castsi256_si128(sum0);
	__m128i sum128hi = _mm256_extracti128_si256(sum0, 1);

	return _mm_add_epi32(_mm_add_epi32(sum128lo, sum128hi), bias);
}

static int m256_hadd(__m256i sum, int bias)
{
	__m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
	sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
	sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));

	return _mm_cvtsi128_si32(sum128) + bias;
}

__m128i vec_cvt_8(__m256i a)
{
	__m128i lo = _mm256_extracti128_si256(a, 0);
	__m128i hi = _mm256_extracti128_si256(a, 1);
	lo = _mm_packus_epi32(lo, _mm_setzero_si128());
	hi = _mm_packus_epi32(hi, _mm_setzero_si128());
	lo = _mm_packus_epi16(lo, _mm_setzero_si128());
	hi = _mm_packus_epi16(hi, _mm_setzero_si128());

	return _mm_unpacklo_epi64(lo, hi);
}

#endif

constexpr int32_t width = BIT_ALIGNMENT / CHUNK_SIZE;
constexpr int32_t outwidth = sizeof(__m256i) / sizeof(uint8_t);
const vec_t scalar = vec_set_32(64);

namespace Seraphina
{
	namespace NNUE
	{
		alignas(64) int16_t input_weights[FEATURES * HIDDEN];
		alignas(64) int16_t input_bias[HIDDEN];

		alignas(64) int8_t L1_weights[L1 * L2];
		alignas(64) int32_t L1_bias[L2];

		alignas(64) int8_t L2_weights[L2 * L3];
		alignas(64) int32_t L2_bias[L3];

		alignas(64) int8_t L3_weights[L3 * OUTPUT];
		alignas(64) int32_t L3_bias[OUTPUT];

		alignas(64) uint16_t LookUpIndices[256][8];

		// Seraphina NNUE King Bucket
		static constexpr int King_Buckets[SQ_NUM]
		{
			-1, -1, -1, -1, 31, 30, 29, 28,
			-1, -1, -1, -1, 27, 26, 25, 24,
			-1, -1, -1, -1, 23, 22, 21, 20,
			-1, -1, -1, -1, 19, 18, 17, 16,
			-1, -1, -1, -1, 15, 14, 13, 12,
			-1, -1, -1, -1, 11, 10, 9, 8,
			-1, -1, -1, -1, 7, 6, 5, 4,
			-1, -1, -1, -1, 3, 2, 1, 0,
		};

		inline int FeatureIndex(int piece, int sq, int kingsq, const int view)
		{
			int oP = 6 * ((piece ^ view) & 0x1) + get_piece((PieceType)piece);
			int oK = (7 * !(kingsq & 4)) ^ (56 * view) ^ kingsq;
			int oSq = (7 * !(kingsq & 4)) ^ (56 * view) ^ sq;

			return King_Buckets[oK] * 12 * 64 + oP * 64 + oSq;
		}

		uint32_t findnnz(const int32_t* input, uint16_t* output, const uint32_t chunk)
		{
			const uint32_t width = BIT_ALIGNMENT / CHUNK_SIZE;
			const uint32_t chunknum = chunk / width;
			const uint32_t inputperchunk = 1;
			const uint32_t outputperchunk = width / 8;
			uint32_t count = 0;

			const auto in = reinterpret_cast<const vec_t*>(input);

			const __m128i increment = _mm_set1_epi16(8);
			__m128i base = _mm_setzero_si128();

			for (uint32_t i = 0; i < chunknum; ++i)
			{
				uint32_t nnz = 0;

				for (uint32_t j = 0; j < inputperchunk; ++j)
				{
					const vec_t inputchunk = vec_load(&in[i * inputperchunk + j]);
					nnz |= vec_nnz(inputchunk) << (j * width);
				}

				for (uint32_t k = 0; k < outputperchunk; ++k)
				{
					const uint16_t lookup = (nnz >> (k * 8)) & 0xFF;
					const __m128i offsets = _mm_loadu_si128((__m128i*) (&LookUpIndices[lookup]));
					_mm_storeu_si128((__m128i*) (output + count), _mm_add_epi16(base, offsets));
					count += Bitboards::popcount(lookup);
					base = _mm_add_epi16(base, increment);
				}
			}

			return count;
		}

		void InputClippedReLU(Accumulator* acc, int8_t* output, Color pov)
		{
			const uint32_t width = sizeof(vec_t) / sizeof(int16_t);
			const uint32_t chunknum = HIDDEN / width;
			const int povs[2] = { pov, ~pov };

			for (int32_t c = 0; c < 2; ++c)
			{
				const auto in = reinterpret_cast<const vec_t*>(&acc->acc[povs[c]]);
				auto out = reinterpret_cast<vec_t*>(&output[HIDDEN * c]);

				for (uint32_t i = 0; i < chunknum / 2; i += 2)
				{
					vec_t v0 = vec_srai_16(in[i * 2], 6);
					vec_t v1 = vec_srai_16(in[i * 2 + 1], 6);
					vec_t v2 = vec_srai_16(in[i * 2 + 2], 6);
					vec_t v3 = vec_srai_16(in[i * 2 + 3], 6);

					out[i] = vec_max_16(vec_packs_16(v0, v1), vec_setzero());
					out[i + 1] = vec_max_16(vec_packs_16(v2, v3), vec_setzero());
				}
			}
		}

		void remove_add(int16_t* input, int16_t* output, int16_t& v1, int16_t& v2)
		{
			vec_t acc[REG_NUM];

			for (int32_t i = 0; i < HIDDEN / BIT_ALIGNMENT; ++i)
			{
				const int32_t offset = i * BIT_ALIGNMENT;
				const auto inputs = reinterpret_cast<const vec_t*>(&input[offset]);
				const auto outputs = reinterpret_cast<vec_t*>(&output[offset]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_load(&inputs[j]);
				}

				const int32_t or1 = v1 * HIDDEN + offset;
				const vec_t* wr1 = reinterpret_cast<const vec_t*>(&input_weights[or1]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_sub_16(acc[j], wr1[j]);
				}

				const int32_t oa = v2 * HIDDEN + offset;
				const vec_t* wa = reinterpret_cast<const vec_t*>(&input_weights[oa]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_add_16(acc[j], wa[j]);
				}

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					vec_store(&outputs[j], acc[j]);
				}
			}
		}

		void remove_remove_add(int16_t* input, int16_t* output, int16_t& v1, int16_t& v2, int16_t& v3)
		{
			vec_t acc[REG_NUM];

			for (int32_t i = 0; i < HIDDEN / BIT_ALIGNMENT; ++i)
			{
				const int32_t offset = i * BIT_ALIGNMENT;
				const auto inputs = reinterpret_cast<const vec_t*>(&input[offset]);
				const auto outputs = reinterpret_cast<vec_t*>(&output[offset]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_load(&inputs[j]);
				}

				const int32_t or1 = v1 * HIDDEN + offset;
				const vec_t* wr1 = reinterpret_cast<const vec_t*>(&input_weights[or1]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_sub_16(acc[j], wr1[j]);
				}

				const int32_t or2 = v2 * HIDDEN + offset;
				const vec_t* wr2 = reinterpret_cast<const vec_t*>(&input_weights[or2]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_sub_16(acc[j], wr2[j]);
				}

				const int32_t oa = v3 * HIDDEN + offset;
				const vec_t* wa = reinterpret_cast<const vec_t*>(&input_weights[oa]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_add_16(acc[j], wa[j]);
				}

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					vec_store(&outputs[j], acc[j]);
				}
			}
		}
		void remove_remove_add_add(int16_t* input, int16_t* output, int16_t& v1, int16_t& v2, int16_t& v3, int16_t& v4)
		{
			vec_t acc[REG_NUM];

			for (int32_t i = 0; i < HIDDEN / BIT_ALIGNMENT; ++i)
			{
				const int32_t offset = i * BIT_ALIGNMENT;
				const auto inputs = reinterpret_cast<const vec_t*>(&input[offset]);
				const auto outputs = reinterpret_cast<vec_t*>(&output[offset]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_load(&inputs[j]);
				}

				const int32_t or1 = v1 * HIDDEN + offset;
				const vec_t* wr1 = reinterpret_cast<const vec_t*>(&input_weights[or1]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_sub_16(acc[j], wr1[j]);
				}

				const int32_t or2 = v2 * HIDDEN + offset;
				const vec_t* wr2 = reinterpret_cast<const vec_t*>(&input_weights[or2]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_sub_16(acc[j], wr2[j]);
				}

				const int32_t oa1 = v3 * HIDDEN + offset;
				const vec_t* wa1 = reinterpret_cast<const vec_t*>(&input_weights[oa1]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_add_16(acc[j], wa1[j]);
				}

				const int32_t oa2 = v4 * HIDDEN + offset;
				const vec_t* wa2 = reinterpret_cast<const vec_t*>(&input_weights[oa2]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					acc[j] = vec_add_16(acc[j], wa2[j]);
				}

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					vec_store(&outputs[j], acc[j]);
				}
			}
		}

		// Scramble weight index (for L1)
		static constexpr int32_t ScrambleWeightIndex(int32_t input)
		{
			return (input / 4) % (L1 / 4) * L2 * 4
				+ input / L1 * 4 + input % 4;
		}

		void* aligned_malloc(size_t size, size_t alignment)
		{
#if defined _WIN64
			void* ptr = _aligned_malloc(size, alignment);

			if (ptr == nullptr)
			{
				std::cerr << "Memory allocation failed\n";
				return nullptr;
			}
#else
			void* ptr = nullptr;
			int result = posix_memalign(&ptr, alignment, size);

			if (result != 0)
			{
				std::cerr << "Memory allocation failed\n";
				return nullptr;
			}
#endif

			return ptr;
		}

		bool Accumulator::RequireRefresh(Move& move, int pov)
		{
			if (get_piece(getPieceType(move)) == PieceList::KING)
			{
				return true;
			}

			return NNUE::King_Buckets[getFrom(move) ^ (56 * pov)] != NNUE::King_Buckets[getTo(move) ^ (56 * pov)];
		}

		bool Accumulator::updatable(Accumulator* accum, Move& move, int pov)
		{
			while (1)
			{
				--accum;

				if (get_color(getPieceType(move)) == pov && RequireRefresh(move, pov))
				{
					return false;
				}
				
				if (accum->computed[pov])
				{
					return true;
				}
			}
		}

		void Accumulator::add_accumulator(int16_t* input, int16_t* output, Delta* delta)
		{
			vec_t accum[REG_NUM];

			for (int32_t i = 0; i < HIDDEN / BIT_ALIGNMENT; ++i)
			{
				const int32_t offset = i * BIT_ALIGNMENT;
				const auto inputs = reinterpret_cast<const vec_t*>(&input[offset]);
				auto outputs = reinterpret_cast<vec_t*>(&output[offset]);

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					accum[j] = vec_load(&inputs[j]);
				}

				for (int32_t r = 0; r < delta->r; ++r)
				{
					const int32_t removedOffset = delta->removed[r] * HIDDEN + offset;
					const vec_t* weights = reinterpret_cast<const vec_t*>(&input_weights[removedOffset]);

					for (int32_t j = 0; j < REG_NUM; ++j)
					{
						accum[j] = vec_sub_16(accum[j], weights[j]);
					}
				}

				for (int32_t a = 0; a < delta->a; ++a)
				{
					const int32_t addedOffset = delta->added[a] * HIDDEN + offset;
					const vec_t* weights = reinterpret_cast<const vec_t*>(&input_weights[addedOffset]);

					for (int32_t j = 0; j < REG_NUM; ++j)
					{
						accum[j] = vec_add_16(accum[j], weights[j]);
					}
				}

				for (int32_t j = 0; j < REG_NUM; ++j)
				{
					vec_store(&outputs[j], accum[j]);
				}
			}
		}

		void AccumulatorRefreshTable::ApplyRefresh(Board& board, int pov)
		{
			Delta delta[1];
			delta->a = delta->r = 0;
			Square KingSq = board.getKingSquare(pov);
			AccumulatorRefreshTableEntry& entry = board.accRT->table[KingSq];

			for (int c = WHITE; c < NO_COLOR; ++c)
			{
				for (int p = PAWN; p < NO_PIECE; ++p)
				{
					const PieceType pt = make_piece(c, p);
					const Bitboard pieceBB = board.getPieceBB(pt);
					const Bitboard entryBB = entry.colorpiece[c][p];

					Bitboard toRemove = entryBB & ~pieceBB;
					Bitboard toAdd = pieceBB & ~entryBB;

					while (toRemove)
					{
						const Square sq = Bitboards::poplsb(toRemove);
						delta->removed[delta->r++] = FeatureIndex(pt, sq, KingSq, c);
					}

					while (toAdd)
					{
						const Square sq = Bitboards::poplsb(toAdd);
						delta->added[delta->a++] = FeatureIndex(pt, sq, KingSq, c);
					}

					entry.colorpiece[c][p] = pieceBB;
				}
			}

			board.acc->add_accumulator(entry.acc.acc[pov], entry.acc.acc[pov], delta);
			std::memcpy(board.acc->acc[pov], entry.acc.acc[pov], sizeof(int16_t) * HIDDEN);
			board.acc->computed[pov] = true;
		}

		void AccumulatorRefreshTable::ResetTable()
		{
			for (int pov = 0; pov < 2; ++pov)
			{
				for (int i = 0; i < SQ_NUM; ++i)
				{
					std::memcpy(table[i].acc.acc[pov], input_bias, sizeof(int16_t) * HIDDEN);
				}
			}
		}

		// Initialize NNUE
		void init()
		{
			size_t index = 0;
			int8_t* l1 = (int8_t*)malloc(L1 * L2 * sizeof(int8_t));

			std::memcpy(input_weights, &gNNUEData[index], FEATURES * HIDDEN * sizeof(int16_t));
			index += FEATURES * HIDDEN * sizeof(int16_t);
			std::memcpy(input_bias, &gNNUEData[index], HIDDEN * sizeof(int16_t));
			index += HIDDEN * sizeof(int16_t);

			std::memcpy(l1, &gNNUEData[index], L1 * L2 * sizeof(int8_t));
			index += L1 * L2 * sizeof(int8_t);
			std::memcpy(L1_bias, &gNNUEData[index], L2 * sizeof(int32_t));
			index += L2 * sizeof(int32_t);

			std::memcpy(L2_weights, &gNNUEData[index], L2 * L3 * sizeof(int8_t));
			index += L2 * L3 * sizeof(int8_t);
			std::memcpy(L2_bias, &gNNUEData[index], L3 * sizeof(int32_t));
			index += L3 * sizeof(int32_t);

			std::memcpy(L3_weights, &gNNUEData[index], L3 * OUTPUT * sizeof(int8_t));
			index += L3 * OUTPUT * sizeof(int8_t);
			std::memcpy(L3_bias, &gNNUEData[index], sizeof(int32_t));

			for (int i = 0; i < L1 * L2; ++i)
			{
				L1_weights[ScrambleWeightIndex(i)] = l1[i];
			}

			free(l1);

#if defined(__AVX512F__) && defined(__AVX512BW__)
			const int32_t width = sizeof(vec_t) / sizeof(int16_t);
			const int32_t weight_chunk = (FEATURES * HIDDEN) / width;
			const int32_t bias_chunk = HIDDEN / width;

			vec_t* weights = reinterpret_cast<vec_t*>(&input_weights);
			vec_t* bias = reinterpret_cast<vec_t*>(&input_bias);

			for (int32_t i = 0; i < weight_chunk; i += 2)
			{
				__m128i wa1 = _mm512_extracti32x4_epi32(weights[i], 1);
				__m128i wa2 = _mm512_extracti32x4_epi32(weights[i], 2);
				__m128i wa3 = _mm512_extracti32x4_epi32(weights[i], 3);
				__m128i wb0 = _mm512_extracti32x4_epi32(weights[i + 1], 0);
				__m128i wb1 = _mm512_extracti32x4_epi32(weights[i + 1], 1);
				__m128i wb2 = _mm512_extracti32x4_epi32(weights[i + 1], 2);

				weights[i] = _mm512_inserti32x4(weights[i], wa2, 1);
				weights[i] = _mm512_inserti32x4(weights[i], wb0, 2);
				weights[i] = _mm512_inserti32x4(weights[i], wb2, 3);
				weights[i + 1] = _mm512_inserti32x4(weights[i + 1], wa1, 0);
				weights[i + 1] = _mm512_inserti32x4(weights[i + 1], wa3, 1);
				weights[i + 1] = _mm512_inserti32x4(weights[i + 1], wb1, 2);
			}

			for (int32_t i = 0; i < bias_chunk; i += 2)
			{
				__m128i ba1 = _mm512_extracti32x4_epi32(bias[i], 1);
				__m128i ba2 = _mm512_extracti32x4_epi32(bias[i], 2);
				__m128i ba3 = _mm512_extracti32x4_epi32(bias[i], 3);
				__m128i bb0 = _mm512_extracti32x4_epi32(bias[i + 1], 0);
				__m128i bb1 = _mm512_extracti32x4_epi32(bias[i + 1], 1);
				__m128i bb2 = _mm512_extracti32x4_epi32(bias[i + 1], 2);

				bias[i] = _mm512_inserti32x4(bias[i], ba2, 1);
				bias[i] = _mm512_inserti32x4(bias[i], bb0, 2);
				bias[i] = _mm512_inserti32x4(bias[i], bb2, 3);
				bias[i + 1] = _mm512_inserti32x4(bias[i + 1], ba1, 0);
				bias[i + 1] = _mm512_inserti32x4(bias[i + 1], ba3, 1);
				bias[i + 1] = _mm512_inserti32x4(bias[i + 1], bb1, 2);
			}
#elif defined(__AVX2__) || defined(__AVX__)
			const int32_t width = sizeof(vec_t) / sizeof(int16_t);
			const int32_t weight_chunk = (FEATURES * HIDDEN) / width;
			const int32_t bias_chunk = HIDDEN / width;

			vec_t* weights = reinterpret_cast<vec_t*>(&input_weights);
			vec_t* bias = reinterpret_cast<vec_t*>(&input_bias);

			for (int32_t i = 0; i < weight_chunk; i += 2)
			{
				__m128i wa1 = _mm256_extracti128_si256(weights[i], 1);
				__m128i wb0 = _mm256_extracti128_si256(weights[i + 1], 0);

				weights[i] = _mm256_inserti128_si256(weights[i], wb0, 1);
				weights[i + 1] = _mm256_inserti128_si256(weights[i + 1], wa1, 0);
			}

			for (int32_t i = 0; i < bias_chunk; i += 2)
			{
				__m128i ba1 = _mm256_extracti128_si256(bias[i], 1);
				__m128i bb0 = _mm256_extracti128_si256(bias[i + 1], 0);

				bias[i] = _mm256_inserti128_si256(bias[i], bb0, 1);
				bias[i + 1] = _mm256_inserti128_si256(bias[i + 1], ba1, 0);
			}
#endif

			for (int i = 0; i < 256; ++i)
			{
				uint64_t j = i;
				uint64_t k = 0;

				while (j)
				{
					LookUpIndices[i][k++] = Bitboards::poplsb(j);
				}
			}
		}

		void load_external(std::string& path)
		{
			FILE* file = fopen(path.c_str(), "rb");
			std::uint8_t data = 0;

			assert(file != nullptr);

			data += fread(input_weights, sizeof(int16_t), FEATURES * HIDDEN, file);
			data += fread(input_bias, sizeof(int16_t), HIDDEN, file);
			data += fread(L1_weights, sizeof(int8_t), L1 * L2, file);
			data += fread(L1_bias, sizeof(int32_t), L2, file);
			data += fread(L2_weights, sizeof(int8_t), L2 * L3, file);
			data += fread(L2_bias, sizeof(int32_t), L3, file);
			data += fread(L3_weights, sizeof(int8_t), L3, file);
			data += fread(L3_bias, sizeof(int32_t), OUTPUT, file);
			fclose(file);

			assert(data != NNUE_SIZE);

			init();
		}

		void update_accumulator(Board& board, const Move& move, int c)
		{
			Accumulator* newacc = board.acc;

			while (!(--newacc)->computed[c]);

			do
			{
				const auto input = newacc->acc[c];
				const auto output = (newacc++)->acc[c];
				const int KingSq = board.getKingSquare(c);
				const Color pov = board.get_pov();
				MoveType mt = getMoveType(move);
				int16_t from = FeatureIndex(getPieceType(move), getFrom(move), KingSq, c);
				int16_t to = FeatureIndex(getPieceType(move), getTo(move), KingSq, c);

				/*
				int16_t to = FeatureIndex((mt == MoveType::PROMOTION || mt == MoveType::PROMOTION_CAPTURE)
					? (mt == MoveType::PROMOTION_CAPTURE ? board.getBoardInfo()->capture : getCapPromo(move))
					: getPieceType(move), getTo(move), KingSq, pov);
				*/

				if (mt == MoveType::PROMOTION)
				{
					to = FeatureIndex(getCapPromo(move), getTo(move), KingSq, c);
				}

				if (mt == MoveType::PROMOTION_CAPTURE)
				{
					to = FeatureIndex(board.getBoardInfo()->capture, getTo(move), KingSq, c);
				}

				if (mt == MoveType::SHORT_CASTLE)
				{
					int16_t rookFrom = FeatureIndex(make_piece(pov, ROOK), (pov == Color::WHITE ? Square::SQ_H1 : Square::SQ_H8), KingSq, c);
					int16_t rookTo = FeatureIndex(make_piece(pov, ROOK), (pov == Color::WHITE ? Square::SQ_F1 : Square::SQ_F8), KingSq, c);

					remove_remove_add_add(input, output, from, rookFrom, to, rookTo);
				}

				else if (mt == MoveType::LONG_CASTLE)
				{
					int16_t rookFrom = FeatureIndex(make_piece(pov, ROOK), (pov == Color::WHITE ? Square::SQ_A1 : Square::SQ_A8), KingSq, c);
					int16_t rookTo = FeatureIndex(make_piece(pov, ROOK), (pov == Color::WHITE ? Square::SQ_D1 : Square::SQ_D8), KingSq, c);

					remove_remove_add_add(input, output, from, rookFrom, to, rookTo);
				}

				else if (mt == MoveType::CAPTURE)
				{
					int16_t captured = FeatureIndex(getCaptured(move), getTo(move), KingSq, c);

					remove_remove_add(input, output, from, captured, to);
				}

				else if (mt == MoveType::ENPASSNT)
				{
					int16_t captured = FeatureIndex(getCaptured(move), getTo(move) - (get_color(getPieceType(move)) == Color::WHITE ? Direction::NORTH : Direction::SOUTH), KingSq, c);

					remove_remove_add(input, output, from, captured, to);
				}

				else
				{
					remove_add(input, output, from, to);
				}

				(newacc + 1)->computed[c] = true;
			} while (++newacc != board.acc);
		}

		void reset_accumulator(Board& board, int c)
		{
			Delta delta[1];
			delta->a = delta->r = 0;
			int KingSq = board.getKingSquare(c);
			Bitboard occBB = board.getoccBB(Color::NO_COLOR);

			while (occBB)
			{
				const Square sq = Bitboards::poplsb(occBB);
				const PieceType pt = board.getPieceType(sq);
				delta->added[delta->a++] = FeatureIndex(pt, sq, KingSq, c);
			}

			int16_t* acc = board.acc->acc[c];
			std::memcpy(acc, input_bias, sizeof(int16_t) * HIDDEN);
			board.acc->add_accumulator(acc, acc, delta);
			board.acc->computed[c] = true;
		}

		void L1_forward(int8_t* input, uint8_t* output)
		{
			// Layer 1
			const int32_t l1_chunknum = L1 / 4;
			const int32_t l1_outputchunk = L2 / width;

			const auto l1_inputs = reinterpret_cast<const int32_t*>(input);
			__m128i* out = reinterpret_cast<__m128i*>(&output);
			const vec_t* l1_bias = reinterpret_cast<vec_t*>(L1_bias);

			uint16_t nnz[l1_chunknum];
			int32_t nnzcount = findnnz(l1_inputs, nnz, l1_chunknum);

			vec_t l1_reg[l1_outputchunk];

			for (int32_t i = 0; i < l1_outputchunk; ++i)
			{
				l1_reg[i] = l1_bias[i];
			}

			int32_t i = 0;

			for (; (i + 1) < nnzcount; i += 2)
			{
				const uint16_t i0 = nnz[i + 0];
				const uint16_t i1 = nnz[i + 1];

				const vec_t v0 = vec_set_32(l1_inputs[i0]);
				const vec_t v1 = vec_set_32(l1_inputs[i1]);
				const vec_t* w0 = reinterpret_cast<const vec_t*>(&L1_weights[i0 * L2 * 4]);
				const vec_t* w1 = reinterpret_cast<const vec_t*>(&L1_weights[i1 * L2 * 4]);

				for (int32_t j = 0; j < l1_outputchunk; ++j)
				{
					vec_add_dpbusd_epi32x2(l1_reg + j, v0, w0[j], v1, w1[j]);
				}
			}

			if (i < nnzcount)
			{
				const uint16_t i0 = nnz[i];
				const vec_t v0 = vec_set_32(l1_inputs[i0]);
				const vec_t* w0 = reinterpret_cast<const vec_t*>(&L1_weights[i0 * L2 * 4]);

				for (int j = 0; j < l1_outputchunk; ++j)
				{
					vec_add_dpbusd_epi32(l1_reg + j, v0, w0[j]);
				}
			}

			for (int32_t j = 0; j < l1_outputchunk; ++j)
			{
				out[j] = vec_cvt_8(vec_min_32(vec_max_32(l1_reg[j], vec_zero()), scalar));
			}
		}

		void L2_forward(uint8_t* input, uint8_t* output)
		{
			// Layer 2
			const int32_t l2_chunknum = L2 / 4;
			const int32_t l2_outputchunk = L3 / width;

			const auto l2_inputs = reinterpret_cast<const int32_t*>(input);
			const vec_t* l2_weights = reinterpret_cast<const vec_t*>(&L2_weights);
			const vec_t* l2_bias = reinterpret_cast<vec_t*>(L2_bias);
			__m128i* out = reinterpret_cast<__m128i*>(&output);

			vec_t l2_reg[l2_outputchunk];

			for (int32_t i = 0; i < l2_outputchunk; ++i)
			{
				l2_reg[i] = l2_bias[i];
			}

			for (int32_t i = 0; i < l2_chunknum; ++i)
			{
				const vec_t in = vec_set_32(l2_inputs[i]);
				const auto weights = reinterpret_cast<const vec_t*>(&l2_weights[i * L3 * 4]);

				for (int32_t j = 0; j < l2_outputchunk; ++j)
				{
					vec_add_dpbusd_epi32(&l2_reg[j], in, weights[j]);
				}
			}

			for (int32_t i = 0; i < l2_outputchunk; ++i)
			{
				out[i] = vec_cvt_8(vec_min_32(vec_max_32(l2_reg[i], vec_zero()), scalar));
			}
		}
		

		void L3_forward(uint8_t* input, int32_t* output)
		{
			// Layer 3
			const int32_t l3_outputchunk = L3 / outwidth;

			const vec_t* l3_inputs = reinterpret_cast<const vec_t*>(&input);
			const vec_t* l3_weights = reinterpret_cast<const vec_t*>(&L3_weights);

			vec_t v0 = vec_setzero();

			for (int i = 0; i < l3_outputchunk; ++i)
			{
				vec_add_dpbusd_epi32(&v0, l3_inputs[i], l3_weights[i]);
			}

#if defined(__AVX512F__) && defined(__AVX512BW__)
			output[0] = m512_hadd(v0, L3_bias[0]);
#elif defined(__AVX2__) || defined(__AVX__)
			output[0] = m256_hadd(v0, L3_bias[0]);
#endif
		}

		int forward(Accumulator& acc, Color c)
		{
			alignas(64) int8_t ft_output[L1];
			alignas(64) uint8_t l1_output[L2];
			alignas(64) uint8_t l2_output[L3];
			int32_t eval;

			InputClippedReLU(&acc, ft_output, c);

			L1_forward(ft_output, l1_output);
			L2_forward(l1_output, l2_output);
			L3_forward(l2_output, &eval);

			return eval / 64;
		}

		int predict(Board& board)
		{
			reset_accumulator(board, Color::WHITE);
			reset_accumulator(board, Color::BLACK);

			return forward(*board.acc, board.get_pov());
		}
	}
}