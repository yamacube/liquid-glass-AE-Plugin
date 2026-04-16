/*******************************************************************/
/*                                                                 */
/*                       LIQUID GLASS                              */
/*                                                                 */
/*   iOS 26 Liquid Glass Effect for Adobe After Effects            */
/*                                                                 */
/*******************************************************************/

#pragma once

#ifndef LIQUIDGLASS_H
#define LIQUIDGLASS_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;

#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "AEFX_SuiteHelper.h"

#include "LiquidGlass_Strings.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Versioning */
#define	MAJOR_VERSION	4
#define	MINOR_VERSION	0
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1

/* ===== Parameter IDs ===== */
enum {
	LG_INPUT = 0,
	LG_SHAPE_LAYER,
	LG_BG_LAYER,

	/* --- Shape Control --- */
	LG_SHAPE_START,
	LG_SOURCE_MODE,
	LG_GEOMETRY_RADIUS,
	LG_EXPANSION,
	LG_SURFACE_TENSION,
	LG_TENSION_THRESHOLD,
	LG_CLIP_COMP_BOUNDS,
	LG_SHAPE_END,

	/* --- Refraction --- */
	LG_REFRACT_START,
	LG_BEVEL_WIDTH,
	LG_BEVEL_FALLOFF,
	LG_REFRACT_STRENGTH,
	LG_CHROMATIC_AB,
	LG_CHROM_DECAY,
	LG_NOISE_AMOUNT,
	LG_REFRACT_END,

	/* --- Glass Surface --- */
	LG_GLASS_START,
	LG_FROST_BLUR,
	LG_SPECULAR_INTENSITY,
	LG_LIGHT_ANGLE,
	LG_SATURATION,
	LG_TINT_COLOR,
	LG_TINT_OPACITY,
	LG_GLASS_END,

	/* --- Inner Shadow --- */
	LG_ISHADOW_START,
	LG_ISHADOW_SPREAD,
	LG_ISHADOW_BLUR,
	LG_ISHADOW_COLOR,
	LG_ISHADOW_END,

	/* --- Outer Shadow --- */
	LG_OSHADOW_START,
	LG_OSHADOW_SPREAD,
	LG_OSHADOW_BLUR,
	LG_OSHADOW_COLOR,
	LG_OSHADOW_END,

	LG_NUM_PARAMS
};

/* ===== Disk IDs (must be unique, never reorder) ===== */
enum {
	SHAPE_LAYER_DISK_ID = 50,
	CHROM_DECAY_DISK_ID = 51,
	BEVEL_FALLOFF_DISK_ID = 52,
	BG_LAYER_DISK_ID = 53,
	SHAPE_START_DISK_ID = 54,
	SOURCE_MODE_DISK_ID = 60,
	GEOMETRY_RADIUS_DISK_ID = 61,
	EXPANSION_DISK_ID = 62,
	SURFACE_TENSION_DISK_ID = 55,
	TENSION_THRESHOLD_DISK_ID = 56,
	SHAPE_END_DISK_ID = 57,
	CORNER_RADIUS_DISK_ID = 58,
	CLIP_COMP_BOUNDS_DISK_ID = 59,
	REFRACT_START_DISK_ID = 1,
	BEVEL_WIDTH_DISK_ID,
	REFRACT_STRENGTH_DISK_ID,
	CHROMATIC_AB_DISK_ID,
	NOISE_AMOUNT_DISK_ID,
	REFRACT_END_DISK_ID,

	GLASS_START_DISK_ID,
	FROST_BLUR_DISK_ID,
	SPECULAR_INTENSITY_DISK_ID,
	LIGHT_ANGLE_DISK_ID,
	SATURATION_DISK_ID,
	TINT_COLOR_DISK_ID,
	TINT_OPACITY_DISK_ID,
	GLASS_END_DISK_ID,

	ISHADOW_START_DISK_ID,
	ISHADOW_SPREAD_DISK_ID,
	ISHADOW_BLUR_DISK_ID,
	ISHADOW_COLOR_DISK_ID,
	ISHADOW_END_DISK_ID,

	OSHADOW_START_DISK_ID,
	OSHADOW_SPREAD_DISK_ID,
	OSHADOW_BLUR_DISK_ID,
	OSHADOW_COLOR_DISK_ID,
	OSHADOW_END_DISK_ID,
};

/* ===== Parameter Defaults ===== */

