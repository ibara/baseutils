/* $OpenBSD: lr0.c,v 1.18 2014/03/13 01:18:22 tedu Exp $	 */
/* $NetBSD: lr0.c,v 1.4 1996/03/19 03:21:35 jtc Exp $	 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Paul Corbett.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "defs.h"

extern short *itemset;
extern short *itemsetend;
extern unsigned *ruleset;

int nstates;
core *first_state;
shifts *first_shift;
reductions *first_reduction;

short get_state(int);
core *new_state(int);

void allocate_itemsets(void);
void allocate_storage(void);
void append_states(void);
void free_storage(void);
void generate_states(void);
void initialize_states(void);
void new_itemsets(void);
void save_shifts(void);
void save_reductions(void);
void set_derives(void);
void print_derives(void);
void set_nullable(void);

static core **state_set;
static core *this_state;
static core *last_state;
static shifts *last_shift;
static reductions *last_reduction;

static int nshifts;
static short *shift_symbol;

static short *redset;
static short *shiftset;

static short **kernel_base;
static short **kernel_end;
static short *kernel_items;

void
allocate_itemsets(void)
{
	short *itemp, *item_end;
	int i, count, max, symbol;
	short *symbol_count;

	count = 0;
	symbol_count = NEW2(nsyms, short);

	item_end = ritem + nitems;
	for (itemp = ritem; itemp < item_end; itemp++) {
		symbol = *itemp;
		if (symbol >= 0) {
			count++;
			symbol_count[symbol]++;
		}
	}

	kernel_base = NEW2(nsyms, short *);
	kernel_items = NEW2(count, short);

	count = 0;
	max = 0;
	for (i = 0; i < nsyms; i++) {
		kernel_base[i] = kernel_items + count;
		count += symbol_count[i];
		if (max < symbol_count[i])
			max = symbol_count[i];
	}

	shift_symbol = symbol_count;
	kernel_end = NEW2(nsyms, short *);
}

void
allocate_storage(void)
{
	allocate_itemsets();
	shiftset = NEW2(nsyms, short);
	redset = NEW2(nrules + 1, short);
	state_set = NEW2(nitems, core *);
}

void
append_states(void)
{
	int i, j, symbol;

#ifdef	TRACE
	fprintf(stderr, "Entering append_states()\n");
#endif
	for (i = 1; i < nshifts; i++) {
		symbol = shift_symbol[i];
		j = i;
		while (j > 0 && shift_symbol[j - 1] > symbol) {
			shift_symbol[j] = shift_symbol[j - 1];
			j--;
		}
		shift_symbol[j] = symbol;
	}

	for (i = 0; i < nshifts; i++) {
		symbol = shift_symbol[i];
		shiftset[i] = get_state(symbol);
	}
}

void
free_storage(void)
{
	free(shift_symbol);
	free(redset);
	free(shiftset);
	free(kernel_base);
	free(kernel_end);
	free(kernel_items);
	free(state_set);
}


void
generate_states(void)
{
	allocate_storage();
	itemset = NEW2(nitems, short);
	ruleset = NEW2(WORDSIZE(nrules), unsigned);
	set_first_derives();
	initialize_states();

	while (this_state) {
		closure(this_state->items, this_state->nitems);
		save_reductions();
		new_itemsets();
		append_states();

		if (nshifts > 0)
			save_shifts();

		this_state = this_state->next;
	}

	finalize_closure();
	free_storage();
}



short
get_state(int symbol)
{
	int n, found, key;
	short *isp1, *isp2, *iend;
	core *sp;

#ifdef	TRACE
	fprintf(stderr, "Entering get_state(%d)\n", symbol);
#endif

	isp1 = kernel_base[symbol];
	iend = kernel_end[symbol];
	n = iend - isp1;

	key = *isp1;
	assert(0 <= key && key < nitems);
	sp = state_set[key];
	if (sp) {
		found = 0;
		while (!found) {
			if (sp->nitems == n) {
				found = 1;
				isp1 = kernel_base[symbol];
				isp2 = sp->items;

				while (found && isp1 < iend) {
					if (*isp1++ != *isp2++)
						found = 0;
				}
			}
			if (!found) {
				if (sp->link) {
					sp = sp->link;
				} else {
					sp = sp->link = new_state(symbol);
					found = 1;
				}
			}
		}
	} else {
		state_set[key] = sp = new_state(symbol);
	}

	return (sp->number);
}


void
initialize_states(void)
{
	int i;
	short *start_derives;
	core *p;

	start_derives = derives[start_symbol];
	for (i = 0; start_derives[i] >= 0; ++i)
		continue;

	p = malloc(sizeof(core) + i * sizeof(short));
	if (p == NULL)
		no_space();

	p->next = 0;
	p->link = 0;
	p->number = 0;
	p->accessing_symbol = 0;
	p->nitems = i;

	for (i = 0; start_derives[i] >= 0; ++i)
		p->items[i] = rrhs[start_derives[i]];

	first_state = last_state = this_state = p;
	nstates = 1;
}

void
new_itemsets(void)
{
	int i, shiftcount;
	short *isp, *ksp;
	int symbol;

	memset(kernel_end, 0, nsyms * sizeof(short *));

	shiftcount = 0;
	isp = itemset;
	while (isp < itemsetend) {
		i = *isp++;
		symbol = ritem[i];
		if (symbol > 0) {
			ksp = kernel_end[symbol];
			if (!ksp) {
				shift_symbol[shiftcount++] = symbol;
				ksp = kernel_base[symbol];
			}
			*ksp++ = i + 1;
			kernel_end[symbol] = ksp;
		}
	}

	nshifts = shiftcount;
}



core *
new_state(int symbol)
{
	int n;
	core *p;
	short *isp1, *isp2, *iend;

#ifdef	TRACE
	fprintf(stderr, "Entering new_state(%d)\n", symbol);
#endif

	if (nstates >= MAXSHORT)
		fatal("too many states");

	isp1 = kernel_base[symbol];
	iend = kernel_end[symbol];
	n = iend - isp1;

	p = allocate(sizeof(core) + (n - 1) * sizeof(short));
	p->accessing_symbol = symbol;
	p->number = nstates;
	p->nitems = n;

	isp2 = p->items;
	while (isp1 < iend)
		*isp2++ = *isp1++;

	last_state->next = p;
	last_state = p;

	nstates++;

	return (p);
}


void
save_shifts(void)
{
	shifts *p;
	short *sp1, *sp2, *send;

	p = allocate(sizeof(shifts) + (nshifts - 1) * sizeof(short));

	p->number = this_state->number;
	p->nshifts = nshifts;

	sp1 = shiftset;
	sp2 = p->shift;
	send = shiftset + nshifts;

	while (sp1 < send)
		*sp2++ = *sp1++;

	if (last_shift) {
		last_shift->next = p;
		last_shift = p;
	} else {
		first_shift = p;
		last_shift = p;
	}
}


void
save_reductions(void)
{
	short *isp, *rp1, *rp2;
	int item, count;
	reductions *p;
	short *rend;

	count = 0;
	for (isp = itemset; isp < itemsetend; isp++) {
		item = ritem[*isp];
		if (item < 0) {
			redset[count++] = -item;
		}
	}

	if (count) {
		p = allocate(sizeof(reductions) + (count - 1) * sizeof(short));

		p->number = this_state->number;
		p->nreds = count;

		rp1 = redset;
		rp2 = p->rules;
		rend = rp1 + count;

		while (rp1 < rend)
			*rp2++ = *rp1++;

		if (last_reduction) {
			last_reduction->next = p;
			last_reduction = p;
		} else {
			first_reduction = p;
			last_reduction = p;
		}
	}
}

void
set_derives(void)
{
	int i, k, lhs;
	short *rules;

	derives = NEW2(nsyms, short *);
	rules = NEW2(nvars + nrules, short);

	k = 0;
	for (lhs = start_symbol; lhs < nsyms; lhs++) {
		derives[lhs] = rules + k;
		for (i = 0; i < nrules; i++) {
			if (rlhs[i] == lhs) {
				rules[k] = i;
				k++;
			}
		}
		rules[k] = -1;
		k++;
	}

#ifdef	DEBUG
	print_derives();
#endif
}

void
free_derives(void)
{
	free(derives[start_symbol]);
	free(derives);
}

#ifdef	DEBUG
void
print_derives(void)
{
	int i;
	short *sp;

	printf("\nDERIVES\n\n");

	for (i = start_symbol; i < nsyms; i++) {
		printf("%s derives ", symbol_name[i]);
		for (sp = derives[i]; *sp >= 0; sp++) {
			printf("  %d", *sp);
		}
		putchar('\n');
	}

	putchar('\n');
}
#endif

void
set_nullable(void)
{
	int i, j;
	int empty;
	int done;

	nullable = calloc(1, nsyms);
	if (nullable == NULL)
		no_space();

	done = 0;
	while (!done) {
		done = 1;
		for (i = 1; i < nitems; i++) {
			empty = 1;
			while ((j = ritem[i]) >= 0) {
				if (!nullable[j])
					empty = 0;
				++i;
			}
			if (empty) {
				j = rlhs[-j];
				if (!nullable[j]) {
					nullable[j] = 1;
					done = 0;
				}
			}
		}
	}

#ifdef DEBUG
	for (i = 0; i < nsyms; i++) {
		if (nullable[i])
			printf("%s is nullable\n", symbol_name[i]);
		else
			printf("%s is not nullable\n", symbol_name[i]);
	}
#endif
}

void
free_nullable(void)
{
	free(nullable);
}

void
lr0(void)
{
	set_derives();
	set_nullable();
	generate_states();
}
