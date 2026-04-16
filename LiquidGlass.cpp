/*******************************************************************/
/*                                                                 */
/*                       LIQUID GLASS  v3.0                        */
/*                                                                 */
/*   iOS 26 Liquid Glass Effect for Adobe After Effects            */
/*                                                                 */
/*   Controls:                                                     */
/*     - Refraction (bevel + chromatic aberration + noise)         */
/*     - Glass Surface (frost blur + specular + saturation + tint) */
/*     - Inner Shadow (spread + blur + color)                      */
/*     - Outer Shadow (spread + blur + color)                      */
/*                                                                 */
/*******************************************************************/

#include "LiquidGlass.h"

/* Inline SmartFX helpers (avoids linking Smart_Utils.cpp) */
static PF_Boolean LG_IsEmptyRect(const PF_LRect *r) {
	return (r->left >= r->right) || (r->top >= r->bottom);
}
static void UnionLRect(const PF_LRect *src, PF_LRect *dst) {
	if (LG_IsEmptyRect(dst)) { *dst = *src; }
	else if (!LG_IsEmptyRect(src)) {
		if (src->left   < dst->left)   dst->left   = src->left;
		if (src->top    < dst->top)    dst->top    = src->top;
		if (src->right  > dst->right)  dst->right  = src->right;
		if (src->bottom > dst->bottom) dst->bottom = src->bottom;
	}
}

/* ===== String accessor (defined in LiquidGlass_Strings.cpp) ===== */
#ifdef __cplusplus
extern "C" {
#endif
char *GetStringPtr(int strNum);
#ifdef __cplusplus
}
#endif
#define STR(id) GetStringPtr(id)


/* ================================================================
 *  About
 * ================================================================ */
static PF_Err
About(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
		"%s v%d.%d\r%s",
		STR(StrID_Name),
		MAJOR_VERSION,
		MINOR_VERSION,
		STR(StrID_Description));
	return PF_Err_NONE;
}

/* ================================================================
 *  GlobalSetup
 * ================================================================ */
static PF_Err
GlobalSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
									  MINOR_VERSION,
									  BUG_VERSION,
									  STAGE_VERSION,
									  BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;

	out_data->out_flags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER;

	return PF_Err_NONE;
}

/* ================================================================
 *  ParamsSetup
 * ================================================================ */
static PF_Err
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err		err = PF_Err_NONE;
	PF_ParamDef	def;

	/* Shape Layer (for adjustment layer / mask support) */
	AEFX_CLR_STRUCT(def);
	def.param_type = PF_Param_LAYER;
	PF_STRCPY(def.name, STR(StrID_Shape_Layer));
	def.u.ld.dephault = PF_LayerDefault_NONE;
	def.uu.id = SHAPE_LAYER_DISK_ID;
	ERR(PF_ADD_PARAM(in_data, -1, &def));

	/* Background Layer (for direct application mode) */
	AEFX_CLR_STRUCT(def);
	def.param_type = PF_Param_LAYER;
	PF_STRCPY(def.name, STR(StrID_BG_Layer));
	def.u.ld.dephault = PF_LayerDefault_NONE;
	def.uu.id = BG_LAYER_DISK_ID;
	ERR(PF_ADD_PARAM(in_data, -1, &def));

	/* --- Shape Control --- */
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_Shape_Group), SHAPE_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Corner_Radius),
						 LG_CORNER_RADIUS_MIN, LG_CORNER_RADIUS_MAX,
						 LG_CORNER_RADIUS_MIN, LG_CORNER_RADIUS_MAX,
						 LG_CORNER_RADIUS_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 CORNER_RADIUS_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Surface_Tension),
						 LG_SURFACE_TENSION_MIN, LG_SURFACE_TENSION_MAX,
						 LG_SURFACE_TENSION_MIN, LG_SURFACE_TENSION_MAX,
						 LG_SURFACE_TENSION_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 SURFACE_TENSION_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tension_Threshold),
						 LG_TENSION_THRESH_MIN, LG_TENSION_THRESH_MAX,
						 LG_TENSION_THRESH_MIN, LG_TENSION_THRESH_MAX,
						 LG_TENSION_THRESH_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 TENSION_THRESHOLD_DISK_ID);

	/* Source Mode: Self Alpha vs External Layer */
	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(STR(StrID_Source_Mode),
				 2,  /* 2 choices */
				 0,  /* default: External Layer */
				 SOURCE_MODE_DISK_ID);
	
	/* Geometry Radius: shape rounding control */
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Geometry_Radius),
						 LG_GEOMETRY_RADIUS_MIN, LG_GEOMETRY_RADIUS_MAX,
						 LG_GEOMETRY_RADIUS_MIN, LG_GEOMETRY_RADIUS_MAX,
						 LG_GEOMETRY_RADIUS_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 GEOMETRY_RADIUS_DISK_ID);
	
	/* Expansion: dilate/erode shape (-100 to 100) */
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Expansion),
						 LG_EXPANSION_MIN, LG_EXPANSION_MAX,
						 LG_EXPANSION_MIN, LG_EXPANSION_MAX,
						 LG_EXPANSION_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 EXPANSION_DISK_ID);

	/* Clip at Comp Bounds: clip edges when glass goes outside comp */
	AEFX_CLR_STRUCT(def);
	PF_STRCPY(def.name, STR(StrID_Clip_Comp_Bounds));
	def.param_type = PF_Param_CHECKBOX;
	def.u.bd.dephault = LG_CLIP_COMP_BOUNDS_DFLT;
	def.uu.id = CLIP_COMP_BOUNDS_DISK_ID;
	ERR(PF_ADD_PARAM(in_data, -1, &def));

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(SHAPE_END_DISK_ID);

	/* --- Refraction --- */
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_Refract_Group), REFRACT_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Bevel_Width),
						 LG_BEVEL_WIDTH_MIN, LG_BEVEL_WIDTH_MAX,
						 LG_BEVEL_WIDTH_MIN, LG_BEVEL_WIDTH_MAX,
						 LG_BEVEL_WIDTH_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 BEVEL_WIDTH_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Bevel_Falloff),
						 LG_BEVEL_FALLOFF_MIN, LG_BEVEL_FALLOFF_MAX,
						 LG_BEVEL_FALLOFF_MIN, LG_BEVEL_FALLOFF_MAX,
						 LG_BEVEL_FALLOFF_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 BEVEL_FALLOFF_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Refract_Strength),
						 LG_REFRACT_STR_MIN, LG_REFRACT_STR_MAX,
						 LG_REFRACT_STR_MIN, LG_REFRACT_STR_MAX,
						 LG_REFRACT_STR_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 REFRACT_STRENGTH_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Chromatic_Ab),
						 LG_CHROM_AB_MIN, LG_CHROM_AB_MAX,
						 LG_CHROM_AB_MIN, LG_CHROM_AB_MAX,
						 LG_CHROM_AB_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 CHROMATIC_AB_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Chrom_Decay),
						 LG_CHROM_DECAY_MIN, LG_CHROM_DECAY_MAX,
						 LG_CHROM_DECAY_MIN, LG_CHROM_DECAY_MAX,
						 LG_CHROM_DECAY_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 CHROM_DECAY_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Noise_Amount),
						 LG_NOISE_AMT_MIN, LG_NOISE_AMT_MAX,
						 LG_NOISE_AMT_MIN, LG_NOISE_AMT_MAX,
						 LG_NOISE_AMT_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 NOISE_AMOUNT_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(REFRACT_END_DISK_ID);

	/* --- Glass Surface --- */
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_Glass_Group), GLASS_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Frost_Blur),
						 LG_FROST_BLUR_MIN, LG_FROST_BLUR_MAX,
						 LG_FROST_BLUR_MIN, 50.0,
						 LG_FROST_BLUR_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 FROST_BLUR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Specular_Intensity),
						 LG_SPEC_INT_MIN, LG_SPEC_INT_MAX,
						 LG_SPEC_INT_MIN, LG_SPEC_INT_MAX,
						 LG_SPEC_INT_DFLT,
						 PF_Precision_TENTHS,
						 0, 0,
						 SPECULAR_INTENSITY_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_ANGLE(STR(StrID_Light_Angle),
				 (A_long)LG_LIGHT_ANGLE_DFLT,
				 LIGHT_ANGLE_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Saturation),
						 LG_SAT_MIN, LG_SAT_MAX,
						 LG_SAT_MIN, LG_SAT_MAX,
						 LG_SAT_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 SATURATION_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(STR(StrID_Tint_Color),
				 30, 32, 38,
				 TINT_COLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_Tint_Opacity),
						 LG_TINT_OPACITY_MIN, LG_TINT_OPACITY_MAX,
						 LG_TINT_OPACITY_MIN, LG_TINT_OPACITY_MAX,
						 LG_TINT_OPACITY_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 TINT_OPACITY_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(GLASS_END_DISK_ID);

	/* --- Inner Shadow --- */
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_InnerShadow_Group), ISHADOW_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_IShadow_Spread),
						 LG_SHADOW_SPREAD_MIN, LG_SHADOW_SPREAD_MAX,
						 LG_SHADOW_SPREAD_MIN, LG_SHADOW_SPREAD_MAX,
						 LG_ISHADOW_SPREAD_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 ISHADOW_SPREAD_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_IShadow_Blur),
						 LG_SHADOW_BLUR_MIN, LG_SHADOW_BLUR_MAX,
						 LG_SHADOW_BLUR_MIN, 50.0,
						 LG_ISHADOW_BLUR_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 ISHADOW_BLUR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(STR(StrID_IShadow_Color),
				 20, 22, 28,
				 ISHADOW_COLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(ISHADOW_END_DISK_ID);

	/* --- Outer Shadow --- */
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC(STR(StrID_OuterShadow_Group), OSHADOW_START_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_OShadow_Spread),
						 LG_SHADOW_SPREAD_MIN, LG_SHADOW_SPREAD_MAX,
						 LG_SHADOW_SPREAD_MIN, LG_SHADOW_SPREAD_MAX,
						 LG_OSHADOW_SPREAD_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 OSHADOW_SPREAD_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX(STR(StrID_OShadow_Blur),
						 LG_SHADOW_BLUR_MIN, LG_SHADOW_BLUR_MAX,
						 LG_SHADOW_BLUR_MIN, LG_SHADOW_BLUR_MAX,
						 LG_OSHADOW_BLUR_DFLT,
						 PF_Precision_INTEGER,
						 0, 0,
						 OSHADOW_BLUR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(STR(StrID_OShadow_Color),
				 20, 22, 28,
				 OSHADOW_COLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(OSHADOW_END_DISK_ID);

	out_data->num_params = LG_NUM_PARAMS;

	return err;
}


/* ================================================================
 *  Pixel access helpers
 * ================================================================ */

static inline PF_Pixel8 *LG_GetPixel8(PF_EffectWorld *world, A_long x, A_long y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= world->width) x = world->width - 1;
	if (y >= world->height) y = world->height - 1;
	return (PF_Pixel8 *)((char *)world->data + y * world->rowbytes) + x;
}

static inline PF_Pixel16 *LG_GetPixel16(PF_EffectWorld *world, A_long x, A_long y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= world->width) x = world->width - 1;
	if (y >= world->height) y = world->height - 1;
	return (PF_Pixel16 *)((char *)world->data + y * world->rowbytes) + x;
}


