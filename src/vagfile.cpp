// VAG-Pack, VAG-Depack, hacked by bITmASTER@bigfoot.com
// v0.1

#include "bloodpill.h"
#include "bigfile.h"

// todo: cleanup
// magic matrix used for VAG depacking
double f[5][2] = 
{ 
	{ 0.0, 0.0 },
	{  60.0 / 64.0,  0.0 },
	{  115.0 / 64.0, -52.0 / 64.0 },
	{  98.0 / 64.0, -55.0 / 64.0 },
	{  122.0 / 64.0, -60.0 / 64.0 } 
};
double samples[28];

// VAG depacking - test raw PCM file
unsigned int VAG_UnpackTest(byte *data, unsigned int datasize, int offset) 
{
	int predict_nr, shift_factor, flags;
    int i, d, s, size;
    static double s_1 = 0.0;
    static double s_2 = 0.0;
	byte *in;

	// determine amount if data that should be written
	// fixme: OPTIMIZE
	in = data + offset; // skip VAG header
	datasize = datasize - offset;
	if (datasize < 0)
		return -1;
	size = 0;
	#define inc() if(datasize < 0) return -1;in++;datasize--;
	while(datasize > 0)
	{
		predict_nr = in[0]; inc()
		shift_factor = predict_nr & 0xf;
		predict_nr = (int)(predict_nr >> 4);
		if (predict_nr < 0)
			return -1;
		if (predict_nr > 4)
			return -1;
		flags = in[0]; inc() // flags
		if (flags == 7)
			break; // end of file             
		for (i = 0; i < 28; i += 2) 
		{
			d = in[0]; inc()
			s = (d & 0xf) << 12;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i] = (double)(s >> shift_factor);
			s = (d & 0xf0) << 8;
			if (s & 0x8000)
			s |= 0xffff0000;
			samples[i+1] = (double)(s >> shift_factor);
		}
		for ( i = 0; i < 28; i++ ) 
		{
			samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
			s_2 = s_1;
			s_1 = samples[i];
			d = (int)(samples[i] + 0.5);
			size += 2;
		}
	}
	#undef inc
	return in - data;
}

// VAG depacking - writes raw PCM file
void VAG_Unpack(byte *data, int offset, int databytes, byte **bufferptr, int *outsize) 
{
    int predict_nr, shift_factor, flags, datasize;
    int i, d, s, size;
    static double s_1 = 0.0;
    static double s_2 = 0.0;
	byte *in, *out;

	// determine amount if data that should be written
	// fixme: OPTIMIZE
	in = data + offset; // skip VAG header
	datasize = databytes - offset;
	size = 0;
	#define inc() if(datasize < 0) break;in++;datasize--;
	while(datasize > 0)
	{
		predict_nr = in[0]; inc()
		shift_factor = predict_nr & 0xf;
		predict_nr >>= 4;
		flags = in[0]; inc() // flags
		if (flags == 7)
			break;             
		for (i = 0; i < 28; i += 2) 
		{
			d = in[0]; inc()
			s = (d & 0xf) << 12;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i] = (double)(s >> shift_factor);
			s = (d & 0xf0) << 8;
			if (s & 0x8000)
			s |= 0xffff0000;
			samples[i+1] = (double)(s >> shift_factor);
		}
		for ( i = 0; i < 28; i++ ) 
		{
			samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
			s_2 = s_1;
			s_1 = samples[i];
			d = (int)(samples[i] + 0.5);
			size += 2;
		}
	}
      
	// decode
	in = data + offset;
	datasize = databytes - offset;
	*bufferptr = (byte *)mem_alloc(size);
	out = *bufferptr;
	while(1)
	{
		predict_nr = in[0]; inc()
		shift_factor = predict_nr & 0xf;
		predict_nr >>= 4;
		flags = in[0]; inc() // flags
		if (flags == 7 || flags == 5)
			break;             
		for (i = 0; i < 28; i += 2) 
		{
			d = in[0]; inc()
			s = (d & 0xf) << 12;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i] = (double)(s >> shift_factor);
			s = (d & 0xf0) << 8;
			if (s & 0x8000)
			s |= 0xffff0000;
			samples[i+1] = (double)(s >> shift_factor);
		}
		for ( i = 0; i < 28; i++ ) 
		{
			samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
			s_2 = s_1;
			s_1 = samples[i];
			d = (int)(samples[i] + 0.5);
			out[0] = d & 0xff;
			out[1] = d >> 8;
			out += 2;
		}
	}
	*outsize = size;
	#undef inc
}


/*
void VAG_Unpack(FILE *vag, FILE *pcm)			
{
    int predict_nr, shift_factor, flags;
    int i, d, s;
    static double s_1 = 0.0;
    static double s_2 = 0.0;
      
	// skip VAG header
	fseek(vag, 64, SEEK_SET );
	while(1)
	{
		predict_nr = fgetc( vag );
		shift_factor = predict_nr & 0xf;
		predict_nr >>= 4;
		flags = fgetc(vag); // flags
		if (flags == 7)
			break;             
		for (i = 0; i < 28; i += 2) 
		{
			d = fgetc(vag);
			s = (d & 0xf) << 12;
			if (s & 0x8000)
				s |= 0xffff0000;
			samples[i] = (double)(s >> shift_factor);
			s = (d & 0xf0) << 8;
			if (s & 0x8000)
			s |= 0xffff0000;
			samples[i+1] = (double)(s >> shift_factor);
		}
		for ( i = 0; i < 28; i++ ) 
		{
			samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
			s_2 = s_1;
			s_1 = samples[i];
			d = (int)(samples[i] + 0.5);
			fputc(d & 0xff, pcm);
			fputc(d >> 8, pcm);
		}
	}
}
*/