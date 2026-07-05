/**
 * @brief   Bitplane the ghosts
 * @verbose Demonstrate how bitplanes work pt 2
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license GPL v. 3, see LICENSE for details.
 *
 * C99 / amiga-gcc (m68k-amigaos) port of the original Lattice C test harness.
 * Behaviour is unchanged; only the language surface was modernised:
 *   - K&R function definitions -> ANSI/C99 prototypes and definitions
 *   - implicit-int main()      -> int main(void)
 *   - proto headers pulled in so the compiler sees real library prototypes
 *     (the manual OpenLibrary calls and global bases are kept: the proto
 *      inline stubs resolve against these globals, exactly as before)
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "font.h"
#include "Bobs.h"

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

/** The Test Harness *******************************************************/

struct Screen *sPacMan;
struct Window *wPacMan;

struct NewScreen nsPacMan =
{
	0, 0,
	320, 200, 4,
	0, 1,
	NULL,
	CUSTOMSCREEN,
	NULL,
	(UBYTE *)"ANATOMY OF A GHOST (BITPLANES)",
	NULL,
	NULL
};

struct NewWindow nwPacMan =
{
	0, 12,
	320, 188,
	0, 1,
	CLOSEWINDOW,
	ACTIVATE|WINDOWCLOSE,
	NULL,
	NULL,
	(UBYTE *)"PRESS TO CLOSE",
	NULL,
	NULL,
	320,200,320,200,
	CUSTOMSCREEN
};

struct TextAttr taFont = {
	(STRPTR)"Arcade8.font",
	8,
	FS_NORMAL,
	FPF_DESIGNED
};

USHORT colorTable[16] =
{
	0x000,		/* 0: Black */
	0xFF0,		/* 1: Yellow */
	0xF00,		/* 2: Light Red */
	0xFFF,		/* 3: White */
	0x00F,		/* 4: Light Blue */
	0xF0F,		/* 5: Light Magenta */
	0x0FF,		/* 6: Light Cyan */
	0xF80,		/* 7: Dark Yellow (pale orange?) */
	0x00F,		/* 8: Light Green */
	0xFA0,		/* 9: Light Orange */
	0x777,		/* A: Pinky white */
	0x00F,		/* B: Light Green */
	0x0FF,		/* C: Light Cyan */
	0xF00,		/* D: Light Red */
	0xF0F,		/* E: Light Magenta */
	0xFFF,		/* F: White */
};

struct Image iClyde =
{
	0, 0,
	16, 16, 4,
	bmClyde_r00,
	0x0F, 0x00,
	NULL
};

struct Image iClyde2 =
{
	0, 0,
	16, 16, 4,
	bmClyde_r01,
	0x0F, 0x00,
	NULL
};

/*
 * A 16x16, 4-plane image = 16 rows x 4 planes = 64 words of source data.
 */
#define GHOST_WORDS (16 * 4)
#define GHOST_BYTES (GHOST_WORDS * (ULONG)sizeof(UWORD))

/**
 * Copy image source data into freshly-allocated Chip RAM.
 *
 * DrawImage() copies its source through the blitter, and the blitter can only
 * READ from Chip RAM. When the machine has non-chip RAM (FS-UAE's default A500
 * is 512K chip + 512K slow), AmigaDOS LoadSeg() may place the program's data
 * hunk in slow RAM, so blitting bmClyde_* straight from the data segment feeds
 * the blitter garbage -> scattered pixels instead of a ghost. Staging the data
 * in a MEMF_CHIP buffer fixes it on every memory configuration. (A real 512K
 * A500 is all-chip, which is why the original Lattice build "just worked".)
 */
static UWORD *copyToChip(const UWORD *src)
{
	UWORD *dst = (UWORD *)AllocMem(GHOST_BYTES, MEMF_CHIP);
	if (dst)
		CopyMem((APTR)src, dst, GHOST_BYTES);
	return dst;
}

/**
 * Draw one bitplane of an Image at a time (columns 32/64/96/128), then the
 * fully-composited Image at column 160. The trick: temporarily lie to
 * DrawImage about the Image's Depth/PlanePick/ImageData so each call paints
 * exactly one plane's worth of data into a single destination plane.
 */