/* ================================================================
 *  Bilinear sampling (8bpc and 16bpc)
 * ================================================================ */

static PF_Pixel8
LG_SampleBilinear8(PF_EffectWorld *world, PF_FpLong fx, PF_FpLong fy)
{
	A_long x0 = (A_long)floor(fx);
	A_long y0 = (A_long)floor(fy);
	PF_FpLong dx = fx - x0;
	PF_FpLong dy = fy - y0;

	PF_Pixel8 *p00 = LG_GetPixel8(world, x0,     y0);
	PF_Pixel8 *p10 = LG_GetPixel8(world, x0 + 1, y0);
	PF_Pixel8 *p01 = LG_GetPixel8(world, x0,     y0 + 1);
	PF_Pixel8 *p11 = LG_GetPixel8(world, x0 + 1, y0 + 1);

	PF_FpLong w00 = (1.0 - dx) * (1.0 - dy);
	PF_FpLong w10 = dx * (1.0 - dy);
	PF_FpLong w01 = (1.0 - dx) * dy;
	PF_FpLong w11 = dx * dy;

	PF_Pixel8 result;
	result.alpha = LG_ClampByte(p00->alpha * w00 + p10->alpha * w10 + p01->alpha * w01 + p11->alpha * w11);
	result.red   = LG_ClampByte(p00->red   * w00 + p10->red   * w10 + p01->red   * w01 + p11->red   * w11);
	result.green = LG_ClampByte(p00->green * w00 + p10->green * w10 + p01->green * w01 + p11->green * w11);
	result.blue  = LG_ClampByte(p00->blue  * w00 + p10->blue  * w10 + p01->blue  * w01 + p11->blue  * w11);
	return result;
}

static PF_Pixel16
LG_SampleBilinear16(PF_EffectWorld *world, PF_FpLong fx, PF_FpLong fy)
{
	A_long x0 = (A_long)floor(fx);
	A_long y0 = (A_long)floor(fy);
	PF_FpLong dx = fx - x0;
	PF_FpLong dy = fy - y0;

	PF_Pixel16 *p00 = LG_GetPixel16(world, x0,     y0);
	PF_Pixel16 *p10 = LG_GetPixel16(world, x0 + 1, y0);
	PF_Pixel16 *p01 = LG_GetPixel16(world, x0,     y0 + 1);
	PF_Pixel16 *p11 = LG_GetPixel16(world, x0 + 1, y0 + 1);

	PF_FpLong w00 = (1.0 - dx) * (1.0 - dy);
	PF_FpLong w10 = dx * (1.0 - dy);
	PF_FpLong w01 = (1.0 - dx) * dy;
	PF_FpLong w11 = dx * dy;

	PF_Pixel16 result;
	result.alpha = LG_Clamp16(p00->alpha * w00 + p10->alpha * w10 + p01->alpha * w01 + p11->alpha * w11);
	result.red   = LG_Clamp16(p00->red   * w00 + p10->red   * w10 + p01->red   * w01 + p11->red   * w11);
	result.green = LG_Clamp16(p00->green * w00 + p10->green * w10 + p01->green * w01 + p11->green * w11);
	result.blue  = LG_Clamp16(p00->blue  * w00 + p10->blue  * w10 + p01->blue  * w01 + p11->blue  * w11);
	return result;
}


/* ================================================================
 *  Noise functions for displacement
 * ================================================================ */

static inline PF_FpLong
LG_Hash(A_long x, A_long y)
{
	A_long h = x * 374761393 + y * 668265263;
	h = (h ^ (h >> 13)) * 1274126177;
	return (PF_FpLong)(h & 0xFFFF) / 65536.0;
}

static PF_FpLong
LG_SmoothNoise(PF_FpLong fx, PF_FpLong fy)
{
	A_long ix = (A_long)floor(fx);
	A_long iy = (A_long)floor(fy);
	PF_FpLong dx = fx - ix;
	PF_FpLong dy = fy - iy;
	dx = dx * dx * (3.0 - 2.0 * dx);
	dy = dy * dy * (3.0 - 2.0 * dy);

	PF_FpLong n00 = LG_Hash(ix, iy);
	PF_FpLong n10 = LG_Hash(ix + 1, iy);
	PF_FpLong n01 = LG_Hash(ix, iy + 1);
	PF_FpLong n11 = LG_Hash(ix + 1, iy + 1);

	PF_FpLong nx0 = n00 + (n10 - n00) * dx;
	PF_FpLong nx1 = n01 + (n11 - n01) * dx;
	return nx0 + (nx1 - nx0) * dy;
}

/* Fractal noise matching CSS feTurbulence: numOctaves=2 */
static PF_FpLong
LG_FractalNoise(PF_FpLong fx, PF_FpLong fy)
{
	return LG_SmoothNoise(fx, fy) * 0.667
		 + LG_SmoothNoise(fx * 2.0, fy * 2.0) * 0.333;
}


/* ================================================================
 *  Gaussian blur using PF_CONVOLVE + PF_GAUSSIAN_KERNEL macros
 *  Works for ANY bit depth (8/16/32bpc) since AE handles it.
 *  Separable: 1D horizontal then 1D vertical.
 * ================================================================ */

static PF_Err
LG_ApplyGaussianBlur(
	PF_InData		*in_data,
	PF_EffectWorld	*src,
	PF_EffectWorld	*dst,
	PF_FpLong		radius)
{
	PF_Err err = PF_Err_NONE;

	if (radius < 0.5) {
		ERR(PF_COPY(src, dst, NULL, NULL));
		return err;
	}

	A_long kRadius = (A_long)(radius + 0.5);
	if (kRadius < 1) kRadius = 1;
	if (kRadius > 50) kRadius = 50;

	A_long diameter = 0;
	A_FpLong kernel[128];
	memset(kernel, 0, sizeof(kernel));

	PF_KernelFlags kFlags = PF_KernelFlag_1D
						   | PF_KernelFlag_NORMALIZED
						   | PF_KernelFlag_CLAMP
						   | PF_KernelFlag_USE_LONG;

	ERR(PF_GAUSSIAN_KERNEL(
		(A_FpLong)kRadius,
		kFlags,
		1.0,
		&diameter,
		kernel));

	if (!err && diameter > 0) {
		/* Need a temp world for 2-pass separable blur */
		PF_EffectWorld tempW;
		memset(&tempW, 0, sizeof(tempW));

		PF_NewWorldFlags wFlags = PF_NewWorldFlag_CLEAR_PIXELS;
		if (PF_WORLD_IS_DEEP(src)) {
			wFlags = (PF_NewWorldFlags)(wFlags | PF_NewWorldFlag_DEEP_PIXELS);
		}
		ERR(PF_NEW_WORLD(src->width, src->height, wFlags, &tempW));

		if (!err) {
			PF_KernelFlags convFlagsH = PF_KernelFlag_1D
									  | PF_KernelFlag_NORMALIZED
									  | PF_KernelFlag_CLAMP
									  | PF_KernelFlag_USE_LONG
									  | PF_KernelFlag_HORIZONTAL
									  | PF_KernelFlag_REPLICATE_BORDERS;

			PF_KernelFlags convFlagsV = PF_KernelFlag_1D
									  | PF_KernelFlag_NORMALIZED
									  | PF_KernelFlag_CLAMP
									  | PF_KernelFlag_USE_LONG
									  | PF_KernelFlag_VERTICAL
									  | PF_KernelFlag_REPLICATE_BORDERS;

			ERR(PF_CONVOLVE(src, NULL, convFlagsH, diameter, kernel, kernel, kernel, kernel, &tempW));

			if (!err) {
				ERR(PF_CONVOLVE(&tempW, NULL, convFlagsV, diameter, kernel, kernel, kernel, kernel, dst));
			}
		}

		if (tempW.data) {
			PF_DISPOSE_WORLD(&tempW);
		}
	}

	return err;
}


/* ================================================================
 *  Chamfer distance transform (2-pass, fast)
 *  mode 0 = inner: distance from opaque pixel to nearest transparent
 *  mode 1 = outer: distance from transparent pixel to nearest opaque
 * ================================================================ */

static void
LG_ChamferDist(
	PF_EffectWorld	*src,
	PF_FpLong		*distMap,
	A_long			width,
	A_long			height,
	PF_FpLong		maxDist,
	PF_Boolean		deep,
	int				mode,			/* 0=inner, 1=outer */
	A_u_char		*processedAlpha = NULL)	/* optional metaball alpha */
{
	/* 5x5 Borgefors weights for near-Euclidean distance */
	const PF_FpLong Da = 1.0;		/* orthogonal */
	const PF_FpLong Db = 1.4142;	/* diagonal */
	const PF_FpLong Dc = 2.2361;	/* knight move (sqrt5) */
	const PF_FpLong INF = maxDist + 1.0;

	/* Initialize seeds */
	for (A_long y = 0; y < height; y++) {
		for (A_long x = 0; x < width; x++) {
			A_long idx = y * width + x;
			PF_Boolean opaque;
			if (processedAlpha) {
				opaque = (processedAlpha[idx] >= 128);
			} else {
				PF_FpLong alpha01;
				if (deep) {
					PF_Pixel16 *p = LG_GetPixel16(src, x, y);
					alpha01 = p->alpha / (PF_FpLong)PF_MAX_CHAN16;
				} else {
					PF_Pixel8 *p = LG_GetPixel8(src, x, y);
					alpha01 = p->alpha / 255.0;
				}
				opaque = (alpha01 >= 0.5);
			}

			if (mode == 0) {
				if (!opaque) {
					distMap[idx] = 0.0;
				} else if (x == 0 || y == 0 || x == width-1 || y == height-1) {
					distMap[idx] = Da;
				} else {
					distMap[idx] = INF;
				}
			} else {
				distMap[idx] = opaque ? 0.0 : INF;
			}
		}
	}

	/* Forward pass (top-left to bottom-right) */
	for (A_long y = 0; y < height; y++) {
		for (A_long x = 0; x < width; x++) {
			A_long idx = y * width + x;
			PF_FpLong d = distMap[idx];
			if (d == 0.0) continue;

			/* Orthogonal */
			if (y > 0)   { PF_FpLong v = distMap[(y-1)*width+x] + Da;     if (v < d) d = v; }
			if (x > 0)   { PF_FpLong v = distMap[y*width+(x-1)] + Da;     if (v < d) d = v; }
			/* Diagonal */
			if (y > 0 && x > 0)       { PF_FpLong v = distMap[(y-1)*width+(x-1)] + Db; if (v < d) d = v; }
			if (y > 0 && x < width-1) { PF_FpLong v = distMap[(y-1)*width+(x+1)] + Db; if (v < d) d = v; }
			/* Knight moves */
			if (y >= 2 && x > 0)       { PF_FpLong v = distMap[(y-2)*width+(x-1)] + Dc; if (v < d) d = v; }
			if (y >= 2 && x < width-1) { PF_FpLong v = distMap[(y-2)*width+(x+1)] + Dc; if (v < d) d = v; }
			if (y > 0 && x >= 2)       { PF_FpLong v = distMap[(y-1)*width+(x-2)] + Dc; if (v < d) d = v; }
			if (y > 0 && x <= width-3) { PF_FpLong v = distMap[(y-1)*width+(x+2)] + Dc; if (v < d) d = v; }

			distMap[idx] = d;
		}
	}

	/* Backward pass (bottom-right to top-left) */
	for (A_long y = height - 1; y >= 0; y--) {
		for (A_long x = width - 1; x >= 0; x--) {
			A_long idx = y * width + x;
			PF_FpLong d = distMap[idx];

			/* Orthogonal */
			if (y < height-1) { PF_FpLong v = distMap[(y+1)*width+x] + Da;     if (v < d) d = v; }
			if (x < width-1)  { PF_FpLong v = distMap[y*width+(x+1)] + Da;     if (v < d) d = v; }
			/* Diagonal */
			if (y < height-1 && x < width-1) { PF_FpLong v = distMap[(y+1)*width+(x+1)] + Db; if (v < d) d = v; }
			if (y < height-1 && x > 0)       { PF_FpLong v = distMap[(y+1)*width+(x-1)] + Db; if (v < d) d = v; }
			/* Knight moves */
			if (y <= height-3 && x < width-1) { PF_FpLong v = distMap[(y+2)*width+(x+1)] + Dc; if (v < d) d = v; }
			if (y <= height-3 && x > 0)       { PF_FpLong v = distMap[(y+2)*width+(x-1)] + Dc; if (v < d) d = v; }
			if (y < height-1 && x <= width-3) { PF_FpLong v = distMap[(y+1)*width+(x+2)] + Dc; if (v < d) d = v; }
			if (y < height-1 && x >= 2)       { PF_FpLong v = distMap[(y+1)*width+(x-2)] + Dc; if (v < d) d = v; }

			if (d > maxDist) d = maxDist;
			distMap[idx] = d;
		}
	}
}


