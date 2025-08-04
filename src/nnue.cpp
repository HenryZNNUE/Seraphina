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

#ifdef _MSC_VER
unsigned char        gNNUEData[1] = { 0x0 };
unsigned char*       gNNUEEnd = &gNNUEData[1];
unsigned int         gNNUESize = 1;
#else
INCBIN(NNUE, Seraphina_NNUE);
#endif

#if defined(__AVX512F__) && defined(__AVX512BW__)
void vec_add_dpbusd_32(vec_t& acc, vec_t a, vec_t b)
{
#if defined (VNNI)
	acc = _mm512_dpbusd_epi32(acc, a, b);
#else
	// Multiply a * b and accumulate neighbouring outputs into int16 values
	vec_t product0 = _mm512_maddubs_epi16(a, b);

	// Multiply product0 by 1 (idempotent) and accumulate neighbouring outputs into int32 values
	vec_t one = vec_set1_16(1);
	product0 = _mm512_madd_epi16(product0, one);

	// Add to the main int32 accumulator.
	acc = _mm512_add_epi32(acc, product0);
#endif
};

void vec_add_dpbusd_32x2(vec_t& acc, vec_t a0, vec_t b0, vec_t a1, vec_t b1)
{
	vec_t product0 = _mm512_maddubs_epi16(a0, b0);
	vec_t product1 = _mm512_maddubs_epi16(a1, b1);

	product0 = _mm512_madd_epi16(vec_add_16(product0, product1), vec_set1_16(1));
	acc = _mm512_add_epi32(acc, product0);
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

static int m256_hadd(__m256i sum, int bias)
{
	__m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
	sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_BADC));
	sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_PERM_CDAB));

	return _mm_cvtsi128_si32(sum128) + bias;
}

#elif defined(__AVX2__) || defined(__AVX__)
void vec_add_dpbusd_32(vec_t& acc, vec_t a, vec_t b)
{
#if defined (VNNI)
	acc = _mm256_dpbusd_epi32(acc, a, b);
#else
	// Multiply a * b and accumulate neighbouring outputs into int16 values
	vec_t product0 = _mm256_maddubs_epi16(a, b);

	// Multiply product0 by 1 (idempotent) and accumulate neighbouring outputs into int32 values
	vec_t one = vec_set1_16(1);
	product0 = _mm256_madd_epi16(product0, one);

	// Add to the main int32 accumulator.
	acc = _mm256_add_epi32(acc, product0);
#endif
}

