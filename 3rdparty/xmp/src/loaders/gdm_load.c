/* Extended Module Player
 * Copyright (C) 1996-2013 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

/*
 * Based on the GDM (General Digital Music) version 1.0 File Format
 * Specification - Revision 2 by MenTaLguY
 */

#include <assert.h>
#include "loader.h"
#include "period.h"

#define MAGIC_GDM	MAGIC4('G','D','M',0xfe)
#define MAGIC_GMFS	MAGIC4('G','M','F','S')


static int gdm_test(HIO_HANDLE *, char *, const int);
static int gdm_load (struct module_data *, HIO_HANDLE *, const int);

const struct format_loader gdm_loader = {
	"Generic Digital Music",
	gdm_test,
	gdm_load
};

static int gdm_test(HIO_HANDLE *f, char *t, const int start)
{
	if (hio_read32b(f) != MAGIC_GDM)
		return -1;

	hio_seek(f, start + 0x47, SEEK_SET);
	if (hio_read32b(f) != MAGIC_GMFS)
		return -1;

	hio_seek(f, start + 4, SEEK_SET);
	read_title(f, t, 32);

	return 0;
}



void fix_effect(uint8 *fxt, uint8 *fxp)
{
	switch (*fxt) {
	case 0x00:			/* no effect */
		*fxp = 0;
		break;
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:			/* same as protracker */
		break;
	case 0x08:
		*fxt = FX_TREMOR;
		break;
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:			/* same as protracker */
		break;
	case 0x10:			/* arpeggio */
		*fxt = FX_ARPEGGIO;
		break;
	case 0x11:			/* set internal flag */
		*fxt = *fxp = 0;
		break;
	case 0x12:
		*fxt = FX_MULTI_RETRIG;
		break;
	case 0x13:
		*fxt = FX_GLOBALVOL;
		break;
	case 0x14:
		*fxt = FX_FINE_VIBRATO;
		break;
	case 0x1e:			/* special misc */
		*fxt = *fxp = 0;
		break;
	case 0x1f:
		*fxt = FX_S3M_BPM;
		break;
	default:
		*fxt = *fxp = 0;
	}
}