/* ================================================================
 *  Smooth distance field (3x3 Gaussian, reduces banding)
 * ================================================================ */

static void
LG_SmoothDistField(PF_FpLong *dist, A_long width, A_long height, int passes)
{
	PF_FpLong *tmp = (PF_FpLong *)malloc(width * height * sizeof(PF_FpLong));
	if (!tmp) return;

	for (int p = 0; p < passes; p++) {
		/* 3x3 Gaussian: [1 2 1; 2 4 2; 1 2 1] / 16 */
		for (A_long y = 1; y < height - 1; y++) {
			for (A_long x = 1; x < width - 1; x++) {
				A_long i = y * width + x;
				tmp[i] = (
					dist[i - width - 1] + 2.0*dist[i - width] + dist[i - width + 1] +
					2.0*dist[i - 1]     + 4.0*dist[i]         + 2.0*dist[i + 1] +
					dist[i + width - 1] + 2.0*dist[i + width] + dist[i + width + 1]
				) / 16.0;
			}
		}
		/* Copy edges unchanged */
		for (A_long x = 0; x < width; x++) {
			tmp[x] = dist[x];
			tmp[(height-1)*width + x] = dist[(height-1)*width + x];
		}
		for (A_long y = 0; y < height; y++) {
			tmp[y*width] = dist[y*width];
			tmp[y*width + width - 1] = dist[y*width + width - 1];
		}
		memcpy(dist, tmp, width * height * sizeof(PF_FpLong));
	}
	free(tmp);
}


/* ================================================================
 *  Shadow intensity helper
 * ================================================================ */

static inline PF_FpLong
LG_ShadowIntensity(PF_FpLong dist, PF_FpLong spread, PF_FpLong blur)
{
	if (blur < 0.1) return 0.0;
	PF_FpLong offset = -spread;
	PF_FpLong adjusted = dist - offset;
	if (adjusted >= blur) return 0.0;
	if (adjusted <= 0.0) return 1.0;
	PF_FpLong t = 1.0 - (adjusted / blur);
	return t * t;
}


/* ================================================================
 *  Chromatic aberration band helper
 *  Concentrates CA in a thin band at the glass edge (like iOS glass).
 *  chromDecay controls the band width as fraction of bevel.
 * ================================================================ */

static inline PF_FpLong
LG_ChromaticBand(PF_FpLong tLens, PF_FpLong effChrom, PF_FpLong chromDecay)
{
	if (effChrom < 0.1 || tLens < 0.001) return 0.0;
	PF_FpLong bandFrac = LG_Clamp(chromDecay * 0.05, 0.01, 1.0);
	PF_FpLong threshold = 1.0 - bandFrac;
	PF_FpLong t = LG_Clamp((tLens - threshold) / bandFrac, 0.0, 1.0);
	t = t * t * (3.0 - 2.0 * t);   /* smoothstep */
	return effChrom * t;
}


/* ================================================================
 *  Analytical SDF for rounded rectangle
 *  Returns signed distance: negative inside, positive outside.
 *  halfW/halfH = half-dimensions of the rectangle.
 *  R = corner radius (clamped to min(halfW, halfH)).
 * ================================================================ */

static inline PF_FpLong
LG_RoundedRectSDF(PF_FpLong px, PF_FpLong py,
				  PF_FpLong cx, PF_FpLong cy,
				  PF_FpLong halfW, PF_FpLong halfH,
				  PF_FpLong R,
				  PF_Boolean clipL, PF_Boolean clipR,
				  PF_Boolean clipT, PF_Boolean clipB)
{
	/* Per-edge signed distance (positive = outside that edge) */
	PF_FpLong dL = -(px - (cx - halfW)) + R;	/* distance from left edge inward */
	PF_FpLong dR =  (px - (cx + halfW)) + R;	/* distance from right edge inward */
	PF_FpLong dT = -(py - (cy - halfH)) + R;	/* top */
	PF_FpLong dB =  (py - (cy + halfH)) + R;	/* bottom */

	/* Suppress clipped edges: force deep-inside */
	if (clipL) dL = -1e6;
	if (clipR) dR = -1e6;
	if (clipT) dT = -1e6;
	if (clipB) dB = -1e6;

	PF_FpLong dx = (dL > dR) ? dL : dR;
	PF_FpLong dy = (dT > dB) ? dT : dB;

	PF_FpLong outsideDist = sqrt(LG_Clamp(dx, 0.0, 1e9) * LG_Clamp(dx, 0.0, 1e9)
							   + LG_Clamp(dy, 0.0, 1e9) * LG_Clamp(dy, 0.0, 1e9));
	PF_FpLong insideDist  = LG_Clamp((dx > dy) ? dx : dy, -1e9, 0.0);
	return outsideDist + insideDist - R;
}

/* Analytical normal (gradient) of the rounded rectangle SDF.
 * Points outward from the shape surface.
 * Clipped edges produce zero normal on that axis. */
static inline void
LG_RoundedRectNormal(PF_FpLong px, PF_FpLong py,
					 PF_FpLong cx, PF_FpLong cy,
					 PF_FpLong halfW, PF_FpLong halfH,
					 PF_FpLong R,
					 PF_Boolean clipL, PF_Boolean clipR,
					 PF_Boolean clipT, PF_Boolean clipB,
					 PF_FpLong *outNx, PF_FpLong *outNy)
{
	PF_FpLong dL = -(px - (cx - halfW)) + R;
	PF_FpLong dR =  (px - (cx + halfW)) + R;
	PF_FpLong dT = -(py - (cy - halfH)) + R;
	PF_FpLong dB =  (py - (cy + halfH)) + R;

	if (clipL) dL = -1e6;
	if (clipR) dR = -1e6;
	if (clipT) dT = -1e6;
	if (clipB) dB = -1e6;

	/* Determine which edge(s) are closest */
	PF_FpLong dx = (dL > dR) ? dL : dR;
	PF_FpLong dy = (dT > dB) ? dT : dB;
	PF_FpLong sx = (dL > dR) ? -1.0 : 1.0;	/* points outward */
	PF_FpLong sy = (dT > dB) ? -1.0 : 1.0;

	if (dx > 0.0 && dy > 0.0) {
		/* Corner region — gradient points radially from corner center */
		PF_FpLong len = sqrt(dx * dx + dy * dy);
		if (len > 0.0001) {
			*outNx = sx * dx / len;
			*outNy = sy * dy / len;
		} else {
			*outNx = 0.0; *outNy = 0.0;
		}
	} else if (dx > dy) {
		*outNx = sx;
		*outNy = 0.0;
	} else {
		*outNx = 0.0;
		*outNy = sy;
	}
}


/* ================================================================
 *  Render context (shared state for multi-threaded scanline processing)
 * ================================================================ */

struct LG_RenderCtx {
	PF_FpLong effBevel, bevelFalloff, effRefStr, effChrom, effNoise;
	PF_FpLong noiseFreq, satFactor, blurRadius, chromDecay;
	PF_FpLong ldx, ldy, specInt;
	PF_Pixel  tintCol;
	PF_FpLong tintOpacity;
	PF_FpLong iSpread, iBlur, oSpread, oBlur;
	PF_Pixel  iColor, oColor;

	PF_FpLong cornerRadius;
	PF_FpLong surfTension, tensionThresh;
	PF_Boolean clipCompBounds;  /* clip edges when glass goes outside comp */

	/* Analytical SDF mode (rounded rectangle) */
	PF_Boolean useAnalyticalSDF;
	PF_FpLong sdfCx, sdfCy;		/* rectangle center */
	PF_FpLong sdfHalfW, sdfHalfH;	/* half-dimensions */
	PF_FpLong sdfRadius;			/* corner radius */

	/* Comp boundary clip: TRUE = this edge is clipped by comp */
	PF_Boolean clipLeft, clipRight, clipTop, clipBottom;

	/* Full layer geometry for SDF */
	A_long layerW, layerH;        /* full layer dimensions */
	A_long bufOriginX, bufOriginY; /* buffer origin in layer coords */

	PF_EffectWorld *blurredWorld;
	PF_FpLong *innerDist;
	PF_FpLong *outerDist;
	A_u_char *processedAlpha;		/* metaball-merged alpha (NULL if no tension) */

	PF_EffectWorld *src;
	PF_EffectWorld *alphaSource;
	PF_EffectWorld *output;
	A_long width, height;
	PF_Boolean deep;
};


/* ================================================================
 *  Per-scanline callback (called by iterate_generic, multi-threaded)
 * ================================================================ */

