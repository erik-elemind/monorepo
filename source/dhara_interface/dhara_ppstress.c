/* Dhara portable pseudo-random stress test
 * Copyright (C) 2022 Igor Institute LLC
 *
 * Author: Daniel Beer <daniel.beer@igorinstitute.com>
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nand_W25N04KW.h"
#include "dhara_utils.h"
#include "dhara_ppstress.h"

#ifdef ENABLE_DHARA_PPSTRESS

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/************************************************************************
 * Disk IO layer: wrap as appropriate for platform
 */

static struct dhara_map *test_map;
static uint8_t io_buf[NAND_PAGE_SIZE];

/************************************************************************
 * PRNG
 */

struct xorwow_state {
	uint32_t a, b, c, d;
	uint32_t counter;
};

static uint32_t xorwow_next(struct xorwow_state *state)
{
	/* Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs" */
	uint32_t t = state->d;

	const uint32_t s = state->a;

	state->d = state->c;
	state->c = state->b;
	state->b = s;

	t ^= t >> 2;
	t ^= t << 1;
	t ^= s ^ (s << 4);
	state->a = t;

	state->counter += 362437;
	return t + state->counter;
}

static void xorwow_init(struct xorwow_state *s, uint64_t seed)
{
	s->a = 0xc2ec1bb2;
	s->b = seed >> 32;
	s->c = 0x4af1d4db;
	s->d = seed;
	s->counter = 0;

	for (int i = 0; i < 16; i++)
		xorwow_next(s);
}

/************************************************************************
 * Arbitrary N-bit permutation
 */

#define PERMS_SIZE	32

struct perms {
	unsigned int	n;
	uint32_t	x[PERMS_SIZE];
	uint32_t	a[PERMS_SIZE];
};

static void perms_init(struct perms *p, unsigned int n, struct xorwow_state *s)
{
	p->n = n;
	for (int i = 0; i < PERMS_SIZE; i++)
		p->a[i] = xorwow_next(s) & 0xfffffffe;
	for (int i = 0; i < PERMS_SIZE; i++)
		p->x[i] = xorwow_next(s) & 0xfffffffd;
}

static uint32_t perms_f(const struct perms *p, uint32_t x)
{
	const uint32_t mask = (1 << p->n) - 1;

	x &= mask;
	for (int i = 0; i < PERMS_SIZE; i++) {
		x = (x >> 1) | ((x & 1) << (p->n - 1));

		if (x & 1) {
			x += p->a[i];
		}
		if (x & 2) {
			x ^= p->x[i];
		}

		x &= mask;
	}

	return x;
}

/* Not actually used, but useful for verifying that perms_f really is a
 * permutation.
 */
#if 0
static uint32_t perms_inverse(const struct perms *p, uint32_t x)
{
	const uint32_t mask = (1 << p->n) - 1;

	x &= mask;
	for (int i = PERMS_SIZE - 1; i >= 0; i--) {
		if (x & 2) {
			x ^= p->x[i];
		}
		if (x & 1) {
			x -= p->a[i];
		}

		x &= mask;
		x = (x >> (p->n - 1)) | ((x << 1) & mask);
	}

	return x;
}
#endif

/************************************************************************
 * Stress-test primitives
 *
 * We read and write pseudo-random patterns to each page in the set. The
 * combination of page and version arguments gives us a seed that
 * determines the content to be written/expected.
 */

#define LOG(...) printf("ppstress: " __VA_ARGS__)

static inline uint64_t pattern_seed(dhara_page_t page, uint32_t version)
{
	return (((uint64_t)page) << 32) | version;
}

static void pattern_place(dhara_page_t page, uint32_t version)
{
	struct xorwow_state s;
	uint64_t seed = pattern_seed(page, version);

	xorwow_init(&s, seed);

	for (unsigned int i = 0; i < 8; i++) {
		io_buf[i] = seed;
		seed >>= 8;
	}

	for (unsigned int i = 8; i < sizeof(io_buf); i++)
		io_buf[i] = xorwow_next(&s);
}

