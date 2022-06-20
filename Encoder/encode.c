//////////////////////////////////////////////////////////////////////////////
// * File name: encode.c
// *
// *
// * Description: Audio Decoder
// * Course: OAiS DSP1
// * Year: 2021
// *
// * Author: <Pavle Vukovic RA135/2019>
// * Faculty of Technical Sciences, Novi Sad
// *
//////////////////////////////////////////////////////////////////////////////
#include "encode.h"

#include "Dsplib.h"
#include "ezdsp5535_aic3204_dma.h"
#include <math.h>
#include "quant.h"


#define PI 3.1415926535
#define FFT_SIZE (2*AUDIO_IO_SIZE)

#pragma DATA_ALIGN(fft_bufferL,4)
#pragma DATA_ALIGN(fft_bufferR,4)
static Int16 fft_bufferL[FFT_SIZE];
static Int16 fft_bufferR[FFT_SIZE];

static Int16 in_delayL[FFT_SIZE/2];
static Int16 in_delayR[FFT_SIZE/2];

Int16 window[FFT_SIZE];


void init_vorbis_window(Int16 N)
{
  int i;
  double t;

  for (i = 0; i < N; i++) {
    t = sin(0.5 * PI * (i + 0.5) / N);
    window[i] = 32767 * sin(0.5 * PI * t * t);
    window[2*N-1-i] = window[i];
  }
}

Int16 sign(Int16 broj){
	if(broj < 0){
		return -1;
	}else return 1;
}

Int16 znakL[FFT_SIZE];
Int16 znakR[FFT_SIZE];
Int16 xL[FFT_SIZE];
Int16 xR[FFT_SIZE];

// TODO: Realizovati funkciju za enkodovanje i unutar nje uraditi analizu i kvantizaciju za svaki kanal ponaosob

void encode(Int16* inL, Int16* inR, Int16* out, Int16 N, Uint16 BL, Uint16 BR){
	Int16 i;

	//*******************************************************************************************************************************************************//
	//FFT

	//Za LEVI kanal

	for(i = 0; i < N; i++){
		fft_bufferL[i] = in_delayL[i];
		fft_bufferL[i + N] = inL[i];
		in_delayL[i] = inL[i];
	}

	for(i = 0; i < 2*N; i++){
		fft_bufferL[i] = _smpy(fft_bufferL[i], window[i]);
	}

	rfft(fft_bufferL, FFT_SIZE, SCALE);



	//Za DESNI kanal

	for(i = 0; i < N; i++){
		fft_bufferR[i] = in_delayR[i];
		fft_bufferR[i + N] = inR[i];
		in_delayR[i] = inR[i];
	}

	for(i = 0; i < 2*N; i++){
		fft_bufferR[i] = _smpy(fft_bufferR[i], window[i]);
	}

	rfft(fft_bufferR, FFT_SIZE, SCALE);
	//*******************************************************************************************************************************************************//



	//*******************************************************************************************************************************************************//
	//NELINEARNA OBRADA
	for(i = 0; i < 2*N; i++){									//potpuno nepotrebna for petlja, ali je intuitivnije raditi sa x nizovima!!!
		xL[i] = fft_bufferL[i];
		xR[i] = fft_bufferR[i];
	}

	//ZNAKOVI
	for(i = 0; i < 2*N; i++){
		znakL[i] = sign(xL[i]);
		znakR[i] = sign(xR[i]);
	}

	for(i = 0; i < 2*N; i++){									//pretvaranje negativnog broja u pozitivan, zbog korenovanja
		if(znakL[i] == -1){
			xL[i] *= (-1);
		}

		if(znakR[i] == -1){
			xR[i] *= (-1);
		}
	}


	//sad su svi pozitivni

	//KORENI
	sqrt_16(xL, xL, FFT_SIZE);									//sqrt(xL)
	sqrt_16(xR, xR, FFT_SIZE);									//sqrt(xR)

	//Za LEVI kanal
	for(i = 0; i < 2*N; i++){
		xL[i] = znakL[i] * xL[i];
		fft_bufferL[i] = xL[i];
	}

	//Za DESNI kanal
	for(i = 0; i < 2*N; i++){
		xR[i] = znakR[i] * xR[i];
		fft_bufferR[i] = xR[i];
	}
	//*******************************************************************************************************************************************************//



	//*******************************************************************************************************************************************************//
	//KVANTIZACIJA

	if(BL == 0 || BR == 0){											//poseban vid kvantizacije
		Int16 k1 = round((double)FFT_SIZE / 48000 * 100);			//koeficijent za 1. opseg, 1
		Int16 k2 = round((double)FFT_SIZE / 48000 * 512);			//koeficijent za 2. opseg, 3
		Int16 k3 = round((double)FFT_SIZE / 48000 * 4096);			//koeficijent za 3. opseg, 22
		Int16 k4 = round((double)FFT_SIZE / 48000 * 14336);			//koeficijent za 4. opseg, 76

		for(i = 0; i < 2*N; i++){
			if(i < k1){
				fft_bufferL[i] = 0;
				fft_bufferR[i] = 0;
			}else if(i < k2){
				fft_bufferL[i] = quantB(fft_bufferL[i], (Uint16)8);
				fft_bufferR[i] = quantB(fft_bufferR[i], (Uint16)8);
			}else if(i < k3){
				fft_bufferL[i] = quantB(fft_bufferL[i], (Uint16)12);
				fft_bufferR[i] = quantB(fft_bufferR[i], (Uint16)12);
			}else if(i < k4){
				fft_bufferL[i] = quantB(fft_bufferL[i], (Uint16)6);
				fft_bufferR[i] = quantB(fft_bufferR[i], (Uint16)6);
			}else{
				fft_bufferL[i] = 0;
				fft_bufferR[i] = 0;
			}
		}
	}else{

		//Za LEVI kanal

		for(i = 0; i < 2*N; i++){
			fft_bufferL[i] = quantB(fft_bufferL[i], BL);
		}



		//Za DESNI kanal

		for(i = 0; i < 2*N; i++){
			fft_bufferR[i] = quantB(fft_bufferR[i], BR);
		}
	}
	//*******************************************************************************************************************************************************//



	//*******************************************************************************************************************************************************//
	//UCESLJAVANJE

	for(i = 0; i < 2*N; i++){
		out[2*i] = fft_bufferL[i];
		out[2*i + 1] = fft_bufferR[i];
	}
	//*******************************************************************************************************************************************************//
}