static PF_Err
LG_ProcessScanline(void *refconPV, A_long /*thread_indexL*/, A_long y, A_long /*iterationsL*/)
{
	LG_RenderCtx *ctx = (LG_RenderCtx *)refconPV;
	A_long width  = ctx->width;
	A_long height = ctx->height;

	/* Check if this row is outside comp bounds (for clipCompBounds) */
	if (ctx->clipCompBounds) {
		/* Expanded margin (20px) to ensure all glass effects including shadows are clipped */
		const A_long MARGIN = 20;
		
		/* When clipTop is TRUE, rows near top boundary are outside comp */
		if (ctx->clipTop && y < MARGIN) {
			for (A_long x = 0; x < width; x++) {
				if (ctx->deep) {
					PF_Pixel16 *outP = LG_GetPixel16(ctx->output, x, y);
					PF_Pixel16 *srcP = LG_GetPixel16(ctx->src, x, y);
					*outP = *srcP;
				} else {
					PF_Pixel8 *outP = LG_GetPixel8(ctx->output, x, y);
					PF_Pixel8 *srcP = LG_GetPixel8(ctx->src, x, y);
					*outP = *srcP;
				}
			}
			return PF_Err_NONE;
		}
		/* When clipBottom is TRUE, rows near bottom boundary are outside comp */
		if (ctx->clipBottom && y >= height - MARGIN) {
			for (A_long x = 0; x < width; x++) {
				if (ctx->deep) {
					PF_Pixel16 *outP = LG_GetPixel16(ctx->output, x, y);
					PF_Pixel16 *srcP = LG_GetPixel16(ctx->src, x, y);
					*outP = *srcP;
				} else {
					PF_Pixel8 *outP = LG_GetPixel8(ctx->output, x, y);
					PF_Pixel8 *srcP = LG_GetPixel8(ctx->src, x, y);
					*outP = *srcP;
				}
			}
			return PF_Err_NONE;
		}
	}

	for (A_long x = 0; x < width; x++) {
		A_long idx = y * width + x;

		/* Check if pixel is outside comp bounds (horizontal) */
		if (ctx->clipCompBounds) {
			/* Expanded margin to match vertical (20px) */
			if ((ctx->clipLeft && x < 20) || (ctx->clipRight && x >= width - 20)) {
				/* Skip glass effects - copy input pixel */
				if (ctx->deep) {
					PF_Pixel16 *outP = LG_GetPixel16(ctx->output, x, y);
					PF_Pixel16 *srcP = LG_GetPixel16(ctx->src, x, y);
					*outP = *srcP;
				} else {
					PF_Pixel8 *outP = LG_GetPixel8(ctx->output, x, y);
					PF_Pixel8 *srcP = LG_GetPixel8(ctx->src, x, y);
					*outP = *srcP;
				}
				continue;
			}
		}

		/* --- Compute distance, normal, and inside/outside per pixel --- */
		PF_FpLong iDist, oDist, nx, ny;
		PF_Boolean isOpaque;

		if (ctx->useAnalyticalSDF) {
			/* Current Layer Alpha mode: read input alpha as glass shape */
			if (ctx->alphaSource == ctx->src) {
				/* Read alpha from input layer (mask is already applied) */
				A_u_char alpha;
				if (ctx->deep) {
					PF_Pixel16 *p = LG_GetPixel16(ctx->src, x, y);
					alpha = (A_u_char)(p->alpha * 255 / PF_MAX_CHAN16);
				} else {
					PF_Pixel8 *p = LG_GetPixel8(ctx->src, x, y);
					alpha = p->alpha;
				}
				isOpaque = (alpha > 128);  /* alpha > 0.5 */
				iDist = isOpaque ? 1.0 : 0.0;  /* simplified distance */
				oDist = isOpaque ? 0.0 : 1.0;
				nx = 0.0; ny = 0.0;  /* neutral normal */
			} else {
				/* Shape Layer mode: use rounded rect SDF */
				PF_FpLong px = (PF_FpLong)x + ctx->bufOriginX;
				PF_FpLong py = (PF_FpLong)y + ctx->bufOriginY;
				PF_FpLong sdf = LG_RoundedRectSDF(px, py,
					ctx->sdfCx, ctx->sdfCy, ctx->sdfHalfW, ctx->sdfHalfH, ctx->sdfRadius,
					ctx->clipLeft, ctx->clipRight, ctx->clipTop, ctx->clipBottom);
				isOpaque = (sdf <= 0.5);
				iDist = (sdf <= 0.0) ? -sdf : 0.0;   /* positive inside */
				oDist = (sdf > 0.0) ? sdf : 0.0;      /* positive outside */
				LG_RoundedRectNormal(px, py,
					ctx->sdfCx, ctx->sdfCy, ctx->sdfHalfW, ctx->sdfHalfH, ctx->sdfRadius,
					ctx->clipLeft, ctx->clipRight, ctx->clipTop, ctx->clipBottom,
					&nx, &ny);
			}
			/* SDF normal points outward, but inner path expects
			 * inward normal (matching Sobel on inner distance).
			 * Negate for inside pixels so refraction pushes sampling
			 * toward center, not toward edge. */
			if (isOpaque) { nx = -nx; ny = -ny; }
		} else {
			iDist = ctx->innerDist[idx];
			oDist = ctx->outerDist[idx];
			nx = 0.0; ny = 0.0;	/* computed later per-path */
			if (ctx->processedAlpha) {
				isOpaque = (ctx->processedAlpha[idx] >= 128);
			} else if (ctx->deep) {
				PF_Pixel16 *shapeP = LG_GetPixel16(ctx->alphaSource, x, y);
				isOpaque = (shapeP->alpha >= 128);
			} else {
				PF_Pixel8 *shapeP = LG_GetPixel8(ctx->alphaSource, x, y);
				isOpaque = (shapeP->alpha >= 128);
			}
		}

		if (ctx->deep) {
			/* ========== 16 bpc ========== */
			PF_Pixel16 *srcP = LG_GetPixel16(ctx->src, x, y);
			PF_Pixel16 *outP = LG_GetPixel16(ctx->output, x, y);

			if (isOpaque) {
				/* Normal: use analytical if available, else Sobel */
				if (!ctx->useAnalyticalSDF) {
					PF_FpLong gx = 0.0, gy = 0.0;
					if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
						PF_FpLong *d = ctx->innerDist;
						gx = (d[idx-width+1] - d[idx-width-1])
						   + 2.0*(d[idx+1] - d[idx-1])
						   + (d[idx+width+1] - d[idx+width-1]);
						gy = (d[idx+width-1] - d[idx-width-1])
						   + 2.0*(d[idx+width] - d[idx-width])
						   + (d[idx+width+1] - d[idx-width+1]);
					}
					PF_FpLong glen = sqrt(gx * gx + gy * gy);
					if (glen > 0.001) { nx = gx / glen; ny = gy / glen; }
				}

				/* Refraction: effBevel controls range from edge, bevelFalloff controls curve.
				 * tLens = 0 beyond effBevel (center = original), 1 at edge. */
				PF_FpLong effRange = ctx->effBevel;
				PF_FpLong tRaw = (effRange > 0.5) ? LG_Clamp(1.0 - iDist / effRange, 0.0, 1.0) : 0.0;
				PF_FpLong falloffPow = LG_Clamp(ctx->bevelFalloff * 2.0, 0.2, 10.0);
				PF_FpLong tLens = pow(tRaw, falloffPow);

				/* Displacement: INWARD (positive gradient = toward center)
				 * — convex lens effect: edge shows magnified center content. */
				PF_FpLong dX = 0.0, dY = 0.0;
				if (ctx->effRefStr > 0.1 && tLens > 0.001) {
					PF_FpLong mag = tLens * ctx->effRefStr;
					dX = nx * mag;
					dY = ny * mag;
				}
				if (ctx->effNoise > 0.1) {
					PF_FpLong nX = LG_FractalNoise(x * ctx->noiseFreq, y * ctx->noiseFreq);
					PF_FpLong nY = LG_FractalNoise(x * ctx->noiseFreq + 31.7, y * ctx->noiseFreq + 47.3);
					dX += (nX - 0.5) * ctx->effNoise;
					dY += (nY - 0.5) * ctx->effNoise;
				}

				/* CA — concentrated in thin band at glass edge */
				PF_FpLong localChrom = LG_ChromaticBand(tLens, ctx->effChrom, ctx->chromDecay);

				PF_FpLong rR, rG, rB;
				if (localChrom > 0.1 && (nx != 0.0 || ny != 0.0)) {
					PF_Pixel16 pR = LG_SampleBilinear16(ctx->blurredWorld, x + dX - nx * localChrom, y + dY - ny * localChrom);
					PF_Pixel16 pG = LG_SampleBilinear16(ctx->blurredWorld, x + dX, y + dY);
					PF_Pixel16 pB = LG_SampleBilinear16(ctx->blurredWorld, x + dX + nx * localChrom, y + dY + ny * localChrom);
					rR = pR.red; rG = pG.green; rB = pB.blue;
				} else {
					PF_Pixel16 pG = LG_SampleBilinear16(ctx->blurredWorld, x + dX, y + dY);
					rR = pG.red; rG = pG.green; rB = pG.blue;
				}

				if (ctx->satFactor > 0.01 && ctx->satFactor != 1.0) {
					PF_FpLong lum = 0.2126 * rR + 0.7152 * rG + 0.0722 * rB;
					rR = lum + ctx->satFactor * (rR - lum);
					rG = lum + ctx->satFactor * (rG - lum);
					rB = lum + ctx->satFactor * (rB - lum);
				}

				if (ctx->specInt > 0.1 && tLens > 0.001) {
					PF_FpLong specNorm = ctx->specInt / 100.0;

					/* 1. Fresnel reflection of background (Schlick, power-5) */
					PF_FpLong R0 = 0.04;
					PF_FpLong t2 = tLens * tLens, t4 = t2 * t2;
					PF_FpLong fresnel = R0 + (1.0 - R0) * t4 * tLens;

					PF_FpLong reflDist = tLens * ctx->effRefStr * 0.5;
					PF_Pixel16 reflP = LG_SampleBilinear16(ctx->blurredWorld,
						x - nx * reflDist, y - ny * reflDist);
					PF_FpLong reflAmt = fresnel * specNorm;
					rR = rR * (1.0 - reflAmt) + reflP.red   * reflAmt;
					rG = rG * (1.0 - reflAmt) + reflP.green * reflAmt;
					rB = rB * (1.0 - reflAmt) + reflP.blue  * reflAmt;

					/* 2. Thin directional specular (light source glint) */
					PF_FpLong specDot = LG_Clamp(nx * ctx->ldx + ny * ctx->ldy, 0.0, 1.0);
					PF_FpLong spec = pow(specDot, 8.0) * tLens * specNorm;
					rR = rR + spec * (PF_MAX_CHAN16 - rR);
					rG = rG + spec * (PF_MAX_CHAN16 - rG);
					rB = rB + spec * (PF_MAX_CHAN16 - rB);
				}

				if (ctx->tintOpacity > 0.1) {
					PF_FpLong t = ctx->tintOpacity / 100.0;
					PF_FpLong tR = ctx->tintCol.red   * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong tG = ctx->tintCol.green * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong tB = ctx->tintCol.blue  * (PF_MAX_CHAN16 / 255.0);
					rR = rR * (1.0 - t) + tR * t;
					rG = rG * (1.0 - t) + tG * t;
					rB = rB * (1.0 - t) + tB * t;
				}

				PF_FpLong is = LG_ShadowIntensity(iDist, ctx->iSpread, ctx->iBlur);
				if (is > 0.001) {
					PF_FpLong isR = ctx->iColor.red   * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong isG = ctx->iColor.green * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong isB = ctx->iColor.blue  * (PF_MAX_CHAN16 / 255.0);
					rR = rR * (1.0 - is) + isR * is;
					rG = rG * (1.0 - is) + isG * is;
					rB = rB * (1.0 - is) + isB * is;
				}

				outP->alpha = srcP->alpha;
				outP->red   = LG_Clamp16(rR);
				outP->green = LG_Clamp16(rG);
				outP->blue  = LG_Clamp16(rB);
			} else {
				/* Outside glass shape — outer refraction + CA */
				PF_FpLong rR = srcP->red, rG = srcP->green, rB = srcP->blue;

				PF_FpLong outerRange = ctx->effBevel;
				if (outerRange > 0.5 && oDist < outerRange) {
					PF_FpLong onx = nx, ony = ny;  /* already from SDF or will be from Sobel */
					PF_Boolean haveOuterNormal = ctx->useAnalyticalSDF;

					if (!ctx->useAnalyticalSDF) {
						PF_FpLong ogx = 0.0, ogy = 0.0;
						if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
							PF_FpLong *d = ctx->outerDist;
							ogx = (d[idx-width+1] - d[idx-width-1])
							    + 2.0*(d[idx+1] - d[idx-1])
							    + (d[idx+width+1] - d[idx+width-1]);
							ogy = (d[idx+width-1] - d[idx-width-1])
							    + 2.0*(d[idx+width] - d[idx-width])
							    + (d[idx+width+1] - d[idx-width+1]);
						}
						PF_FpLong oglen = sqrt(ogx * ogx + ogy * ogy);
						if (oglen > 0.001) {
							onx = ogx / oglen; ony = ogy / oglen;
							haveOuterNormal = TRUE;
						}
					}

					if (haveOuterNormal) {
						/* Outer tLens: 1 at edge, 0 at outerRange */
						PF_FpLong otRaw = LG_Clamp(1.0 - oDist / outerRange, 0.0, 1.0);
						PF_FpLong oFalloff = LG_Clamp(ctx->bevelFalloff * 2.0, 0.2, 10.0);
						PF_FpLong otLens = pow(otRaw, oFalloff);

						/* Outer refraction: displace INWARD (toward shape) */
						if (ctx->effRefStr > 0.1 && otLens > 0.001) {
							PF_FpLong oMag = otLens * ctx->effRefStr;
							PF_FpLong odX = -onx * oMag;
							PF_FpLong odY = -ony * oMag;

							PF_FpLong oCA = LG_ChromaticBand(otLens, ctx->effChrom, ctx->chromDecay);

							if (oCA > 0.1) {
								PF_Pixel16 pR = LG_SampleBilinear16(ctx->blurredWorld,
									x + odX + onx * oCA, y + odY + ony * oCA);
								PF_Pixel16 pG = LG_SampleBilinear16(ctx->blurredWorld,
									x + odX, y + odY);
								PF_Pixel16 pB = LG_SampleBilinear16(ctx->blurredWorld,
									x + odX - onx * oCA, y + odY - ony * oCA);
								rR = pR.red; rG = pG.green; rB = pB.blue;
							} else {
								PF_Pixel16 pG = LG_SampleBilinear16(ctx->blurredWorld,
									x + odX, y + odY);
								rR = pG.red; rG = pG.green; rB = pG.blue;
							}
						}
					}
				}

				PF_FpLong os = LG_ShadowIntensity(oDist, ctx->oSpread, ctx->oBlur);
				if (os > 0.001) {
					PF_FpLong sR = ctx->oColor.red   * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong sG = ctx->oColor.green * (PF_MAX_CHAN16 / 255.0);
					PF_FpLong sB = ctx->oColor.blue  * (PF_MAX_CHAN16 / 255.0);
					rR = rR * (1.0 - os) + sR * os;
					rG = rG * (1.0 - os) + sG * os;
					rB = rB * (1.0 - os) + sB * os;
				}

				outP->alpha = srcP->alpha;
				A_u_short shadowAlpha = (A_u_short)(os * PF_MAX_CHAN16);
				if (shadowAlpha > outP->alpha) outP->alpha = shadowAlpha;
				outP->red   = LG_Clamp16(rR);
				outP->green = LG_Clamp16(rG);
				outP->blue  = LG_Clamp16(rB);
			}
		} else {
			/* ========== 8 bpc ========== */
			PF_Pixel8 *srcP = LG_GetPixel8(ctx->src, x, y);
			PF_Pixel8 *outP = LG_GetPixel8(ctx->output, x, y);

			if (isOpaque) {
				/* Normal: use analytical if available, else Sobel */
				if (!ctx->useAnalyticalSDF) {
					PF_FpLong gx = 0.0, gy = 0.0;
					if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
						PF_FpLong *d = ctx->innerDist;
						gx = (d[idx-width+1] - d[idx-width-1])
						   + 2.0*(d[idx+1] - d[idx-1])
						   + (d[idx+width+1] - d[idx+width-1]);
						gy = (d[idx+width-1] - d[idx-width-1])
						   + 2.0*(d[idx+width] - d[idx-width])
						   + (d[idx+width+1] - d[idx-width+1]);
					}
					PF_FpLong glen = sqrt(gx * gx + gy * gy);
					if (glen > 0.001) { nx = gx / glen; ny = gy / glen; }
				}

				/* Refraction: effBevel range + bevelFalloff power curve */
				PF_FpLong effRange = ctx->effBevel;
				PF_FpLong tRaw = (effRange > 0.5) ? LG_Clamp(1.0 - iDist / effRange, 0.0, 1.0) : 0.0;
				PF_FpLong falloffPow = LG_Clamp(ctx->bevelFalloff * 2.0, 0.2, 10.0);
				PF_FpLong tLens = pow(tRaw, falloffPow);

				/* Displacement: INWARD (toward center) — convex lens */
				PF_FpLong dX = 0.0, dY = 0.0;
				if (ctx->effRefStr > 0.1 && tLens > 0.001) {
					PF_FpLong mag = tLens * ctx->effRefStr;
					dX = nx * mag;
					dY = ny * mag;
				}
				if (ctx->effNoise > 0.1) {
					PF_FpLong nX = LG_FractalNoise(x * ctx->noiseFreq, y * ctx->noiseFreq);
					PF_FpLong nY = LG_FractalNoise(x * ctx->noiseFreq + 31.7, y * ctx->noiseFreq + 47.3);
					dX += (nX - 0.5) * ctx->effNoise;
					dY += (nY - 0.5) * ctx->effNoise;
				}

				/* CA — concentrated in thin band at glass edge */
				PF_FpLong localChrom = LG_ChromaticBand(tLens, ctx->effChrom, ctx->chromDecay);

				PF_FpLong rR, rG, rB;
				if (localChrom > 0.1 && (nx != 0.0 || ny != 0.0)) {
					PF_Pixel8 pR = LG_SampleBilinear8(ctx->blurredWorld, x + dX - nx * localChrom, y + dY - ny * localChrom);
					PF_Pixel8 pG = LG_SampleBilinear8(ctx->blurredWorld, x + dX, y + dY);
					PF_Pixel8 pB = LG_SampleBilinear8(ctx->blurredWorld, x + dX + nx * localChrom, y + dY + ny * localChrom);
					rR = pR.red; rG = pG.green; rB = pB.blue;
				} else {
					PF_Pixel8 pG = LG_SampleBilinear8(ctx->blurredWorld, x + dX, y + dY);
					rR = pG.red; rG = pG.green; rB = pG.blue;
				}

				if (ctx->satFactor > 0.01 && ctx->satFactor != 1.0) {
					PF_FpLong lum = 0.2126 * rR + 0.7152 * rG + 0.0722 * rB;
					rR = lum + ctx->satFactor * (rR - lum);
					rG = lum + ctx->satFactor * (rG - lum);
					rB = lum + ctx->satFactor * (rB - lum);
				}

				if (ctx->specInt > 0.1 && tLens > 0.001) {
					PF_FpLong specNorm = ctx->specInt / 100.0;

					/* 1. Fresnel reflection of background content */
					PF_FpLong R0 = 0.04;
					PF_FpLong t2 = tLens * tLens, t4 = t2 * t2;
					PF_FpLong fresnel = R0 + (1.0 - R0) * t4 * tLens;

					PF_FpLong reflDist = tLens * ctx->effRefStr * 0.5;
					PF_Pixel8 reflP = LG_SampleBilinear8(ctx->blurredWorld,
						x - nx * reflDist, y - ny * reflDist);
					PF_FpLong reflAmt = fresnel * specNorm;
					rR = rR * (1.0 - reflAmt) + reflP.red   * reflAmt;
					rG = rG * (1.0 - reflAmt) + reflP.green * reflAmt;
					rB = rB * (1.0 - reflAmt) + reflP.blue  * reflAmt;

					/* 2. Thin directional specular (light source glint) */
					PF_FpLong specDot = LG_Clamp(nx * ctx->ldx + ny * ctx->ldy, 0.0, 1.0);
					PF_FpLong spec = pow(specDot, 8.0) * tLens * specNorm;
					rR = rR + spec * (255.0 - rR);
					rG = rG + spec * (255.0 - rG);
					rB = rB + spec * (255.0 - rB);
				}

				if (ctx->tintOpacity > 0.1) {
					PF_FpLong t = ctx->tintOpacity / 100.0;
					rR = rR * (1.0 - t) + ctx->tintCol.red   * t;
					rG = rG * (1.0 - t) + ctx->tintCol.green * t;
					rB = rB * (1.0 - t) + ctx->tintCol.blue  * t;
				}

				PF_FpLong is = LG_ShadowIntensity(iDist, ctx->iSpread, ctx->iBlur);
				if (is > 0.001) {
					rR = rR * (1.0 - is) + ctx->iColor.red   * is;
					rG = rG * (1.0 - is) + ctx->iColor.green * is;
					rB = rB * (1.0 - is) + ctx->iColor.blue  * is;
				}

				outP->alpha = srcP->alpha;
				outP->red   = LG_ClampByte(rR);
				outP->green = LG_ClampByte(rG);
				outP->blue  = LG_ClampByte(rB);
			} else {
				/* Outside glass shape — outer refraction + CA */
				PF_FpLong rR = srcP->red, rG = srcP->green, rB = srcP->blue;

				PF_FpLong outerRange8 = ctx->effBevel;
				if (outerRange8 > 0.5 && oDist < outerRange8) {
					PF_FpLong onx = nx, ony = ny;
					PF_Boolean haveOuterNormal = ctx->useAnalyticalSDF;

					if (!ctx->useAnalyticalSDF) {
						PF_FpLong ogx = 0.0, ogy = 0.0;
						if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
							PF_FpLong *d = ctx->outerDist;
							ogx = (d[idx-width+1] - d[idx-width-1])
							    + 2.0*(d[idx+1] - d[idx-1])
							    + (d[idx+width+1] - d[idx+width-1]);
							ogy = (d[idx+width-1] - d[idx-width-1])
							    + 2.0*(d[idx+width] - d[idx-width])
							    + (d[idx+width+1] - d[idx-width+1]);
						}
						PF_FpLong oglen = sqrt(ogx * ogx + ogy * ogy);
						if (oglen > 0.001) {
							onx = ogx / oglen; ony = ogy / oglen;
							haveOuterNormal = TRUE;
						}
					}

					if (haveOuterNormal) {
						PF_FpLong otRaw = LG_Clamp(1.0 - oDist / outerRange8, 0.0, 1.0);
						PF_FpLong oFalloff = LG_Clamp(ctx->bevelFalloff * 2.0, 0.2, 10.0);
						PF_FpLong otLens = pow(otRaw, oFalloff);

						if (ctx->effRefStr > 0.1 && otLens > 0.001) {
							PF_FpLong oMag = otLens * ctx->effRefStr;
							PF_FpLong odX = -onx * oMag;
							PF_FpLong odY = -ony * oMag;

							PF_FpLong oCA = LG_ChromaticBand(otLens, ctx->effChrom, ctx->chromDecay);

							if (oCA > 0.1) {
								PF_Pixel8 pR = LG_SampleBilinear8(ctx->blurredWorld,
									x + odX + onx * oCA, y + odY + ony * oCA);
								PF_Pixel8 pG = LG_SampleBilinear8(ctx->blurredWorld,
									x + odX, y + odY);
								PF_Pixel8 pB = LG_SampleBilinear8(ctx->blurredWorld,
									x + odX - onx * oCA, y + odY - ony * oCA);
								rR = pR.red; rG = pG.green; rB = pB.blue;
							} else {
								PF_Pixel8 pG = LG_SampleBilinear8(ctx->blurredWorld,
									x + odX, y + odY);
								rR = pG.red; rG = pG.green; rB = pG.blue;
							}
						}
					}
				}

				PF_FpLong os = LG_ShadowIntensity(oDist, ctx->oSpread, ctx->oBlur);
				if (os > 0.001) {
					rR = rR * (1.0 - os) + ctx->oColor.red   * os;
					rG = rG * (1.0 - os) + ctx->oColor.green * os;
					rB = rB * (1.0 - os) + ctx->oColor.blue  * os;
				}

				outP->alpha = srcP->alpha;
				A_u_char shadowAlpha = LG_ClampByte(os * 255.0);
				if (shadowAlpha > outP->alpha) outP->alpha = shadowAlpha;
				outP->red   = LG_ClampByte(rR);
				outP->green = LG_ClampByte(rG);
				outP->blue  = LG_ClampByte(rB);
			}
		}
	}
	return PF_Err_NONE;
}


