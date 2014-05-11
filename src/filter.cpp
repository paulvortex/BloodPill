////////////////////////////////////////////////////////////////
//
// Blood Pill - scaler
// coded by Pavel [VorteX] Timofeyev and placed to public domain
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "bloodpill.h"
#include "scale2x.h"
#include "filter.h"
#include "rawfile.h"

// return scaled size
int ImgFilter_Size(int sourcesize, imgfilter_t scaler)
{
	imgfilter_t magnify = scaler & FILTER_MASK_MAGNIFY;

	if (magnify & FILTER_SCALE4X) return sourcesize * 4;
	if (magnify & FILTER_SCALE2X) return sourcesize * 2;
	return sourcesize;
}

// image filter create borders
// this fitler will prepare tile for linear texture filtering
// it will reduce image size by 2 pixels, and create the border which will copied from neighboring pixels
void ImgFilter_CreateClampedBorder(int width, int height, int bpp, byte *pixels)
{
	byte *out, *end, *tmp;
	int i, pitch;

	if (width < 8 || height < 8)
		return;
	pitch = width*bpp;
	end = pixels + pitch*height;
	tmp = (byte *)mem_alloc(pitch);
	// left
	i = (width/2) - 2;
	for (out = pixels; out < end; out += pitch)
	{
		memcpy(tmp, out, i * bpp);
		memcpy(out + bpp, tmp, i * bpp);
	}
	// right
	i = width/2 - 1;
	for (out = pixels + i + 1; out < end; out += pitch)
	{
		memcpy(tmp, out+bpp, i*bpp);
		memcpy(out, tmp, i*bpp);
	}
	// top
	for(i = (width/2) - 2; i > 0; i--)
		memcpy(pixels + i*pitch, pixels + (i-1)*pitch, pitch);
	// bottom
	for(i = width/2; i < (height-1); i++)
		memcpy(pixels + i*pitch, pixels + (i+1)*pitch, pitch);
	mem_free(tmp);
}

// image filter - scale
void ImgFilter_Scale(int src_width, int src_height, int src_bpp, byte *src_pixels, byte *out, imgfilter_t scaler)
{
	int scaledwidth, scaledheight;
	unsigned int scale;
	byte *in;

	scale = ImgFilter_Size(1, scaler);
	if (scale > 1)
	{
		scaledwidth  = ImgFilter_Size(src_width, scaler);
		scaledheight = ImgFilter_Size(src_height, scaler);
		if (sxCheck(scale, src_bpp, src_width, src_height) != SCALEX_OK) 
			Error("ImgFilter: unsupported scale/BPP/width/height %i/%i/%i/%i\n", scale, src_bpp, src_width, src_height);
		// copy source data to temporary storage
		if (src_pixels != out)
			in = src_pixels;
		else
		{
			in = (byte *)mem_alloc(scaledwidth * scaledheight * src_bpp);
			memcpy(in, src_pixels, src_width * src_height * src_bpp);
		}
		// scale
		sxScale(scale, out, scaledwidth * src_bpp, in, src_width * src_bpp, src_bpp, src_width, src_height);
		if (src_pixels == out)
			mem_free(in);
		return;
	}

	// no scale, just copy pixels if needed
	if (src_pixels != out)
		memcpy(out, src_pixels, src_width*src_height*src_bpp);
}

// image filter - scale and prepare borders
void ImgFilter_ScaleClamp(int src_width, int src_height, int src_bpp, byte *src_pixels, byte *out, imgfilter_t scaler)
{
	ImgFilter_Scale(src_width, src_height, src_bpp, src_pixels, out, scaler);
	if (scaler & FILTER_TRANSFORM_CREATEBORDER)
		ImgFilter_CreateClampedBorder(ImgFilter_Size(src_width, scaler), ImgFilter_Size(src_height, scaler), src_bpp, out);
}

// image filter
void ImgFilter(int src_width, int src_height, int src_bpp, byte *src_pixels, byte *out, imgfilter_t scaler)
{
	byte *tiledata[64], *in;
	int i, j, tw, th, col, row;

	// no scale?
	if (scaler == FILTER_NONE)
		return;

	// out == NULL defaults to src_pixels
	if (!out)
		out = src_pixels;

	// tilemap filtering
	if (scaler & FILTER_TRANSFORM_TILEMAP8X8)
	{
		tw = src_width / 8;
		th = src_height / 8;
		// decompose images into separate tiles
		for (i = 0; i < 64; i++)
		{
			row = i/8;
			col = i - row*8;
			tiledata[i] = (byte *)mem_alloc(ImgFilter_Size(tw, scaler) * ImgFilter_Size(th, scaler) * src_bpp);
			in = tiledata[i];
			for (j = 0; j < th; j++)
			{
				memcpy(in, src_pixels + (row*th + j)*src_width*src_bpp + col*tw*src_bpp, tw*src_bpp);
				in += tw*src_bpp;
			}
		}
		// scale tiles
		for (i = 0; i < 64; i++)
			ImgFilter_ScaleClamp(tw, th, src_bpp, tiledata[i], tiledata[i], scaler);
		// compose tiles back to the image
		tw = ImgFilter_Size(tw, scaler);
		th = ImgFilter_Size(th, scaler);
		for (i = 0; i < 64; i++)
		{
			row = i/8;
			col = i - row*8;
			in = tiledata[i];
			for (j = 0; j < th; j++)
			{
				memcpy(src_pixels + (row*th + j)*ImgFilter_Size(src_width, scaler)*src_bpp + col*tw*src_bpp, in, tw*src_bpp);
				in += tw*src_bpp;
			}
			mem_free(tiledata[i]);
		}
		return;
	}
	// standart image filtering
	ImgFilter_ScaleClamp(src_width, src_height, src_bpp, src_pixels, out, scaler);
}

// color transform filter
void ImgFilter_ColorTransform(int src_width, int src_height, int src_bpp, byte *src_pixels, float color_scale, int color_subtract)
{
	byte *in, *end;

	// no changes?
	if (color_scale == 1.0f && color_subtract == 0)
		return;

	#define channeltransform(x) (byte)max(0, min(255, (x)*color_scale - color_subtract))

	// transform
	in = src_pixels;
	end = src_pixels + src_width * src_height * src_bpp;
	if (src_bpp == 4)
	{
		while (in < end)
		{
			in[0] = channeltransform(in[0]);
			in[1] = channeltransform(in[1]);
			in[2] = channeltransform(in[2]);
			in += 4;
		}
	}
	else if (src_bpp == 3)
	{
		while (in < end)
		{
			in[0] = channeltransform(in[0]);
			in[1] = channeltransform(in[1]);
			in[2] = channeltransform(in[2]);
			in += 3;
		}
	}
	else if (src_bpp == 1)
	{
		while (in < end)
		{
			in[0] = channeltransform(in[0]);
			in += 1;
		}
	}
	else
		Error("ImgFilter_ColorTransform: unsupported BPP %i\n", src_bpp);

	#undef channeltransform
}