/* Refraction */
#define LG_BEVEL_WIDTH_MIN		1.0
#define LG_BEVEL_WIDTH_MAX		1000.0
#define LG_BEVEL_WIDTH_DFLT		500.0

#define LG_BEVEL_FALLOFF_MIN	10.0
#define LG_BEVEL_FALLOFF_MAX	500.0
#define LG_BEVEL_FALLOFF_DFLT	300.0

#define LG_REFRACT_STR_MIN		0.0
#define LG_REFRACT_STR_MAX		400.0
#define LG_REFRACT_STR_DFLT		200.0

#define LG_CHROM_AB_MIN			0.0
#define LG_CHROM_AB_MAX			100.0
#define LG_CHROM_AB_DFLT		50.0

#define LG_CHROM_DECAY_MIN		10.0
#define LG_CHROM_DECAY_MAX		1000.0
#define LG_CHROM_DECAY_DFLT		500.0

#define LG_NOISE_AMT_MIN		0.0
#define LG_NOISE_AMT_MAX		100.0
#define LG_NOISE_AMT_DFLT		0.0

/* Glass Surface */
#define LG_FROST_BLUR_MIN		0.0
#define LG_FROST_BLUR_MAX		200.0
#define LG_FROST_BLUR_DFLT		0.0

#define LG_SPEC_INT_MIN			0.0
#define LG_SPEC_INT_MAX			100.0
#define LG_SPEC_INT_DFLT		12.0

#define LG_LIGHT_ANGLE_DFLT		315.0

#define LG_SAT_MIN				0.0
#define LG_SAT_MAX				200.0
#define LG_SAT_DFLT				0.0

#define LG_TINT_OPACITY_MIN		0.0
#define LG_TINT_OPACITY_MAX		100.0
#define LG_TINT_OPACITY_DFLT	8.0

/* Inner Shadow */
#define LG_SHADOW_SPREAD_MIN	-50.0
#define LG_SHADOW_SPREAD_MAX	 50.0

#define LG_SHADOW_BLUR_MIN		0.0
#define LG_SHADOW_BLUR_MAX		100.0

#define LG_ISHADOW_SPREAD_DFLT	20.0
#define LG_ISHADOW_BLUR_DFLT	40.0

/* Shape Control */
#define LG_CORNER_RADIUS_MIN		0.0
/* Source Mode */
#define LG_SOURCE_MODE_MIN			0
#define LG_SOURCE_MODE_MAX			1
#define LG_SOURCE_MODE_DFLT			0
#define LG_SOURCE_EXTERNAL			0
#define LG_SOURCE_SELF				1

/* Geometry Radius (formerly Corner Radius) */
#define LG_GEOMETRY_RADIUS_MIN		0.0
#define LG_GEOMETRY_RADIUS_MAX		500.0
#define LG_GEOMETRY_RADIUS_DFLT		200.0

/* Expansion (dilate/erode) */
#define LG_EXPANSION_MIN			-100.0
#define LG_EXPANSION_MAX			100.0
#define LG_EXPANSION_DFLT			0.0

/* Legacy Corner Radius (keep for compatibility) */
#define LG_CORNER_RADIUS_MAX		500.0
#define LG_CORNER_RADIUS_DFLT		200.0

#define LG_SURFACE_TENSION_MIN		0.0
#define LG_SURFACE_TENSION_MAX		100.0
#define LG_SURFACE_TENSION_DFLT		0.0

#define LG_TENSION_THRESH_MIN		1.0
#define LG_TENSION_THRESH_MAX		255.0
#define LG_TENSION_THRESH_DFLT		128.0

#define LG_CLIP_COMP_BOUNDS_DFLT	TRUE

/* Outer Shadow */
#define LG_OSHADOW_SPREAD_DFLT	15.0
#define LG_OSHADOW_BLUR_DFLT	30.0


extern "C" {

	DllExport
	PF_Err
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

/* Helper functions */
static inline PF_FpLong LG_Clamp(PF_FpLong val, PF_FpLong mn, PF_FpLong mx) {
	return (val < mn) ? mn : ((val > mx) ? mx : val);
}

static inline A_u_char LG_ClampByte(PF_FpLong val) {
	return (A_u_char)LG_Clamp(val, 0.0, 255.0);
}

static inline A_u_short LG_Clamp16(PF_FpLong val) {
	return (A_u_short)LG_Clamp(val, 0.0, 32768.0);
}

#endif // LIQUIDGLASS_H