void vec_add_dpbusd_32x2(vec_t& acc, vec_t a0, vec_t b0, vec_t a1, vec_t b1)
{
	vec_t product0 = _mm256_maddubs_epi16(a0, b0);
	vec_t product1 = _mm256_maddubs_epi16(a1, b1);

	product0 = _mm256_madd_epi16(vec_add_16(product0, product1), vec_set1_16(1));
	acc = _mm256_add_epi32(acc, product0);
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
constexpr int32_t acc_width = sizeof(vec_t) / sizeof(int16_t);
const vec_t scalar = vec_set1_32(64);

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
		alignas(64) int32_t L3_bias;

		alignas(64) uint16_t LookUpIndices[256][8];

		// Seraphina NNUE King Bucket
		static constexpr int King_Buckets[SQ_NUM]
		{
			28, 29, 30, 31, 31, 30, 29, 28,
			24, 25, 26, 27, 27, 26, 25, 24,
			20, 21, 22, 23, 23, 22, 21, 20,
			16, 17, 18, 19, 19, 18, 17, 16,
			12, 13, 14, 15, 15, 14, 13, 12,
			  8, 9, 10, 11, 11, 10, 9, 8,
			    4, 5, 6, 7, 7, 6, 5, 4,
			    0, 1, 2, 3, 3, 2, 1, 0,
		};

		inline int FeatureIndex(int piece, int sq, int kingsq, const int view)
		{
			int oP = 6 * ((piece ^ view) & 0x1) + get_piece(piece);
			int oK = (7 * !(kingsq & 4)) ^ (56 * view) ^ kingsq;
			int oSq = (7 * !(kingsq & 4)) ^ (56 * view) ^ sq;

			return King_Buckets[oK] * 12 * 64 + oP * 64 + oSq;
		}

		uint32_t findnnz(const int32_t* input, uint16_t* output, const uint32_t chunk)
		{
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
			const uint32_t chunknum = HIDDEN / acc_width;
			const int povs[2] = { pov, 1 - pov };

			const auto inw = reinterpret_cast<const vec_t*>(&acc->acc[povs[Color::WHITE]]);
			auto outw = reinterpret_cast<vec_t*>(&output[HIDDEN * Color::WHITE]);

			for (uint32_t i = 0; i < chunknum / 2; i += 2)
			{
				vec_t v0 = vec_srai_16(inw[i * 2 + 0], QB1);
				vec_t v1 = vec_srai_16(inw[i * 2 + 1], QB1);
				vec_t v2 = vec_srai_16(inw[i * 2 + 2], QB1);
				vec_t v3 = vec_srai_16(inw[i * 2 + 3], QB1);

				outw[i] = vec_max_8(vec_packs_16(v0, v1), vec_setzero());
				outw[i + 1] = vec_max_8(vec_packs_16(v2, v3), vec_setzero());
			}

			const auto inb = reinterpret_cast<const vec_t*>(&acc->acc[povs[Color::BLACK]]);
			auto outb = reinterpret_cast<vec_t*>(&output[HIDDEN * Color::BLACK]);

			for (uint32_t i = 0; i < chunknum / 2; i += 2)
			{
				vec_t v0 = vec_srai_16(inb[i * 2 + 0], QB1);
				vec_t v1 = vec_srai_16(inb[i * 2 + 1], QB1);
				vec_t v2 = vec_srai_16(inb[i * 2 + 2], QB1);
				vec_t v3 = vec_srai_16(inb[i * 2 + 3], QB1);

				outb[i] = vec_max_8(vec_packs_16(v0, v1), vec_setzero());
				outb[i + 1] = vec_max_8(vec_packs_16(v2, v3), vec_setzero());
			}
		}

		void ClippedReLU(int32_t* input, uint8_t* output, int dim)
		{
			const int32_t chunk = dim / CHUNK_SIZE;
			const auto in = reinterpret_cast<const vec_t*>(input);
			const auto out = reinterpret_cast<vec_t*>(output);

			for (int i = 0; i < chunk; ++i)
			{
                vec_t p0 = vec_srli_16(
                    vec_packus_32(
                        vec_load(&in[i * 4 + 0]),
                        vec_load(&in[i * 4 + 1])),
                    QB1);
				vec_t p1 = vec_srli_16(
					vec_packus_32(
						vec_load(&in[i * 4 + 2]),
						vec_load(&in[i * 4 + 3])),
					QB1);
				vec_store(&out[i], vec_permute_32(
					vec_packs_16(p0, p1), permute_idx));
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

			for (int p = PieceList::PAWN; p < PieceList::NO_PIECE; ++p)
			{
				const PieceType pt = make_piece(Color::WHITE, p);
				const Bitboard pieceBB = board.getPieceBB(pt);
				const Bitboard entryBB = entry.colorpiece[Color::WHITE][p];

				Bitboard toRemove = entryBB & ~pieceBB;
				Bitboard toAdd = pieceBB & ~entryBB;

				while (toRemove)
				{
					const Square sq = Bitboards::poplsb(toRemove);
					delta->removed[delta->r++] = FeatureIndex(pt, sq, KingSq, Color::WHITE);
				}

				while (toAdd)
				{
					const Square sq = Bitboards::poplsb(toAdd);
					delta->added[delta->a++] = FeatureIndex(pt, sq, KingSq, Color::WHITE);
				}

				entry.colorpiece[Color::WHITE][p] = pieceBB;
			}

			for (int p = PieceList::PAWN; p < PieceList::NO_PIECE; ++p)
			{
				const PieceType pt = make_piece(Color::BLACK, p);
				const Bitboard pieceBB = board.getPieceBB(pt);
				const Bitboard entryBB = entry.colorpiece[Color::BLACK][p];

				Bitboard toRemove = entryBB & ~pieceBB;
				Bitboard toAdd = pieceBB & ~entryBB;

				while (toRemove)
				{
					const Square sq = Bitboards::poplsb(toRemove);
					delta->removed[delta->r++] = FeatureIndex(pt, sq, KingSq, Color::BLACK);
				}

				while (toAdd)
				{
					const Square sq = Bitboards::poplsb(toAdd);
					delta->added[delta->a++] = FeatureIndex(pt, sq, KingSq, Color::BLACK);
				}

				entry.colorpiece[Color::BLACK][p] = pieceBB;
			}

			board.acc->add_accumulator(entry.acc.acc[pov], entry.acc.acc[pov], delta);
			std::memcpy(board.acc->acc[pov], entry.acc.acc[pov], sizeof(int16_t) * HIDDEN);
			board.acc->computed[pov] = true;
		}

		void AccumulatorRefreshTable::ResetTable()
		{
			for (int i = 0; i < SQ_NUM; ++i)
			{
				std::memcpy(table[i].acc.acc[Color::WHITE], input_bias, sizeof(int16_t) * HIDDEN);
				std::memcpy(table[i].acc.acc[Color::BLACK], input_bias, sizeof(int16_t) * HIDDEN);
			}
		}

		// Initialize NNUE
		void init()
		{
			size_t index = 0;

			std::memcpy(input_weights, &gNNUEData[index], FEATURES * HIDDEN * sizeof(int16_t));
			index += FEATURES * HIDDEN * sizeof(int16_t);
			std::memcpy(input_bias, &gNNUEData[index], HIDDEN * sizeof(int16_t));
			index += HIDDEN * sizeof(int16_t);

			std::memcpy(L1_weights, &gNNUEData[index], L1 * L2 * sizeof(int8_t));
			index += L1 * L2 * sizeof(int8_t);
			std::memcpy(L1_bias, &gNNUEData[index], L2 * sizeof(int32_t));
			index += L2 * sizeof(int32_t);

			std::memcpy(L2_weights, &gNNUEData[index], L2 * L3 * sizeof(int8_t));
			index += L2 * L3 * sizeof(int8_t);
			std::memcpy(L2_bias, &gNNUEData[index], L3 * sizeof(int32_t));
			index += L3 * sizeof(int32_t);

			std::memcpy(L3_weights, &gNNUEData[index], L3 * OUTPUT * sizeof(int8_t));
			index += L3 * OUTPUT * sizeof(int8_t);
			std::memcpy(&L3_bias, &gNNUEData[index], sizeof(int32_t));

			for (int i = 0; i < L1 * L2; ++i)
			{
				L1_weights[i] = L1_weights[ScrambleWeightIndex(i)];
			}

#if defined(__AVX512F__) && defined(__AVX512BW__)
			const int32_t weight_chunk = (FEATURES * HIDDEN) / acc_width;
			const int32_t bias_chunk = HIDDEN / acc_width;

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
			const int32_t weight_chunk = (FEATURES * HIDDEN) / acc_width;
			const int32_t bias_chunk = HIDDEN / acc_width;

			vec_t* weights = reinterpret_cast<vec_t*>(&input_weights);
			vec_t* bias = reinterpret_cast<vec_t*>(&input_bias);

			for (int32_t i = 0; i < weight_chunk; i += 2)
			{
				__m128i wa1 = _mm256_extracti128_si256(weights[i + 0], 1);
				__m128i wb0 = _mm256_extracti128_si256(weights[i + 1], 0);

				weights[i + 0] = _mm256_inserti128_si256(weights[i + 0], wb0, 1);
				weights[i + 1] = _mm256_inserti128_si256(weights[i + 1], wa1, 0);
			}

			for (int32_t i = 0; i < bias_chunk; i += 2)
			{
				__m128i ba1 = _mm256_extracti128_si256(bias[i + 0], 1);
				__m128i bb0 = _mm256_extracti128_si256(bias[i + 1], 0);

				bias[i + 0] = _mm256_inserti128_si256(bias[i + 0], bb0, 1);
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
			data += fread(&L3_bias, sizeof(int32_t), OUTPUT, file);
			fclose(file);

			assert(data != NNUE_SIZE);
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

		void L1_forward(int8_t* input, int32_t* output)
		{
			const int32_t l1_chunknum = L1 / 4;
			const int32_t l1_outputchunk = L2 / width;
			const int32_t crelu_chunk = L2 / CHUNK_SIZE;

			const auto l1_inputs = reinterpret_cast<const int32_t*>(input);
			vec_t* out = reinterpret_cast<vec_t*>(output);
			const vec_t* l1_bias = reinterpret_cast<const vec_t*>(L1_bias);

			uint16_t nnz[l1_chunknum];
			int32_t nnzcount = findnnz(l1_inputs, nnz, l1_chunknum);

			vec_t l1_reg[l1_outputchunk];

			for (int32_t i = 0; i < l1_outputchunk; ++i)
			{
				l1_reg[i] = l1_bias[i];
			}

			for (int32_t i = 0; i < nnzcount; ++i)
			{
				const auto nz = nnz[i];
				const vec_t v = vec_set1_32(l1_inputs[nz]);
				const auto w = reinterpret_cast<const vec_t*>(&L1_weights[nz * L2 * 4]);

				for (int j = 0; j < l1_outputchunk; ++j)
				{
					vec_add_dpbusd_32(l1_reg[j], v, w[j]);
				}
			}

			for (int j = 0; j < l1_outputchunk; ++j)
			{
				out[j] = l1_reg[j];
			}
		}

		void L2_forward(uint8_t* input, int32_t* output)
		{
			// Layer 2
			const int32_t l2_chunknum = L2 / 4;
			const int32_t l2_outputchunk = L3 / width;

			const auto l2_inputs = reinterpret_cast<const int32_t*>(input);
			const vec_t* l2_weights = reinterpret_cast<const vec_t*>(&L2_weights);
			const vec_t* l2_bias = reinterpret_cast<vec_t*>(L2_bias);
			vec_t* out = reinterpret_cast<vec_t*>(output);

			vec_t l2_reg[l2_outputchunk];

			for (int32_t i = 0; i < l2_outputchunk; ++i)
			{
				l2_reg[i] = l2_bias[i];
			}

			for (int32_t i = 0; i < l2_chunknum; ++i)
			{
				const vec_t in = vec_set1_32(l2_inputs[i]);
				const auto weights = reinterpret_cast<const vec_t*>(&l2_weights[i * L3 * 4]);

				for (int32_t j = 0; j < l2_outputchunk; ++j)
				{
					vec_add_dpbusd_32(l2_reg[j], in, weights[j]);
				}
			}

			for (int32_t i = 0; i < l2_outputchunk; ++i)
			{
				out[i] = l2_reg[i];
			}
		}
		

		void L3_forward(uint8_t* input, int32_t* output)
		{
			const int32_t l3_outputchunk = L3 / width;

			const __m256i* l3_inputs = reinterpret_cast<const __m256i*>(input);
			const __m256i* l3_weights = reinterpret_cast<const __m256i*>(&L3_weights);

			__m256i v0 = _mm256_setzero_si256();

			for (int i = 0; i < l3_outputchunk; ++i)
			{
				const vec_t in = l3_inputs[i];
				vec_add_dpbusd_32(v0, in, l3_weights[i]);
			}
			
			output[0] = m256_hadd(v0, L3_bias);
		}

		int forward(Accumulator& acc, Color c)
		{
			alignas(64) int8_t ft_output[L1];
			alignas(64) int32_t l1_output[L2];
			alignas(64) uint8_t l1_crelu[L2];
			alignas(64) int32_t l2_output[L3];
			alignas(64) uint8_t l2_crelu[L3];
			int32_t eval;

			InputClippedReLU(&acc, ft_output, c);

			L1_forward(ft_output, l1_output);
			ClippedReLU(l1_output, l1_crelu, L2);

			L2_forward(l1_crelu, l2_output);
			ClippedReLU(l2_output, l2_crelu, L3);

			L3_forward(l2_crelu, &eval);

			std::cout << "Accumulator Output:\n" << "Current POV\n";
			for (int i = 0; i < HIDDEN; ++i)
			{
				std::cout << acc.acc[0][i] << " ";
			}

			std::cout << "\n\nOpponent POV\n";
			for (int i = 0; i < HIDDEN; ++i)
			{
				std::cout << acc.acc[1][i] << " ";
			}
			
			std::cout << "\n\nInput ClippedReLU Output:\n";
			for (int i = 0; i < L1; ++i)
			{
				std::cout << static_cast<int>(ft_output[i]) << " ";
			}

			std::cout << "\n\nL1 Output:\n";
			for (int i = 0; i < L2; ++i)
			{
				std::cout << static_cast<int>(l1_output[i]) << " ";
			}

			std::cout << "\n\nL1 CReLU Output:\n";
			for (int i = 0; i < L2; ++i)
			{
				std::cout << static_cast<int>(l1_crelu[i]) << " ";
			}

			std::cout << "\n\nL2 Output:\n";
			for (int i = 0; i < L3; ++i)
			{
				std::cout << static_cast<int>(l2_output[i]) << " ";
			}

			std::cout << "\n\nL2 CReLU Output:\n";
			for (int i = 0; i < L3; ++i)
			{
				std::cout << static_cast<int>(l2_crelu[i]) << " ";
			}

			std::cout << "\n\nNNUE Evaluation Output -> " << eval << "\n"
				      << "Divided by 64 -> " << (eval >> QB1) << "\n\n";

			return (eval >> QB1);
		}

		int predict(Board& board)
		{
			reset_accumulator(board, Color::WHITE);
			reset_accumulator(board, Color::BLACK);

			return forward(*board.acc, board.get_pov());
		}
	}
}