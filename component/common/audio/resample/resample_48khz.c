/*
 * This file contains resampling functions between 48 kHz and nb/wb.
 * The description header can be found in resample_library.h
 *
 */
#include "resample_library.h"
#include <platform/platform_stdlib.h>
/**
  * @brief  48K -> 16K resampler.
  * @param  in: input, short  in[len]
  * @param  len: input length
  * @param  out: output, short out[len/3]
  * @param  state: 48K to 16K state
  * @param  temp: intermediate variable, int temp[len+16]
  * @note  len and len/3 must be integer
  * @retval None
  */
void Resample48khzTo16khz(const short* in, int len, short* out, RESAMPLE_STAT48TO16* state, int *temp)
{
	///// 48 --> 48(LP)   short --> int/////
	ResampleLPBy2ShortToInt(in, len, temp + 16, state->S_48_48);
	///// 48 --> 32  int --> int /////
	memcpy(temp + 8, state->S_48_32, 8 * sizeof(int));
	memcpy(state->S_48_32, temp + 8 + len, 8 * sizeof(int));
	Resample48khzTo32khz(temp + 8, temp, len/3);
	///// 32 --> 16  int --> short /////
	ResampleDownBy2IntToShort(temp, len/3*2, out, state->S_32_16);
}

// initialize state of 48 -> 16 resampler
void ResampleReset48khzTo16khz(RESAMPLE_STAT48TO16* state)
{
	memset(state->S_48_48, 0, 16 * sizeof(int));
	memset(state->S_48_32, 0, 8 * sizeof(int));
	memset(state->S_32_16, 0, 8 * sizeof(int));
}

/**
  * @brief  16K -> 48K resampler.
  * @param  in: input, short  in[len]
  * @param  len: input length
  * @param  out: output, short out[len*3]
  * @param  state: 16K to 48K state
  * @param  temp: intermediate variable, int temp[len*2+16]
  * @note  len and len/2 must be integer
  * @retval None
  */
void Resample16khzTo48khz(const short* in, int len, short* out, RESAMPLE_STAT16TO48* state, int* temp)
{
	///// 16 --> 32 /////
	ResampleUpBy2ShortToInt(in, len, temp + 16, state->S_16_32);
	///// 32 --> 24 /////
	memcpy(temp + 8, state->S_32_24, 8 * sizeof(int));
	memcpy(state->S_32_24, temp + 8 + len*2, 8 * sizeof(int));
	Resample32khzTo24khz(temp + 8, temp, len>>1);
	///// 24 --> 48 /////
	ResampleUpBy2IntToShort(temp, len/2*3, out, state->S_24_48);
}

// initialize state of 16 -> 48 resampler
void ResampleReset16khzTo48khz(RESAMPLE_STAT16TO48* state)
{
	memset(state->S_16_32, 0, 8 * sizeof(int));
	memset(state->S_32_24, 0, 8 * sizeof(int));
	memset(state->S_24_48, 0, 8 * sizeof(int));
}

/**
  * @brief  48K -> 8K resampler.
  * @param  in: input, short  in[len]
  * @param  len: input length
  * @param  out: output, short out[len/6]
  * @param  state: 48K to 8K state
  * @param  temp: intermediate variable, int temp[len+16]
  * @note  len and len/6 must be integer
  * @retval None
  */
void Resample48khzTo8khz(const short* in, int len, short* out, RESAMPLE_STAT48TO8* state, int* temp)
{
	///// 48 --> 24 /////
	ResampleDownBy2ShortToInt(in, len, temp + 16 + len/2, state->S_48_24);
	///// 24 --> 24(LP) /////
	ResampleLPBy2IntToInt(temp + 16 + len/2, len>>1, temp + 16, state->S_24_24);
	///// 24 --> 16 /////
	memcpy(temp + 8, state->S_24_16, 8 * sizeof(int));
	memcpy(state->S_24_16, temp + 8 + len/2, 8 * sizeof(int));
	Resample48khzTo32khz(temp + 8, temp, len/6);
	///// 16 --> 8 /////
	ResampleDownBy2IntToShort(temp, len/3, out, state->S_16_8);
}

// initialize state of 48 -> 8 resampler
void ResampleReset48khzTo8khz(RESAMPLE_STAT48TO8* state)
{
	memset(state->S_48_24, 0, 8 * sizeof(int));
	memset(state->S_24_24, 0, 16 * sizeof(int));
	memset(state->S_24_16, 0, 8 * sizeof(int));
	memset(state->S_16_8, 0, 8 * sizeof(int));
}

/**
  * @brief  8K -> 48K resampler.
  * @param  in: input, short  in[len]
  * @param  len: input length
  * @param  out: output, short out[len*6]
  * @param  state: 16K to 48K state
  * @param  temp: intermediate variable, int temp[len*5+24]
  * @note  len and len/2 must be integer
  * @retval None
  */
void Resample8khzTo48khz(const short* in, int len, short* out, RESAMPLE_STAT8TO48* state, int* temp)
{
    ///// 8 --> 16 /////
    ResampleUpBy2ShortToInt(in, len, temp + 24 + len*3, state->S_8_16);
    // copy state to and from input array
    memcpy(temp + 16 + len*3, state->S_16_12, 8 * sizeof(int));
    memcpy(state->S_16_12, temp + 16 + len*5, 8 * sizeof(int));
    Resample32khzTo24khz(temp + 16 + len*3, temp + len*3, len>>1);
    ///// 12 --> 24 /////
    ResampleUpBy2IntToInt(temp + len*3, len*3/2, temp, state->S_12_24);
    ///// 24 --> 48 /////
    ResampleUpBy2IntToShort(temp, len*3, out, state->S_24_48);
}

// initialize state of 8 -> 48 resampler
void ResampleReset8khzTo48khz(RESAMPLE_STAT8TO48* state)
{
	memset(state->S_8_16, 0, 8 * sizeof(int));
	memset(state->S_16_12, 0, 8 * sizeof(int));
	memset(state->S_12_24, 0, 8 * sizeof(int));
	memset(state->S_24_48, 0, 8 * sizeof(int));
}