/* ================================================================
 *  Shared rendering core
 *  Called by both non-smart Render and SmartRender.
 *  alphaSource: layer whose alpha defines the glass shape
 *               (may differ from src when Shape Layer is set)
 * ================================================================ */

static PF_Err
LG_DoRender(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_EffectWorld	*src,
	PF_EffectWorld	*alphaSource,
	PF_EffectWorld	*output,
	LG_RenderCtx	*ctx)
{
	PF_Err err = PF_Err_NONE;

	A_long width  = output->width;
	A_long height = output->height;
	PF_Boolean deep = PF_WORLD_IS_DEEP(output);

	ctx->width       = width;
	ctx->height      = height;
	ctx->deep        = deep;
	ctx->src         = src;
	ctx->alphaSource = alphaSource;
	ctx->output      = output;

	/* Clip flags are set by PreRender via LG_PreRenderData → ctx.
	 * Do NOT override them here. */

	/* ---- Step 1: Frost Blur ---- */
	PF_EffectWorld blurredWorld;
	memset(&blurredWorld, 0, sizeof(blurredWorld));
	{
		PF_NewWorldFlags wFlags = PF_NewWorldFlag_CLEAR_PIXELS;
		if (deep) wFlags = (PF_NewWorldFlags)(wFlags | PF_NewWorldFlag_DEEP_PIXELS);
		ERR(PF_NEW_WORLD(width, height, wFlags, &blurredWorld));
	}
	if (!err) {
		ERR(LG_ApplyGaussianBlur(in_data, src, &blurredWorld, ctx->blurRadius));
	}

	ctx->blurredWorld = &blurredWorld;

	/* ---- Decide: Analytical SDF vs Chamfer distance ---- */
	/* Use Analytical SDF for visible-rect based glass tracking (B plan).
	 * SDF represents the VISIBLE RECT as glass plate, positioned at bufOrigin.
	 * This follows adjustment layer movement. */
	PF_Boolean useAnalyticalSDF = (alphaSource == src) && (ctx->surfTension < 0.5);
	ctx->useAnalyticalSDF = useAnalyticalSDF;

	A_long pixelCount = width * height;
	A_u_char *processedAlpha = NULL;
	PF_FpLong *innerDist = NULL;
	PF_FpLong *outerDist = NULL;

	if (useAnalyticalSDF) {
		/* ---- Analytical SDF path (B plan: visible rect as glass plate) ----
		 * SDF represents the VISIBLE BUFFER as glass plate.
		 * Positioned at bufOrigin so it follows layer movement.
		 * Comp-clipped edges use clipLeft/Right/Top/Bottom. */
		PF_FpLong halfW = (PF_FpLong)width * 0.5;
		PF_FpLong halfH = (PF_FpLong)height * 0.5;
		PF_FpLong R = ctx->cornerRadius;
		PF_FpLong maxR = (halfW < halfH) ? halfW : halfH;
		if (R > maxR) R = maxR;

		/* Center of visible rect in layer-space (B plan) */
		ctx->sdfCx    = (PF_FpLong)ctx->bufOriginX + (PF_FpLong)(width  - 1) * 0.5;
		ctx->sdfCy    = (PF_FpLong)ctx->bufOriginY + (PF_FpLong)(height - 1) * 0.5;
		ctx->sdfHalfW = halfW;
		ctx->sdfHalfH = halfH;
		ctx->sdfRadius = R;

		/* No distance arrays or processedAlpha needed */
		ctx->innerDist = NULL;
		ctx->outerDist = NULL;
		ctx->processedAlpha = NULL;
	} else {
		/* ---- Chamfer distance path (Shape Layer / Surface Tension) ---- */
		PF_Boolean needAlphaProc = (ctx->cornerRadius > 0.5) || (ctx->surfTension > 0.5);
		if (!err && needAlphaProc) {
			processedAlpha = (A_u_char *)malloc(pixelCount);
			if (!processedAlpha) { err = PF_Err_OUT_OF_MEMORY; }
			else {
				for (A_long yy = 0; yy < height; yy++) {
					for (A_long xx = 0; xx < width; xx++) {
						A_u_char a;
						if (deep) {
							PF_Pixel16 *p = LG_GetPixel16(alphaSource, xx, yy);
							a = (A_u_char)(p->alpha * 255 / PF_MAX_CHAN16);
						} else {
							PF_Pixel8 *p = LG_GetPixel8(alphaSource, xx, yy);
							a = p->alpha;
						}
						processedAlpha[yy * width + xx] = a;
					}
				}

				A_u_char *tmpBuf = (A_u_char *)malloc(pixelCount);

				/* Corner Rounding: binary mask */
				if (ctx->cornerRadius > 0.5) {
					PF_FpLong R = ctx->cornerRadius;
					PF_FpLong maxR = ((width < height) ? width : height) * 0.5;
					if (R > maxR) R = maxR;

					PF_FpLong cx0 = R, cy0 = R;
					PF_FpLong cx1 = (PF_FpLong)width  - 1.0 - R, cy1 = R;
					PF_FpLong cx2 = R, cy2 = (PF_FpLong)height - 1.0 - R;
					PF_FpLong cx3 = (PF_FpLong)width  - 1.0 - R;
					PF_FpLong cy3 = (PF_FpLong)height - 1.0 - R;

					for (A_long yy = 0; yy < height; yy++) {
						for (A_long xx = 0; xx < width; xx++) {
							PF_FpLong d2 = -1.0;
							if      (xx < R  && yy < R)   { PF_FpLong dx=xx-cx0, dy=yy-cy0; d2=dx*dx+dy*dy; }
							else if (xx > cx1 && yy < R)   { PF_FpLong dx=xx-cx1, dy=yy-cy1; d2=dx*dx+dy*dy; }
							else if (xx < R  && yy > cy2)  { PF_FpLong dx=xx-cx2, dy=yy-cy2; d2=dx*dx+dy*dy; }
							else if (xx > cx3 && yy > cy3) { PF_FpLong dx=xx-cx3, dy=yy-cy3; d2=dx*dx+dy*dy; }

							if (d2 >= 0.0 && d2 > R * R) {
								processedAlpha[yy * width + xx] = 0;
							}
						}
					}
				}

				/* Surface Tension: blur + threshold for metaball merge */
				if (tmpBuf && ctx->surfTension > 0.5) {
					A_long stRad = (A_long)(ctx->surfTension + 0.5);
					A_u_char threshold = (A_u_char)LG_Clamp(ctx->tensionThresh, 1.0, 255.0);
					int passes = 3;
					for (int p = 0; p < passes; p++) {
						for (A_long yy = 0; yy < height; yy++) {
							A_long sum = 0, count = 0;
							for (A_long xx = 0; xx <= stRad && xx < width; xx++) {
								sum += processedAlpha[yy * width + xx]; count++;
							}
							for (A_long xx = 0; xx < width; xx++) {
								tmpBuf[yy * width + xx] = (A_u_char)(sum / count);
								A_long addX = xx + stRad + 1, remX = xx - stRad;
								if (addX < width) { sum += processedAlpha[yy * width + addX]; count++; }
								if (remX >= 0)    { sum -= processedAlpha[yy * width + remX]; count--; }
							}
						}
						for (A_long xx = 0; xx < width; xx++) {
							A_long sum = 0, count = 0;
							for (A_long yy = 0; yy <= stRad && yy < height; yy++) {
								sum += tmpBuf[yy * width + xx]; count++;
							}
							for (A_long yy = 0; yy < height; yy++) {
								processedAlpha[yy * width + xx] = (A_u_char)(sum / count);
								A_long addY = yy + stRad + 1, remY = yy - stRad;
								if (addY < height) { sum += tmpBuf[addY * width + xx]; count++; }
								if (remY >= 0)     { sum -= tmpBuf[remY * width + xx]; count--; }
							}
						}
					}
					for (A_long i = 0; i < pixelCount; i++) {
						processedAlpha[i] = (processedAlpha[i] >= threshold) ? 255 : 0;
					}
				}

				if (tmpBuf) free(tmpBuf);
			}
		}
		ctx->processedAlpha = processedAlpha;

		/* ---- Edge distance maps (Chamfer) ---- */
		if (!err) {
			innerDist = (PF_FpLong *)calloc(pixelCount, sizeof(PF_FpLong));
			outerDist = (PF_FpLong *)calloc(pixelCount, sizeof(PF_FpLong));
			if (!innerDist || !outerDist) err = PF_Err_OUT_OF_MEMORY;
		}

		if (!err) {
			PF_FpLong maxInner = ctx->effBevel + 5.0;
			PF_FpLong maxShadowInner = (-ctx->iSpread) + ctx->iBlur + 5.0;
			if (maxShadowInner > maxInner) maxInner = maxShadowInner;
			if (maxInner < 30.0) maxInner = 30.0;

			PF_FpLong maxOuter = ctx->effBevel + 5.0;
			PF_FpLong maxShadowOuter = (-ctx->oSpread) + ctx->oBlur + 5.0;
			if (maxShadowOuter > maxOuter) maxOuter = maxShadowOuter;
			if (maxOuter < 50.0) maxOuter = 50.0;

			LG_ChamferDist(alphaSource, innerDist, width, height, maxInner, deep, 0, processedAlpha);
			LG_ChamferDist(alphaSource, outerDist, width, height, maxOuter, deep, 1, processedAlpha);

			LG_SmoothDistField(innerDist, width, height, 2);
			LG_SmoothDistField(outerDist, width, height, 1);
		}

		ctx->innerDist = innerDist;
		ctx->outerDist = outerDist;
	}

	/* ---- Step 3: Multi-threaded composite via iterate_generic ---- */
	if (!err) {
		AEFX_SuiteScoper<PF_Iterate8Suite1> iterSuite =
			AEFX_SuiteScoper<PF_Iterate8Suite1>(in_data,
				kPFIterate8Suite,
				kPFIterate8SuiteVersion1,
				out_data);
		ERR(iterSuite->iterate_generic(height, (void *)ctx, LG_ProcessScanline));
	}

	/* Cleanup */
	if (processedAlpha) free(processedAlpha);
	if (innerDist) free(innerDist);
	if (outerDist) free(outerDist);
	if (blurredWorld.data) {
		PF_DISPOSE_WORLD(&blurredWorld);
	}

	return err;
}