static int pattern_check(dhara_page_t page, uint32_t version)
{
	uint64_t seed = 0;
	struct xorwow_state s;
	int ret = 0;

	for (unsigned int i = 0; i < 8; i++) {
		seed <<= 8;
		seed |= io_buf[7 - i];
	}

	xorwow_init(&s, seed);

	for (unsigned int i = 8; i < sizeof(io_buf); i++) {
		const uint8_t e = xorwow_next(&s);

		if (io_buf[i] != e) {
			LOG("mismatch on page %d (version %08x) offset %d: "
				"expected %02x, got %02x\n",
			    (int)page, (int)version, i, e, io_buf[i]);
			ret = -1;
			break;
		}
	}

	if (seed != pattern_seed(page, version)) {
		LOG("seed mismatch: page=%d (expected %d), "
		    "version=%08x (expected %08x)\n",
			(int)(seed >> 32), (int)page,
			(int)(seed & 0xffffffff), (int)version);
		ret = -1;
	}

	if (ret < 0) {
		LOG("--- dump of page data follows ---\n");
		for (unsigned int i = 0; i < sizeof(io_buf); i++) {
			printf(" %02x", io_buf[i]);
			if (!((i + 1) & 0x1f)) {
				printf("\n");
			}
		}
	}

	return ret;
}

static int blocktest_write(dhara_page_t page, uint32_t version)
{
	pattern_place(page, version);

	dhara_error_t err = DHARA_E_NONE;

	if (dhara_map_write(test_map, page, io_buf, &err) < 0) {
		LOG("error writing page %d: %d (%s)\n", (int)page, err,
			dhara_strerror(err));
		return -1;
	}

	return 0;
}

static int blocktest_verify(dhara_page_t page, uint32_t version)
{
	dhara_error_t err = DHARA_E_NONE;

	if (dhara_map_read(test_map, page, io_buf, &err) < 0) {
		if (err == DHARA_E_ECC_WARNING) {
			LOG("ECC warning reading page %d\n", (int)page);
		} else {
			LOG("error reading page %d: %d (%s)\n", (int)page, err,
				dhara_strerror(err));
			return -1;
		}
	}

	return pattern_check(page, version);
}

/************************************************************************
 * Stress-test scheduler
 */

#define SHARD_BITS	8
#define NUM_SHARDS	(1 << SHARD_BITS)

/* What would be best is if we could randomly rewrite pages with random
 * data and at any given time know what data they *should* contain. But
 * this would take as much RAM as we have flash.
 *
 * We can cut down the amount of memory required first by using
 * pseudo-random sequences derived from seeds, and just remembering
 * which seed was written to each page. Now we only need 4 bytes per
 * page in RAM.
 *
 * We can cut things down further by compromising on the random write
 * pattern. Rather than arbitrary rewrites of pages, we group pages into
 * a small number of roughly-equal sized shards. Each shard can be
 * rewritten with randomly chosen data, but the pages within each shard
 * are rewritten in sequence.
 *
 * To try to randomize the pattern, we shard *virtual* page numbers,
 * which are then passed through a pseudo-random mapping to the actual
 * page numbers read and written. Our algorithm for doing this expects
 * that the address space is a power of two. To allow for this not to be
 * the case, we take the space we're given and round up to the nearest
 * power of two. Any writes or verifies to a page between the real size
 * and the padded size are silently ignored.
 *
 * The lower SHARD_BITS of a virtual page number are the shard index,
 * and the upper bits are the sequence number within the shard. The
 * entire virtual page number is passed through perms_f() to get a real
 * page number.
 *
 * Within a shard, virtual pages with a sequence number < ptr are
 * expected to contain a page seeded with this_version. All other
 * virtual pages within the shard are expected to contain a page seeded
 * with last_version.
 */
struct shard {
	uint32_t	last_version;
	uint32_t	this_version;
	uint32_t	ptr;
};

static struct shard shards[NUM_SHARDS];

