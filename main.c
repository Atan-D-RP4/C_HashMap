#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
	Nob_String_View key;
	size_t value;
	bool occupied;
} FreqKV;

typedef struct {
	FreqKV *items;
	size_t count;
	size_t capacity;
} FreqKV_Array;

int bitwise_sub(int x, int y) {
	while (y != 0) {
		// Borrow contains common set bits of y and unset bits of x
		int borrow = (~x) & y;

		// Subtraction of bits of x and y where at least one of the bits is not set
		x = x ^ y;

		// Borrow is shifted by one so that subtracting it from x gives the required difference
		y = borrow << 1;
	}
	return x;
}

FreqKV *find_key(FreqKV_Array haystack,  Nob_String_View needle) {
	for (size_t i = 0; i < haystack.count; ++i) {
		if (nob_sv_eq(haystack.items[i].key, needle)) {
			return &haystack.items[i];
		}
	}
	return NULL;
}

int nob_cmp_freqkv(const void *a, const void *b) {
	const FreqKV *kva = a;
	const FreqKV *kvb = b;
	int val_a = (int) kva->value;
	int val_b = (int) kvb->value;
	return bitwise_sub((val_b < val_a), (val_a < val_b));
}

void log_elapsed(struct timespec begin, struct timespec end) {
	/* Another cool way to do this
	 * double a = (double) begin.tv_sec + (double) begin.tv_nsec / 1.0e9;
	 * double b = (double) end.tv_sec + (double) end.tv_nsec / 1.0e9;
	 * nob_log(NOB_INFO, "Elapsed time: %f seconds", b - a);
	 */
	double elapsed = end.tv_sec - begin.tv_sec;
	elapsed += (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
	nob_log(NOB_INFO, "Elapsed time: %.3lfs", elapsed);
}

void naive_analysis(Nob_String_View content) {

	nob_log(NOB_INFO, "Linear Analysis:");
	struct timespec begin, end;
	assert(clock_gettime(CLOCK_MONOTONIC, &begin) == 0);
	FreqKV_Array freqs = {0}; // TODO: Solve Memory Leak

	// The tokenizer
	size_t count = 0;
	for (; content.count > 0; ++count) {
		content = nob_sv_trim_left(content);
		Nob_String_View token  = nob_sv_chop_by_space(&content);

		FreqKV *kv = find_key(freqs, token);
		if (kv) {
			kv->value++;
		} else {
			nob_da_append(&freqs, ((FreqKV) {.key = token, .value = 1, .occupied = true}));
		}
	}

	qsort(freqs.items, freqs.count, sizeof(freqs.items[0]), nob_cmp_freqkv);

	assert(clock_gettime(CLOCK_MONOTONIC, &end) == 0);

	// top 10 tokens
	nob_log(NOB_INFO, "	 Top 10 tokens:");
	nob_log(NOB_INFO, "	 Tokens: %zu", count);
	for (size_t i = 1; i < freqs.count && i <= 10; ++i) {
		nob_log(NOB_INFO, "	 %zu: "SV_Fmt" => %zu", i, SV_Arg(freqs.items[i - 1].key), freqs.items[i - 1].value);
	}
	nob_log(NOB_INFO, "Unique Tokens: %zu", freqs.count);

	log_elapsed(begin, end);

}

uint32_t hash(uint8_t *data, size_t len) {
	//  uint32_t hash = (uint32_t) data[0] << 7;
	// 	for (size_t i = 0; i < len; ++i) {
	// 		uint32_t byte = (uint32_t) data[i];
	// 		hash = ((hash * 31) ^ byte);
	// 	}
	uint64_t hash = 5381;
	for (size_t i = 0; i < len; ++i) {
		hash = ((hash << 5) + hash) + (uint64_t) data[i];
	}
	return hash;
}

#define hash_init(ht, cap) \
	do { \
		(ht)->items = malloc(sizeof(*(ht)->items) * cap); \
		memset((ht)->items, 0, cap * sizeof(*(ht)->items)); \
		(ht)->count = 0; \
		(ht)->capacity = cap; \
	} while (0)

#define hash_find(ht, hash) \

bool hash_analysis(Nob_String_View content) {

	// A Hash based algorithm
	nob_log(NOB_INFO, "Hash Analysis:");
	struct timespec begin, end;
	assert(clock_gettime(CLOCK_MONOTONIC, &begin) == 0);

	size_t N = 70000;
	FreqKV_Array ht = { 0 };
	hash_init(&ht, N);

	//	nob_log(NOB_INFO, "Tokens with Hashs:");
	size_t count = 0;
	size_t occupied = 0;
	size_t collisions = 0;
	for (; content.count > 0; ++count) {
		content = nob_sv_trim_left(content);
		Nob_String_View token  = nob_sv_chop_by_space(&content);

		uint32_t h = (uint32_t) hash((uint8_t *) token.data, token.count) % ht.capacity;
		//		nob_log(NOB_INFO, "	 %zu: 0x%08X - Frequency: %zu - "SV_Fmt, count + 1, h, ht.items[h].value, SV_Arg(token));
		for (size_t i = 0; i < ht.capacity && ht.items[h].occupied && !nob_sv_eq(ht.items[h].key, token); ++i) {
			h = (h + 1) % ht.capacity;
			collisions++;
		}

		if (ht.items[h].occupied && nob_sv_eq(ht.items[h].key, token)) {
			if (!nob_sv_eq(ht.items[h].key, token)) {
				nob_log(NOB_INFO, "Table Overflow");
				return 1;
			}
			ht.items[h].value++;
		} else {
			ht.items[h].occupied = true;
			ht.items[h].key = token;
			ht.items[h].value = 1;
			occupied++;
		}
	}

	// nob_log(NOB_INFO, "	 Table:");

	// nobfor (size_t i = 0; i < N && i < 50; ++i) {
	// nob	if (ht.items[i].occupied)
	// nob		nob_log(NOB_INFO, "	 %zu: "SV_Fmt" => %zu", i, SV_Arg(ht.items[i].key), ht.items[i].value);
	// nob}

	FreqKV_Array freqs = {0};
	for (size_t i = 0; i < ht.capacity; ++i) {
		if (ht.items[i].occupied) {
			nob_da_append(&freqs, ht.items[i]);
		}
	}

	qsort(freqs.items, freqs.count, sizeof(freqs.items[0]), nob_cmp_freqkv);

	assert(clock_gettime(CLOCK_MONOTONIC, &end) == 0);

	// top 10 tokens
	//	nob_log(NOB_INFO, "	 Top 10 tokens:");
	for (size_t i = 1; i < freqs.count  && i <= 10; ++i) {
		nob_log(NOB_INFO, "	 %zu: "SV_Fmt" => %zu", i, SV_Arg(freqs.items[i - 1].key), freqs.items[i - 1].value);
	}

	nob_log(NOB_INFO, "Unique Tokens: %zu", freqs.count);
	nob_log(NOB_INFO, "Occupied: %zu", occupied);
	nob_log(NOB_INFO, "Collisions: %zu", collisions);

	log_elapsed(begin, end);

	free(ht.items);

	return 0;
}

int main(int argc, char **argv) {
	const char *program = nob_shift_args(&argc, &argv);
	if (argc <= 0) {
		nob_log(NOB_ERROR, "No input provided\n", program);
		nob_log(NOB_ERROR, "Usage: %s <file_path>\n", program);
		return 1;
	}

	printf("Hello, World!\n");
	const char *file_path = nob_shift_args(&argc, &argv);
	Nob_String_Builder buf = {0};

	if (!nob_read_entire_file(file_path, &buf)) return 1;

	Nob_String_View content = {
		.data = buf.items,
		.count = buf.count
	};

	naive_analysis(content);
	// if (hash_analysis(content)) return 1;
	nob_log(NOB_INFO, "Size of %s: %zu Bytes", file_path, buf.count);

	printf("Bye, World!\n");

	return 0;
}