static void drawPlanes(struct Image *img, SHORT y)
{
	SHORT oldDepth, oldPlanePick;
	UWORD *oldImageData;

	oldDepth = img->Depth;
	oldPlanePick = img->PlanePick;
	oldImageData = img->ImageData;

	if (oldPlanePick & 0x01)
	{
		img->Depth = 1;
		img->PlanePick = 0x1;
		DrawImage(wPacMan->RPort,img,32,y);
		img->ImageData += 16;
	}

	if (oldPlanePick & 0x02)
	{
		img->PlanePick = 0x02;
		DrawImage(wPacMan->RPort,img,64,y);
		img->ImageData += 16;
	}

	if (oldPlanePick & 0x04)
	{
		img->PlanePick = 0x04;
		DrawImage(wPacMan->RPort,img,96,y);
		img->ImageData += 16;
	}

	if (oldPlanePick & 0x08)
	{
		img->PlanePick = 0x08;
		DrawImage(wPacMan->RPort,img,128,y);
		img->ImageData += 16;
	}

	img->Depth = oldDepth;
	img->PlanePick = oldPlanePick;
	img->ImageData = oldImageData;
	DrawImage(wPacMan->RPort,img,160,y);
}

char *colorLegend="0123456789ABCDEF";

static void drawColors(void)
{
	int i;

	SetOPen(wPacMan->RPort, 0); /* Black border around color chips */

	for (i=0;i<16;i++)
	{
		SetAPen(wPacMan->RPort,i);
		RectFill(wPacMan->RPort,312-16,(i<<3)+12,320-16,((i<<3)+8)+12);
		Move(wPacMan->RPort,300-16,(i<<3)+12+7);
		Text(wPacMan->RPort,colorLegend++,1);
	}

}

#define LEGEND_Y 160

static void drawLegends(void)
{
	SetAPen(wPacMan->RPort,0x0F);

	Move(wPacMan->RPort,32,LEGEND_Y);
	Text(wPacMan->RPort,"0",1);
	Move(wPacMan->RPort,64,LEGEND_Y);
	Text(wPacMan->RPort,"1",1);
	Move(wPacMan->RPort,96,LEGEND_Y);
	Text(wPacMan->RPort,"2",1);
	Move(wPacMan->RPort,128,LEGEND_Y);
	Text(wPacMan->RPort,"3",1);
	Move(wPacMan->RPort,160,LEGEND_Y);
	Text(wPacMan->RPort,"ALL",3);

	Move(wPacMan->RPort,32,LEGEND_Y+8);
	Text(wPacMan->RPort,"1",1);
	Move(wPacMan->RPort,64,LEGEND_Y+8);
	Text(wPacMan->RPort,"2",1);
	Move(wPacMan->RPort,96,LEGEND_Y+8);
	Text(wPacMan->RPort,"4",1);
	Move(wPacMan->RPort,128,LEGEND_Y+8);
	Text(wPacMan->RPort,"8",1);
}

/**
 * The test harness.
 */
int main(void)
{
	UWORD *chipClyde  = NULL;
	UWORD *chipClyde2 = NULL;

	GfxBase = (struct GfxBase *)
		OpenLibrary("graphics.library",0);

	if (!GfxBase)
		goto bye;

	IntuitionBase = (struct IntuitionBase *)
		OpenLibrary("intuition.library",0);

	if (!IntuitionBase)
		goto bye;

	/* Stage the ghost bitmaps in Chip RAM so DrawImage's blitter can read
	 * them regardless of where LoadSeg placed the data hunk. */
	chipClyde  = copyToChip(bmClyde_r00);
	chipClyde2 = copyToChip(bmClyde_r01);
	if (!chipClyde || !chipClyde2)
		goto bye;
	iClyde.ImageData  = chipClyde;
	iClyde2.ImageData = chipClyde2;

	FontInit_Arcade8();
	AddFont(&Arcade8Font);

	nsPacMan.Font = &taFont;

	sPacMan = OpenScreen(&nsPacMan);
	if (!sPacMan)
		goto bye;

	/* Load playfield color palette */
	LoadRGB4(&sPacMan->ViewPort, colorTable, 16);

	/* Set our mouse (sprite 0 colors) */
	SetRGB4(&sPacMan->ViewPort,0x11,0x07,0x07,0x00);
	SetRGB4(&sPacMan->ViewPort,0x13,0x0F,0x0F,0x00);

	nwPacMan.Screen = sPacMan;
	wPacMan = OpenWindow(&nwPacMan);
	if (!wPacMan)
		goto bye;

	drawColors();
	drawLegends();

/*	drawPlanes(&iBlinky,32);
	drawPlanes(&iPinky,48);
	drawPlanes(&iInky,64); */
	drawPlanes(&iClyde2,64);
	drawPlanes(&iClyde,80);

	Wait(1<<wPacMan->UserPort->mp_SigBit);

bye:
	RemFont(&Arcade8Font);

	/* Intuition will drain the message port for us. */
	if (wPacMan)
		CloseWindow(wPacMan);

	if (sPacMan)
		CloseScreen(sPacMan);

	if (chipClyde)
		FreeMem(chipClyde, GHOST_BYTES);

	if (chipClyde2)
		FreeMem(chipClyde2, GHOST_BYTES);

	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);

	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);

	return 0;
}