static int ppstress(dhara_page_t max, unsigned int churn, unsigned int seed)
{
	unsigned int bits = SHARD_BITS;

	while (max >> bits)
		bits++;

	const uint32_t shard_size = (1 << (bits - SHARD_BITS));
	struct xorwow_state xs;
	struct perms perm;

	LOG("seed is %d\n", (int)seed);
	LOG("%d pages: %d-bit namespace\n", (int)max, bits);
	LOG("initializing %d shards of size %d\n", NUM_SHARDS,
		(int)shard_size);

	xorwow_init(&xs, seed);
	perms_init(&perm, bits, &xs);

	/* Initial state */
	for (int i = 0; i < NUM_SHARDS; i++) {
		struct shard *s = &shards[i];

		s->last_version = s->this_version = xorwow_next(&xs);
		s->ptr = 0;

		LOG("set up shard %d with version %08x\n",
			i, (int)s->this_version);

		for (int j = 0; j < shard_size; j++) {
			const dhara_page_t p = perms_f(&perm,
				(j << SHARD_BITS) | i);

			if ((p < max) &&
				blocktest_write(p, s->this_version) < 0) {
				LOG("failed to init page %d (#%d)\n",
					(int)p, j);
				goto fail;
			}
		}
	}

	/* Churn pages with random sampling */
	const unsigned int nwrite = max * churn;
	unsigned int lastpc = 0;

	LOG("testing %d read/write cycles (size * %d)\n", nwrite, churn);

	for (int i = 0; i < nwrite; i++) {
		unsigned int pc = i * 100 / nwrite;

		/* Pick a random shard to write */
		unsigned int n = xorwow_next(&xs) & (NUM_SHARDS - 1);
		struct shard *s = &shards[n];
		unsigned int ptr = s->ptr++;
		unsigned int p = perms_f(&perm, (ptr << SHARD_BITS) | n);

		if ((p < max) &&
		    blocktest_write(p, s->this_version) < 0) {
			LOG("failed to write page %d (shard %d, #%d)\n",
				(int)p, n, ptr);
			goto fail;
		}

		if (s->ptr >= shard_size) {
			s->ptr = 0;
			s->last_version = s->this_version;
			s->this_version = xorwow_next(&xs);
		}

		/* Pick a random page to verify */
		n = xorwow_next(&xs) & ((1 << bits) - 1);
		s = &shards[n & (NUM_SHARDS - 1)];
		ptr = n >> SHARD_BITS;
		p = perms_f(&perm, n);
		const uint32_t vexpect = ptr < s->ptr ?
			s->this_version : s->last_version;

		if ((p < max) && blocktest_verify(p, vexpect) < 0) {
			LOG("failed to verify page %d (shard %d, #%d)\n",
				(int)p, n & (NUM_SHARDS - 1), (int)s->ptr);
			goto fail;
		}

		if (pc != lastpc) {
			LOG("%3d%% done\n", pc);
			lastpc = pc;
		}
	}

	/* Final verify */
	LOG("final verification\n");
	for (int i = 0; i < NUM_SHARDS; i++) {
		struct shard *s = &shards[i];

		LOG("verify shard %d: ptr=%d, versions %08x/%08x\n",
			i, (int)s->ptr, (int)s->last_version,
				(int)s->this_version);

		for (int j = 0; j < s->ptr; j++) {
			const dhara_page_t p = perms_f(&perm,
				(j << SHARD_BITS) | i);

			if ((p < max) &&
				blocktest_verify(p, s->this_version) < 0) {
				LOG("failed to verify page %d (#%d)\n",
					(int)p, j);
				goto fail;
			}
		}

		for (int j = s->ptr; j < shard_size; j++) {
			const dhara_page_t p = perms_f(&perm,
				(j << SHARD_BITS) | i);

			if ((p < max) &&
				blocktest_verify(p, s->last_version) < 0) {
				LOG("failed to verify page %d (#%d)\n",
					(int)p, j);
				goto fail;
			}
		}
	}

	LOG("all ok\n");
	return 0;
fail:
	LOG("FAILED\n");
	return -1;
}

/************************************************************************
 * NAND test
 */

/* NAND test sequence state machine.
 *
 * For each page, decide whether we should generate a fresh pattern or
 * copy a previous page. Every fresh page is considered as a candidate
 * for future copying.
 */
struct nts {
	dhara_page_t		sources[32];
	struct xorwow_state	gen;
	int			is_copy;
	dhara_page_t		source;
};

static void nts_reset(struct nts *n, unsigned int seed)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(n->sources); i++)
		n->sources[i] = DHARA_PAGE_NONE;

	xorwow_init(&n->gen, seed * 29 + 1231235);
}

static void nts_next_page(struct nts *n, dhara_page_t ptr)
{
	uint32_t t = xorwow_next(&n->gen);
	int c = 0;

	while (t & 1) {
		t >>= 1;
		c++;
	}

	if ((xorwow_next(&n->gen) & 1) && (n->sources[c] != DHARA_PAGE_NONE)) {
		n->source = n->sources[c];
		n->is_copy = 1;
	} else {
		n->sources[c] = n->source = ptr;
		n->is_copy = 0;
	}
}