/* ================================================================
 *  Non-smart Render (Premiere fallback)
 * ================================================================ */

static PF_Err
Render(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err err = PF_Err_NONE;

	PF_EffectWorld *src = &params[LG_INPUT]->u.ld;

	/* Shape Layer: use its alpha for edge detection if set */
	PF_EffectWorld *alphaSource = src;
	if (params[LG_SHAPE_LAYER]->u.ld.data != NULL) {
		alphaSource = &params[LG_SHAPE_LAYER]->u.ld;
	}

	/* Background Layer: if set, refract this instead of input */
	PF_EffectWorld *bgSource = src;
	if (params[LG_BG_LAYER]->u.ld.data != NULL) {
		bgSource = &params[LG_BG_LAYER]->u.ld;
	}

	PF_FpLong dsX = (PF_FpLong)in_data->downsample_x.num / in_data->downsample_x.den;
	PF_FpLong lightAngle = PF_FpLong(params[LG_LIGHT_ANGLE]->u.ad.value) / 65536.0;
	PF_FpLong lightRad   = lightAngle * PF_RAD_PER_DEGREE;
	PF_FpLong frostBlur  = params[LG_FROST_BLUR]->u.fs_d.value;

	LG_RenderCtx ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.effBevel     = params[LG_BEVEL_WIDTH]->u.fs_d.value * dsX;
	ctx.bevelFalloff = params[LG_BEVEL_FALLOFF]->u.fs_d.value / 100.0;
	ctx.effRefStr    = params[LG_REFRACT_STRENGTH]->u.fs_d.value * dsX;
	ctx.effChrom     = params[LG_CHROMATIC_AB]->u.fs_d.value * dsX;
	ctx.chromDecay   = params[LG_CHROM_DECAY]->u.fs_d.value / 100.0;
	ctx.effNoise     = params[LG_NOISE_AMOUNT]->u.fs_d.value * dsX;
	ctx.noiseFreq    = 0.025;
	ctx.blurRadius   = frostBlur * dsX;
	ctx.satFactor    = params[LG_SATURATION]->u.fs_d.value / 100.0;
	ctx.ldx          = cos(lightRad);
	ctx.ldy          = -sin(lightRad);
	ctx.specInt      = params[LG_SPECULAR_INTENSITY]->u.fs_d.value;
	ctx.tintCol      = params[LG_TINT_COLOR]->u.cd.value;
	ctx.tintOpacity  = params[LG_TINT_OPACITY]->u.fs_d.value;
	ctx.iSpread      = params[LG_ISHADOW_SPREAD]->u.fs_d.value;
	ctx.iBlur        = params[LG_ISHADOW_BLUR]->u.fs_d.value;
	ctx.iColor       = params[LG_ISHADOW_COLOR]->u.cd.value;
	ctx.oSpread      = params[LG_OSHADOW_SPREAD]->u.fs_d.value;
	ctx.oBlur        = params[LG_OSHADOW_BLUR]->u.fs_d.value;
	ctx.oColor       = params[LG_OSHADOW_COLOR]->u.cd.value;
	ctx.cornerRadius = params[LG_CORNER_RADIUS]->u.fs_d.value * dsX;
	ctx.surfTension  = params[LG_SURFACE_TENSION]->u.fs_d.value;
	ctx.tensionThresh= params[LG_TENSION_THRESHOLD]->u.fs_d.value;
	ctx.clipCompBounds = params[LG_CLIP_COMP_BOUNDS]->u.bd.value;

	/* In the legacy Render path (used by 3D layers), derive buffer origin
	 * and clipping from extent_hint so SDF matches layer position.
	 * extent_hint is in layer-local coordinates relative to output buffer. */
	{
		PF_LRect eh = in_data->extent_hint;
		A_long fullW = in_data->width;
		A_long fullH = in_data->height;
		A_long bufW  = output->width;
		A_long bufH  = output->height;

		/* Buffer origin: where this render buffer starts in layer coordinates.
		 * For 3D layers, extent_hint gives us the layer's position. */
		ctx.bufOriginX = eh.left;
		ctx.bufOriginY = eh.top;
		ctx.layerW     = fullW;
		ctx.layerH     = fullH;

		/* Clip at Comp Bounds: When OFF, show edges even outside comp */
		if (ctx.clipCompBounds) {
			/* Clipping: buffer edge doesn't reach layer edge (2px tolerance) */
			ctx.clipLeft   = (eh.left   > 2);
			ctx.clipRight  = (eh.right  < (fullW - 2));
			ctx.clipTop    = (eh.top    > 2);
			ctx.clipBottom = (eh.bottom < (fullH - 2));

			/* If buffer covers full layer, no clipping */
			if (bufW >= fullW - 2) {
				ctx.clipLeft = ctx.clipRight = FALSE;
				ctx.bufOriginX = 0;
			}
			if (bufH >= fullH - 2) {
				ctx.clipTop = ctx.clipBottom = FALSE;
				ctx.bufOriginY = 0;
			}
		} else {
			/* Show edges even when glass goes outside comp */
			ctx.clipLeft = ctx.clipRight = ctx.clipTop = ctx.clipBottom = FALSE;
		}
	}

	/* Debug: log legacy path values */
	{
		PF_LRect eh = in_data->extent_hint;
		FILE *f = fopen("/tmp/lg_render_legacy.txt", "a");
		if (f) {
			fprintf(f, "eh(%d,%d,%d,%d) outW=%d outH=%d inW=%d inH=%d srcW=%d srcH=%d\n",
				(int)eh.left,(int)eh.top,(int)eh.right,(int)eh.bottom,
				(int)output->width,(int)output->height,
				(int)in_data->width,(int)in_data->height,
				(int)src->width,(int)src->height);
			fclose(f);
		}
	}

	ERR(LG_DoRender(in_data, out_data, bgSource, alphaSource, output, &ctx));

	return err;
}


