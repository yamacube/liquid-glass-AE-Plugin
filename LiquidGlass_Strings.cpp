/*******************************************************************/
/*                       LIQUID GLASS                              */
/*              String Resources                                   */
/*******************************************************************/

#include "LiquidGlass_Strings.h"

typedef struct {
	unsigned long	index;
	char			str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
	{StrID_NONE,						""},
	{StrID_Name,						"Liquid Glass"},
	{StrID_Description,					"iOS 26 Liquid Glass effect.\rBezel refraction, specular rim, inner & outer shadow."},

	{StrID_Shape_Layer,					"Shape Layer"},
	{StrID_BG_Layer,					"Background Layer"},

	{StrID_Shape_Group,					"Shape Control"},
	{StrID_Source_Mode,					"Source Mode"},
	{StrID_Self_Alpha,					"Self Alpha"},
	{StrID_External_Layer,				"External Layer"},
	{StrID_Corner_Radius,				"Corner Radius"},
	{StrID_Geometry_Radius,				"Geometry Radius"},
	{StrID_Expansion,					"Expansion"},
	{StrID_Surface_Tension,				"Surface Tension"},
	{StrID_Tension_Threshold,			"Tension Threshold"},
	{StrID_Clip_Comp_Bounds,			"Clip at Comp Bounds"},

	{StrID_Refract_Group,				"Refraction"},
	{StrID_Bevel_Width,					"Bevel Width"},
	{StrID_Bevel_Falloff,				"Bevel Falloff (%)"},
	{StrID_Refract_Strength,			"Refraction Strength"},
	{StrID_Chromatic_Ab,				"Chromatic Aberration"},
	{StrID_Chrom_Decay,					"CA Range (%)"},
	{StrID_Noise_Amount,				"Noise Amount"},

	{StrID_Glass_Group,					"Glass Surface"},
	{StrID_Frost_Blur,					"Frost Blur"},
	{StrID_Specular_Intensity,			"Specular Intensity"},
	{StrID_Light_Angle,					"Light Angle"},
	{StrID_Saturation,					"Saturation Boost"},
	{StrID_Tint_Color,					"Tint Color"},
	{StrID_Tint_Opacity,				"Tint Opacity"},

	{StrID_InnerShadow_Group,			"Inner Shadow"},
	{StrID_IShadow_Spread,				"Shadow Spread"},
	{StrID_IShadow_Blur,				"Shadow Blur"},
	{StrID_IShadow_Color,				"Shadow Color"},

	{StrID_OuterShadow_Group,			"Outer Shadow"},
	{StrID_OShadow_Spread,				"Shadow Spread"},
	{StrID_OShadow_Blur,				"Shadow Blur"},
	{StrID_OShadow_Color,				"Shadow Color"},
};

#ifdef __cplusplus
extern "C" {
#endif

char *GetStringPtr(int strNum)
{
	return (char *)g_strs[strNum].str;
}

#ifdef __cplusplus
}
#endif