static int ppnand(unsigned int seed)
{
	const struct dhara_nand *n = test_map->journal.nand;

	LOG("NAND test: seed=%d, blocks=%d (2**%d pages each)\n",
		(int)seed, n->num_blocks, n->log2_ppb);

	LOG("erase/program whole chip\n");

	unsigned int last_pc = 0;
	unsigned int bb_count = 0;
	unsigned int bb_new = 0;
	struct nts nts;

	nts_reset(&nts, seed);
	for (unsigned int i = 0; i < n->num_blocks; i++) {
		unsigned int pc = i * 100 / n->num_blocks;
		dhara_error_t err = DHARA_E_NONE;

		if (pc != last_pc) {
			LOG("%3d%% programmed\n", pc);
			last_pc = pc;
		}

		if (dhara_nand_is_bad(n, i)) {
			LOG("skipping bad block %d\n", i);
			bb_count++;
			continue;
		}

		if (dhara_nand_erase(n, i, &err) < 0) {
			LOG("erase of block %d failed: %d (%s)\n",
				i, err, dhara_strerror(err));

			if (err != DHARA_E_BAD_BLOCK) {
				goto fail;
			}

			bb_new++;
			LOG("marking block %d as bad\n", i);
			dhara_nand_mark_bad(n, i);
			continue;
		}

		for (unsigned int j = 0; !(j >> n->log2_ppb); j++) {
			const dhara_page_t p = (i << n->log2_ppb) | j;

			nts_next_page(&nts, p);

			/* Our copy test requires that blocks don't
			 * become bad mid-program in order to reproduce
			 * the same sequence during the read phase
			 */
			if (nts.is_copy) {
				if (dhara_nand_copy(n, nts.source, p,
					&err) < 0) {
					LOG("copy of page %d -> %d failed: "
					    "%d (%s)\n",
					    (int)nts.source, (int)p,
					    err, dhara_strerror(err));
					goto fail;
				}
			} else {
				pattern_place(nts.source, seed);
				if (dhara_nand_prog(n, p, io_buf, &err) < 0) {
					LOG("prog of page %d failed: %d (%s)\n",
						(int)p, err,
						dhara_strerror(err));
					goto fail;
				}
			}
		}
	}

	LOG("verify whole chip (full pages)\n");

	last_pc = 0;
	nts_reset(&nts, seed);
	for (unsigned int i = 0; i < n->num_blocks; i++) {
		unsigned int pc = i * 100 / n->num_blocks;
		dhara_error_t err = DHARA_E_NONE;

		if (pc != last_pc) {
			LOG("%3d%% verified\n", pc);
			last_pc = pc;
		}

		if (dhara_nand_is_bad(n, i)) {
			LOG("skipping bad block %d\n", i);
			continue;
		}

		for (unsigned int j = 0; !(j >> n->log2_ppb); j++) {
			const dhara_page_t p = (i << n->log2_ppb) | j;

			nts_next_page(&nts, p);

			if (dhara_nand_read(n, p, 0, sizeof(io_buf),
						io_buf, &err) < 0) {
				if (err == DHARA_E_ECC_WARNING) {
					LOG("ECC warning on page %d\n", (int)p);
				} else {
					LOG("read of page %d failed: %d (%s)\n",
						(int)p, err, dhara_strerror(err));
					goto fail;
				}
			}

			if (pattern_check(nts.source, seed) < 0)
				goto fail;
		}
	}

	LOG("verify whole chip (partial pages)\n");

	struct xorwow_state sroff;
	xorwow_init(&sroff, seed * 33);

	last_pc = 0;
	nts_reset(&nts, seed);
	for (unsigned int i = 0; i < n->num_blocks; i++) {
		unsigned int pc = i * 100 / n->num_blocks;
		dhara_error_t err = DHARA_E_NONE;

		if (pc != last_pc) {
			LOG("%3d%% verified\n", pc);
			last_pc = pc;
		}

		if (dhara_nand_is_bad(n, i)) {
			LOG("skipping bad block %d\n", i);
			continue;
		}

		for (unsigned int j = 0; !(j >> n->log2_ppb); j++) {
			const dhara_page_t p = (i << n->log2_ppb) | j;

			nts_next_page(&nts, p);
			pattern_place(nts.source, seed);

			uint8_t subbuf[256];
			unsigned int rlen = xorwow_next(&sroff) %
				(sizeof(subbuf) - 1) + 1;
			unsigned int roff = xorwow_next(&sroff) %
				(sizeof(io_buf) - rlen);

			if (dhara_nand_read(n, p, roff, rlen, subbuf,
						&err) < 0) {
				if (err == DHARA_E_ECC_WARNING) {
					LOG("ECC warning on page %d "
						"(partial read)\n", (int)p);
				} else {
					LOG("read of page %d failed: %d (%s)\n",
						(int)p, err, dhara_strerror(err));
					goto fail;
				}
			}

			if (memcmp(io_buf + roff, subbuf, rlen)) {
				LOG("sub-buffer mismatch (page %d, "
					"offset %d, len %d)\n",
					(int)p, roff, rlen);
				goto fail;
			}
		}
	}

	LOG("skipped %d bad blocks\n", bb_count);
	LOG("marked %d new bad blocks\n", bb_new);
	LOG("all ok\n");
	return 0;
fail:
	LOG("nts.is_copy = %d, nts.source = %d\n",
		nts.is_copy, (int)nts.source);
	LOG("FAILED\n");
	return -1;
}