/* ================================================================
 *  SmartFX: PreRender data
 * ================================================================ */

struct LG_PreRenderData {
	PF_FpLong bezelWidth, bevelFalloff, refractStr, chromAb, chromDecay, noiseAmt;
	PF_FpLong frostBlur, specInt, lightAngle, satBoost;
	PF_Pixel  tintCol;
	PF_FpLong tintOpacity;
	PF_FpLong iSpread, iBlur, oSpread, oBlur;
	PF_Pixel  iColor, oColor;
	PF_FpLong cornerRadius;
	PF_FpLong surfTension, tensionThresh;
	PF_Boolean hasShapeLayer;
	PF_Boolean hasBGLayer;
	/* Clip edges when glass goes outside comp bounds */
	PF_Boolean clipCompBounds;
	/* Comp boundary clip: layer extends beyond comp on this edge */
	PF_Boolean clipLeft, clipRight, clipTop, clipBottom;
	/* Full layer geometry for SDF (comp coords) */
	A_long layerW, layerH;        /* full layer dimensions */
	A_long bufOriginX, bufOriginY; /* buffer origin in layer coords */
};

static void
LG_DisposePreRenderData(void *pre_render_dataPV)
{
	if (pre_render_dataPV) {
		free(pre_render_dataPV);
	}
}


/* ================================================================
 *  SmartFX: PreRender
 * ================================================================ */

