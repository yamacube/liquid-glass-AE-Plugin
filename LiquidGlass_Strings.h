/*******************************************************************/
/*                       LIQUID GLASS                              */
/*              String Resource IDs                                */
/*******************************************************************/

#pragma once

typedef enum {
	StrID_NONE = 0,
	StrID_Name,
	StrID_Description,

	StrID_Shape_Layer,
	StrID_BG_Layer,

	StrID_Shape_Group,
	StrID_Source_Mode,
	StrID_Self_Alpha,
	StrID_External_Layer,
	StrID_Corner_Radius,
	StrID_Geometry_Radius,
	StrID_Expansion,
	StrID_Surface_Tension,
	StrID_Tension_Threshold,
	StrID_Clip_Comp_Bounds,

	StrID_Refract_Group,
	StrID_Bevel_Width,
	StrID_Bevel_Falloff,
	StrID_Refract_Strength,
	StrID_Chromatic_Ab,
	StrID_Chrom_Decay,
	StrID_Noise_Amount,

	StrID_Glass_Group,
	StrID_Frost_Blur,
	StrID_Specular_Intensity,
	StrID_Light_Angle,
	StrID_Saturation,
	StrID_Tint_Color,
	StrID_Tint_Opacity,

	StrID_InnerShadow_Group,
	StrID_IShadow_Spread,
	StrID_IShadow_Blur,
	StrID_IShadow_Color,

	StrID_OuterShadow_Group,
	StrID_OShadow_Spread,
	StrID_OShadow_Blur,
	StrID_OShadow_Color,

	StrID_NUMTYPES
} StrIDType;