/************************************************************************
 * Test commands
 */

void dhara_cmd_stat(int argc, char **argv)
{
	struct dhara_map *m = dhara_get_my_map();

	printf("Dhara state:\n");
	printf("  map occupancy: %d of %d\n",
		(int)m->count, (int)dhara_map_capacity(m));
	printf("  gc_ratio: %d\n", m->gc_ratio);
	printf("  epoch: %d\n", m->journal.epoch);
	printf("  ppc: 2**%d\n", m->journal.log2_ppc);
	printf("  flags: 0x%02x", m->journal.flags);
	if (m->journal.flags & DHARA_JOURNAL_F_DIRTY) {
		printf(" DIRTY");
	}
	if (m->journal.flags & DHARA_JOURNAL_F_BAD_META) {
		printf(" BAD_META");
	}
	if (m->journal.flags & DHARA_JOURNAL_F_RECOVERY) {
		printf(" RECOVERY");
	}
	if (m->journal.flags & DHARA_JOURNAL_F_ENUM_DONE) {
		printf(" ENUM_DONE");
	}
	printf("  bb_current: %d\n", (int)m->journal.bb_current);
	printf("  head: 0x%08x\n", (int)m->journal.head);
	printf("  tail: 0x%08x\n", (int)m->journal.tail);
	printf("  root: 0x%08x\n", (int)m->journal.root);
}

void dhara_cmd_ppstress(int argc, char **argv)
{
	test_map = dhara_get_my_map();

	int set_size = dhara_map_capacity(test_map) / 2;
	int churn = 4;
	int seed = 1;

	if (argc >= 2) {
		set_size = atoi(argv[1]);
	}
	if (argc >= 3) {
		churn = atoi(argv[2]);
	}
	if (argc >= 4) {
		seed = atoi(argv[3]);
	}

	ppstress(set_size, churn, seed);
}

void dhara_cmd_ppnand(int argc, char **argv)
{
	int seed = 1;

	if (argc >= 2) {
		seed = atoi(argv[1]);
	}

	test_map = dhara_get_my_map();
	ppnand(seed);
}

void dhara_cmd_bbmap(int argc, char **argv)
{
	const struct dhara_nand *n = dhara_get_my_map()->journal.nand;
	unsigned int bb_count = 0;

	LOG("bad blocks are:\n");
	for (int i = 0; i < n->num_blocks; i++) {
		if (dhara_nand_is_bad(n, i)) {
			LOG("  %d\n", i);
			bb_count++;
		}
	}
	LOG("%d total\n", bb_count);
}

void dhara_cmd_blkerase(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: dhara_blkerase <block>\n");
		return;
	}

	const struct dhara_nand *n = dhara_get_my_map()->journal.nand;
	dhara_error_t err = DHARA_E_NONE;
	dhara_block_t blk = atoi(argv[1]);

	LOG("erase block %d\n", (int)blk);
	if (dhara_nand_erase(n, blk, &err) < 0) {
		LOG("erase failed: %d (%s)\n", err, dhara_strerror(err));
	}
}

void dhara_cmd_blkmark(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: dhara_blkmark <block>\n");
		return;
	}

	const struct dhara_nand *n = dhara_get_my_map()->journal.nand;
	dhara_block_t blk = atoi(argv[1]);

	LOG("mark block %d\n", (int)blk);
	dhara_nand_mark_bad(n, blk);
}

void dhara_cmd_eraseall(int argc, char **argv)
{
	const struct dhara_nand *n = dhara_get_my_map()->journal.nand;

	LOG("erase all blocks\n");

	unsigned int last_pc = 0;
	unsigned int bb_new = 0;

	for (unsigned int i = 0; i < n->num_blocks; i++) {
		unsigned int pc = i * 100 / n->num_blocks;
		dhara_error_t err;

		if (pc != last_pc) {
			LOG("%3d%% erased\n", pc);
			last_pc = pc;
		}

		if (dhara_nand_erase(n, i, &err) < 0) {
			LOG("erase of block %d failed: %d (%s)\n",
				i, err, dhara_strerror(err));

			if (err != DHARA_E_BAD_BLOCK)
				return;

			bb_new++;
			LOG("marking block %d as bad\n", i);
			dhara_nand_mark_bad(n, i);
			continue;
		}
	}

	LOG("done, %d blocks marked bad\n", bb_new);
}

#endif /* ENABLE_DHARA_PPSTRESS */