static PF_Err
PreRender(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_PreRenderExtra	*extraP)
{
	PF_Err err = PF_Err_NONE;
	PF_RenderRequest req = extraP->input->output_request;
	PF_CheckoutResult in_result;

	LG_PreRenderData *infoP = (LG_PreRenderData *)malloc(sizeof(LG_PreRenderData));
	if (!infoP) return PF_Err_OUT_OF_MEMORY;
	memset(infoP, 0, sizeof(LG_PreRenderData));

	/* Checkout all parameters */
	PF_ParamDef cur;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_BEVEL_WIDTH, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->bezelWidth = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_BEVEL_FALLOFF, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->bevelFalloff = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_REFRACT_STRENGTH, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->refractStr = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_CHROMATIC_AB, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->chromAb = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_CHROM_DECAY, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->chromDecay = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_NOISE_AMOUNT, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->noiseAmt = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_FROST_BLUR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->frostBlur = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_SPECULAR_INTENSITY, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->specInt = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_LIGHT_ANGLE, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->lightAngle = PF_FpLong(cur.u.ad.value) / 65536.0;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_SATURATION, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->satBoost = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_TINT_COLOR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->tintCol = cur.u.cd.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_TINT_OPACITY, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->tintOpacity = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_ISHADOW_SPREAD, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->iSpread = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_ISHADOW_BLUR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->iBlur = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_ISHADOW_COLOR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->iColor = cur.u.cd.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_OSHADOW_SPREAD, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->oSpread = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_OSHADOW_BLUR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->oBlur = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_OSHADOW_COLOR, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->oColor = cur.u.cd.value;

	/* Shape Control */
	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_CORNER_RADIUS, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->cornerRadius = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_SURFACE_TENSION, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->surfTension = cur.u.fs_d.value;

	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_TENSION_THRESHOLD, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->tensionThresh = cur.u.fs_d.value;

	/* Clip at Comp Bounds */
	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_CLIP_COMP_BOUNDS, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->clipCompBounds = cur.u.bd.value;

	/* Check shape layer */
	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_SHAPE_LAYER, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->hasShapeLayer = (cur.u.ld.data != NULL);
	ERR(PF_CHECKIN_PARAM(in_data, &cur));

	/* Check background layer */
	AEFX_CLR_STRUCT(cur);
	ERR(PF_CHECKOUT_PARAM(in_data, LG_BG_LAYER, in_data->current_time, in_data->time_step, in_data->time_scale, &cur));
	infoP->hasBGLayer = (cur.u.ld.data != NULL);
	ERR(PF_CHECKIN_PARAM(in_data, &cur));

	extraP->output->pre_render_data = infoP;
	extraP->output->delete_pre_render_data_func = LG_DisposePreRenderData;

	/* Checkout input layer with original request */
	ERR(extraP->cb->checkout_layer(in_data->effect_ref,
		LG_INPUT, LG_INPUT, &req,
		in_data->current_time, in_data->time_step, in_data->time_scale,
		&in_result));

	UnionLRect(&in_result.result_rect,     &extraP->output->result_rect);
	UnionLRect(&in_result.max_result_rect, &extraP->output->max_result_rect);

	/* Compute full layer geometry and clipping flags.
	 *
	 * in_data->width/height = the TRUE full layer size in layer-local px.
	 * vis (in_result.result_rect) = the visible sub-rect we actually get
	 *   to render, clipped to comp bounds.  Coords are layer-local.
	 *
	 * The buffer we receive in SmartRender has size (vis.W × vis.H) and
	 * its pixel (0,0) corresponds to layer-local (vis.left, vis.top).
	 *
	 * bufOrigin = (vis.left, vis.top): how many pixels from the full
	 *   layer's top-left corner the buffer starts.
	 *
	 * Clipping: an edge is clipped when the vis rect is cut short
	 *   of the full layer rect on that side (layer extends beyond
	 *   comp there → suppress the glass edge). */
	{
		PF_LRect vis = in_result.result_rect;
		A_long fullW = in_data->width;
		A_long fullH = in_data->height;
		A_long visW  = vis.right - vis.left;
		A_long visH  = vis.bottom - vis.top;

		/* Buffer origin in layer-local coords */
		A_long bufOX = vis.left;
		A_long bufOY = vis.top;

		/* Clip at Comp Bounds: When OFF, don't clip at comp edges
		 * When ON (default), clip edges where glass goes outside comp */
		if (infoP->clipCompBounds) {
			/* Clipping: vis edge doesn't reach the full layer edge.
			 * We give a small tolerance of 2px for AE rounding. */
			infoP->clipLeft   = (vis.left   > 2);
			infoP->clipRight  = (vis.right  < (fullW - 2));
			infoP->clipTop    = (vis.top    > 2);
			infoP->clipBottom = (vis.bottom < (fullH - 2));

			/* But if vis covers the full layer on an axis, no clip. */
			if (visW >= fullW - 2) {
				infoP->clipLeft = infoP->clipRight = FALSE;
				bufOX = 0;  /* buffer == full layer width */
			}
			if (visH >= fullH - 2) {
				infoP->clipTop = infoP->clipBottom = FALSE;
				bufOY = 0;
			}
		} else {
			/* Clip at Comp Bounds is OFF: show edges even outside comp */
			infoP->clipLeft = infoP->clipRight = infoP->clipTop = infoP->clipBottom = FALSE;
		}

		infoP->layerW     = fullW;
		infoP->layerH     = fullH;
		infoP->bufOriginX = bufOX;
		infoP->bufOriginY = bufOY;

		/* Debug log */
		{
			FILE *f = fopen("/tmp/lg_prerender.txt", "a");
			if (f) {
				fprintf(f, "vis(%d,%d,%d,%d) full(%dx%d) clip[L%d R%d T%d B%d] bufOrig(%d,%d)\n",
					(int)vis.left,(int)vis.top,(int)vis.right,(int)vis.bottom,
					(int)fullW,(int)fullH,
					infoP->clipLeft,infoP->clipRight,infoP->clipTop,infoP->clipBottom,
					(int)bufOX,(int)bufOY);
				fclose(f);
			}
		}
	}

	/* Checkout shape layer if set */
	if (!err && infoP->hasShapeLayer) {
		PF_CheckoutResult shape_result;
		ERR(extraP->cb->checkout_layer(in_data->effect_ref,
			LG_SHAPE_LAYER, LG_SHAPE_LAYER, &req,
			in_data->current_time, in_data->time_step, in_data->time_scale,
			&shape_result));
		UnionLRect(&shape_result.result_rect,     &extraP->output->result_rect);
		UnionLRect(&shape_result.max_result_rect, &extraP->output->max_result_rect);
	}

	/* Checkout background layer if set */
	if (!err && infoP->hasBGLayer) {
		PF_CheckoutResult bg_result;
		ERR(extraP->cb->checkout_layer(in_data->effect_ref,
			LG_BG_LAYER, LG_BG_LAYER, &req,
			in_data->current_time, in_data->time_step, in_data->time_scale,
			&bg_result));
		UnionLRect(&bg_result.result_rect,     &extraP->output->result_rect);
		UnionLRect(&bg_result.max_result_rect, &extraP->output->max_result_rect);
	}

	return err;
}


/* ================================================================
 *  SmartFX: SmartRender
 * ================================================================ */

static PF_Err
SmartRender(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_SmartRenderExtra	*extraP)
{
	PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

	LG_PreRenderData *infoP = (LG_PreRenderData *)extraP->input->pre_render_data;
	if (!infoP) return PF_Err_INTERNAL_STRUCT_DAMAGED;

	PF_EffectWorld *input_worldP  = NULL;
	PF_EffectWorld *output_worldP = NULL;
	PF_EffectWorld *shape_worldP  = NULL;
	PF_EffectWorld *bg_worldP     = NULL;

	ERR(extraP->cb->checkout_layer_pixels(in_data->effect_ref, LG_INPUT, &input_worldP));
	ERR(extraP->cb->checkout_output(in_data->effect_ref, &output_worldP));

	if (!err && infoP->hasShapeLayer) {
		ERR(extraP->cb->checkout_layer_pixels(in_data->effect_ref, LG_SHAPE_LAYER, &shape_worldP));
	}
	if (!err && infoP->hasBGLayer) {
		ERR(extraP->cb->checkout_layer_pixels(in_data->effect_ref, LG_BG_LAYER, &bg_worldP));
	}

	if (!err && input_worldP && output_worldP) {
		PF_FpLong dsX = (PF_FpLong)in_data->downsample_x.num / in_data->downsample_x.den;
		PF_FpLong lightRad = infoP->lightAngle * PF_RAD_PER_DEGREE;

		LG_RenderCtx ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.effBevel      = infoP->bezelWidth * dsX;
		ctx.bevelFalloff  = infoP->bevelFalloff / 100.0;
		ctx.effRefStr     = infoP->refractStr * dsX;
		ctx.effChrom      = infoP->chromAb * dsX;
		ctx.chromDecay    = infoP->chromDecay / 100.0;
		ctx.effNoise      = infoP->noiseAmt * dsX;
		ctx.noiseFreq     = 0.025;
		ctx.blurRadius    = infoP->frostBlur * dsX;
		ctx.satFactor     = infoP->satBoost / 100.0;
		ctx.ldx           = cos(lightRad);
		ctx.ldy           = -sin(lightRad);
		ctx.specInt       = infoP->specInt;
		ctx.tintCol       = infoP->tintCol;
		ctx.tintOpacity   = infoP->tintOpacity;
		ctx.iSpread       = infoP->iSpread;
		ctx.iBlur         = infoP->iBlur;
		ctx.iColor        = infoP->iColor;
		ctx.oSpread       = infoP->oSpread;
		ctx.oBlur         = infoP->oBlur;
		ctx.oColor        = infoP->oColor;
		ctx.cornerRadius  = infoP->cornerRadius * dsX;
		ctx.surfTension   = infoP->surfTension;
		ctx.tensionThresh = infoP->tensionThresh;
		ctx.clipCompBounds = infoP->clipCompBounds;
		ctx.clipLeft      = infoP->clipLeft;
		ctx.clipRight     = infoP->clipRight;
		ctx.clipTop       = infoP->clipTop;
		ctx.clipBottom    = infoP->clipBottom;
		ctx.layerW        = infoP->layerW;
		ctx.layerH        = infoP->layerH;
		ctx.bufOriginX    = infoP->bufOriginX;
		ctx.bufOriginY    = infoP->bufOriginY;

		PF_EffectWorld *alphaSource = shape_worldP ? shape_worldP : input_worldP;
		PF_EffectWorld *bgSource    = bg_worldP    ? bg_worldP    : input_worldP;
		ERR(LG_DoRender(in_data, out_data, bgSource, alphaSource, output_worldP, &ctx));
	}

	ERR2(extraP->cb->checkin_layer_pixels(in_data->effect_ref, LG_INPUT));
	if (infoP->hasShapeLayer) {
		ERR2(extraP->cb->checkin_layer_pixels(in_data->effect_ref, LG_SHAPE_LAYER));
	}
	if (infoP->hasBGLayer) {
		ERR2(extraP->cb->checkin_layer_pixels(in_data->effect_ref, LG_BG_LAYER));
	}

	return err;
}


/* ================================================================
 *  Plugin Entry Points
 * ================================================================ */

extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"Liquid Glass",
		"LiquidGlass",
		"Stylize",
		AE_RESERVED_INFO);

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err err = PF_Err_NONE;

	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data, out_data, params, output);
				break;

			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data, out_data, params, output);
				break;

			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data, out_data, params, output);
				break;

			case PF_Cmd_RENDER:
			{
				FILE *f = fopen("/tmp/lg_path.txt", "a");
				if (f) { fprintf(f, "RENDER (legacy)\n"); fclose(f); }
			}
			err = Render(in_data, out_data, params, output);
			break;

			case PF_Cmd_SMART_PRE_RENDER:
			{
				FILE *f = fopen("/tmp/lg_path.txt", "a");
				if (f) { fprintf(f, "SMART_PRE_RENDER\n"); fclose(f); }
			}
			err = PreRender(in_data, out_data, (PF_PreRenderExtra *)extra);
			break;

			case PF_Cmd_SMART_RENDER:
			{
				FILE *f = fopen("/tmp/lg_path.txt", "a");
				if (f) { fprintf(f, "SMART_RENDER\n"); fclose(f); }
			}
			err = SmartRender(in_data, out_data, (PF_SmartRenderExtra *)extra);
			break;
		}
	}
	catch (PF_Err &thrown_err) {
		err = thrown_err;
	}
	return err;
}
