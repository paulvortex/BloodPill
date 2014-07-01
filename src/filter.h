#ifndef _FILTER_H_
#define _FILTER_H_

// filters
#define FILTER_NONE		              0
#define FILTER_SCALE2X	              1
#define FILTER_SCALE4X	              2
#define FILTER_XBRZ2X	              4
#define FILTER_XBRZ4X	              8
#define FILTER_TRANSFORM_TILEMAP8X8   1024
#define FILTER_TRANSFORM_CREATEBORDER 2048

#define FILTER_MASK_MAGNIFY (FILTER_SCALE2X+FILTER_SCALE4X+FILTER_XBRZ2X+FILTER_XBRZ4X)

typedef unsigned int imgfilter_t;

// filter functions
int  ImgFilter_Size(int sourcesize, imgfilter_t scaler);
void ImgFilter_ColorTransform(int src_width, int src_height, int src_bpp, byte *src_pixels, float color_scale, int color_subtract);
void ImgFilter(int src_width, int src_height, int src_bpp, byte *src_pixels, byte *src_palette, int src_palette_bpp, byte *out, imgfilter_t scaler);

#endif