static int gdm_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
	struct xmp_module *mod = &m->mod;
	struct xmp_event *event;
	int vermaj, vermin, tvmaj, tvmin, tracker;
	int origfmt, ord_ofs, pat_ofs, ins_ofs, smp_ofs;
	uint8 buffer[32], panmap[32];
	int i;

	LOAD_INIT();

	hio_read32b(f);		/* skip magic */
	hio_read(mod->name, 1, 32, f);
	hio_read(mod->author, 1, 32, f);

	hio_seek(f, 7, SEEK_CUR);

	vermaj = hio_read8(f);
	vermin = hio_read8(f);
	tracker = hio_read16l(f);
	tvmaj = hio_read8(f);
	tvmin = hio_read8(f);

	if (tracker == 0) {
		set_type(m, "GDM %d.%02d (2GDM %d.%02d)",
					vermaj, vermin, tvmaj, tvmin);
	} else {
		set_type(m, "GDM %d.%02d (unknown tracker %d.%02d)",
					vermaj, vermin, tvmaj, tvmin);
	}

	hio_read(panmap, 32, 1, f);
	for (i = 0; i < 32; i++) {
		if (panmap[i] == 16)
			panmap[i] = 8;
		mod->xxc[i].pan = 0x80 + (panmap[i] - 8) * 16;
	}

	mod->chn = 32;
	mod->gvl = hio_read8(f);
	mod->spd = hio_read8(f);
	mod->bpm = hio_read8(f);
	origfmt = hio_read16l(f);
	ord_ofs = hio_read32l(f);
	mod->len = hio_read8(f) + 1;
	pat_ofs = hio_read32l(f);
	mod->pat = hio_read8(f) + 1;
	ins_ofs = hio_read32l(f);
	smp_ofs = hio_read32l(f);
	mod->ins = mod->smp = hio_read8(f) + 1;
	mod->trk = mod->pat * mod->chn;
	
	MODULE_INFO();

	hio_seek(f, start + ord_ofs, SEEK_SET);

	for (i = 0; i < mod->len; i++)
		mod->xxo[i] = hio_read8(f);

	/* Read instrument data */

	hio_seek(f, start + ins_ofs, SEEK_SET);

	if (instrument_init(mod) < 0)
		return -1;

	for (i = 0; i < mod->ins; i++) {
		int flg, c4spd, vol, pan;

		if (subinstrument_alloc(mod, i, 1) < 0)
			return -1;

		hio_read(buffer, 32, 1, f);
		instrument_name(mod, i, buffer, 32);
		hio_seek(f, 12, SEEK_CUR);		/* skip filename */
		hio_read8(f);			/* skip EMS handle */
		mod->xxs[i].len = hio_read32l(f);
		mod->xxs[i].lps = hio_read32l(f);
		mod->xxs[i].lpe = hio_read32l(f);
		flg = hio_read8(f);
		c4spd = hio_read16l(f);
		vol = hio_read8(f);
		pan = hio_read8(f);
		
		mod->xxi[i].sub[0].vol = vol > 0x40 ? 0x40 : vol;
		mod->xxi[i].sub[0].pan = pan > 15 ? 0x80 : 0x80 + (pan - 8) * 16;
		c2spd_to_note(c4spd, &mod->xxi[i].sub[0].xpo, &mod->xxi[i].sub[0].fin);

		mod->xxi[i].sub[0].sid = i;
		mod->xxs[i].flg = 0;


		if (mod->xxs[i].len > 0)
			mod->xxi[i].nsm = 1;

		if (flg & 0x01) {
			mod->xxs[i].flg |= XMP_SAMPLE_LOOP;
		}
		if (flg & 0x02) {
			mod->xxs[i].flg |= XMP_SAMPLE_16BIT;
			mod->xxs[i].len >>= 1;
			mod->xxs[i].lps >>= 1;
			mod->xxs[i].lpe >>= 1;
		}

		D_(D_INFO "[%2X] %-32.32s %05x%c%05x %05x %c V%02x P%02x %5d",
				i, mod->xxi[i].name,
				mod->xxs[i].len,
				mod->xxs[i].flg & XMP_SAMPLE_16BIT ? '+' : ' ',
				mod->xxs[i].lps,
				mod->xxs[i].lpe,
				mod->xxs[i].flg & XMP_SAMPLE_LOOP ? 'L' : ' ',
				mod->xxi[i].sub[0].vol,
				mod->xxi[i].sub[0].pan,
				c4spd);
	}

	/* Read and convert patterns */

	hio_seek(f, start + pat_ofs, SEEK_SET);

	if (pattern_init(mod) < 0)
		return -1;

	D_(D_INFO "Stored patterns: %d", mod->pat);

	for (i = 0; i < mod->pat; i++) {
		int len, c, r, k;

		if (pattern_tracks_alloc(mod, i, 64) < 0)
			return -1;

		len = hio_read16l(f);
		len -= 2;

		for (r = 0; len > 0; ) {
			c = hio_read8(f);
			len--;

			if (c == 0) {
				r++;
				continue;
			}

			assert((c & 0x1f) < mod->chn);
			event = &EVENT (i, c & 0x1f, r);

			if (c & 0x20) {		/* note and sample follows */
				k = hio_read8(f);
				event->note = 12 + 12 * MSN(k & 0x7f) + LSN(k);
				event->ins = hio_read8(f);
				len -= 2;
			}

			if (c & 0x40) {		/* effect(s) follow */
				do {
					k = hio_read8(f);
					len--;
					switch ((k & 0xc0) >> 6) {
					case 0:
						event->fxt = k & 0x1f;
						event->fxp = hio_read8(f);
						len--;
						fix_effect(&event->fxt, &event->fxp);
						break;
					case 1:
						event->f2t = k & 0x1f;
						event->f2p = hio_read8(f);
						len--;
						fix_effect(&event->f2t, &event->f2p);
						break;
					case 2:
						hio_read8(f);
						len--;
					}
				} while (k & 0x20);
			}
		}
	}

	/* Read samples */

	hio_seek(f, start + smp_ofs, SEEK_SET);

	D_(D_INFO "Stored samples: %d", mod->smp);

	for (i = 0; i < mod->ins; i++) {
		if (load_sample(m, f, SAMPLE_FLAG_UNS, &mod->xxs[i], NULL) < 0)
			return -1;
	}

	return 0;
}
