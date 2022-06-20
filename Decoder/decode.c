//////////////////////////////////////////////////////////////////////////////
// * File name: decode.c
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

#include "decode.h"

#include "Dsplib.h"
#include "ezdsp5535_aic3204_dma.h"
#include <math.h>
#include "quant.h"


#define PI 3.1415926535
#define FFT_SIZE (2*AUDIO_IO_SIZE)

//Niz koji ce biti prosledjen funkciji rifft mora biti poravnat na 4.
#pragma DATA_ALIGN(fft_buffer,4)
 Int16 fft_bufferL[FFT_SIZE];
 Int16 fft_bufferR[FFT_SIZE];

 static Int16 out_delayL[FFT_SIZE/2];
 static Int16 out_delayR[FFT_SIZE/2];

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

Int16 znakL[FFT_SIZE];
Int16 znakR[FFT_SIZE];
Int16 xL[FFT_SIZE];
Int16 xR[FFT_SIZE];

// TODO: Realizovati funkciju za dekodovanje i unutar nje uraditi vracanje u opseg i sintezu za svaki kanal ponaosob.
void decode(Int16* in, Int16* outL, Int16* outR, Int16 N, Uint16 BL, Uint16 BR){
	int i;
	//*******************************************************************************************************************************************************//
	//VRACANJE U OPSEG, REKONSTRUKCIJA

	for(i = 0; i < 2*N; i++){
		fft_bufferL[i] = in[2*i];										//ucitavanje okvira podataka za levi kanal
	}

	for(i = 0; i < 2*N; i++){
		fft_bufferR[i] = in[2*i + 1];										//ucitavanje okvira podataka za levi kanal
	}

	if(BL == 0 || BR == 0){													//poseban vid kvantizacije
		Int16 k1 = round((double)FFT_SIZE / 48000 * 100);					//koeficijent za 1. opseg, 1
		Int16 k2 = round((double)FFT_SIZE / 48000 * 512);					//koeficijent za 2. opseg, 3
		Int16 k3 = round((double)FFT_SIZE / 48000 * 4096);					//koeficijent za 3. opseg, 22
		Int16 k4 = round((double)FFT_SIZE / 48000 * 14336);					//koeficijent za 4. opseg, 76

		for(i = 0; i < 2*N; i++){
			if(i < k1){
				fft_bufferL[i] = 0;
				fft_bufferR[i] = 0;
			}
			else if(i < k2){
				fft_bufferL[i] = reconstructB(fft_bufferL[i], (Uint16)8);
				fft_bufferR[i] = reconstructB(fft_bufferR[i], (Uint16)8);
			}else if(i < k3){
				fft_bufferL[i] = reconstructB(fft_bufferL[i], (Uint16)12);
				fft_bufferR[i] = reconstructB(fft_bufferR[i], (Uint16)12);
			}else if(i < k4){
				fft_bufferL[i] = reconstructB(fft_bufferL[i], (Uint16)6);
				fft_bufferR[i] = reconstructB(fft_bufferR[i], (Uint16)6);
			}else{
				fft_bufferL[i] = 0;
				fft_bufferR[i] = 0;
			}
		}
	}else{

		//*******************************************************************************************************************************************************//
		//VRACANJE U OPSEG, REKONSTRUKCIJA

		//Za LEVI kanal

		for(i = 0; i < 2*N; i++){
			fft_bufferL[i] = reconstructB(fft_bufferL[i], BL);				//rekonstruisanje
		}

		//Za DESNI kanal

		for(i = 0; i < 2*N; i++){
			fft_bufferR[i] = reconstructB(fft_bufferR[i], BR);				//rekonstruisanje
		}
	}
	//*******************************************************************************************************************************************************//



	//*******************************************************************************************************************************************************//
	//NELINEARNA OBRADA

	//x = sign(x) * sqrt(x);									encoder
	//x = sign(x) * pow(x, 2);									decoder

	for(i = 0; i < 2*N; i++){									//potpuno nepotrebna for petlja, ali je intuitivnije raditi sa xL nizom!!!
		xL[i] = fft_bufferL[i];
	}

	for(i = 0; i < 2*N; i++){									//potpuno nepotrebna for petlja, ali je intuitivnije raditi sa xR nizom!!!
		xR[i] = fft_bufferR[i];
	}

	for(i = 0; i < 2*N; i++){									//popunjavanje nizova znakL i znakR
		znakL[i] = sign(xL[i]);
		znakR[i] = sign(xR[i]);
	}

	//Za LEVI kanal

	for(i = 0; i < 2*N; i++){									//popunjavanje nizova znakL i znakR
		xL[i] = _smpy(xL[i], xL[i]);							//pow(xL, 2);
		xL[i] = znakL[i] * xL[i];
		fft_bufferL[i] = xL[i];
	}

	//Za DESNI kanal

	for(i = 0; i < 2*N; i++){									//popunjavanje nizova znakL i znakR
		xR[i] = _smpy(xR[i], xR[i]);							//pow(xL, 2);
		xR[i] = znakR[i] * xR[i];
		fft_bufferR[i] = xR[i];
	}
	//*******************************************************************************************************************************************************//



	//*******************************************************************************************************************************************************//
	//INVERZNA FFT
	//Za LEVI kanal

	rifft(fft_bufferL, FFT_SIZE, NOSCALE);

	//ovde treba breakpoint LEVI

	for(i = 0; i < N; i++){
		outL[i] = _smpy(fft_bufferL[i], window[i]) + _smpy(out_delayL[i], window[i + N]);
		outL[i] = outL[i] * 4;
		out_delayL[i] = fft_bufferL[i + N];													//out_delayL cuva drugih N odbiraka fft_buffer-a, za sledecu iteraciju
	}

	//Za DESNI kanal

	rifft(fft_bufferR, FFT_SIZE, NOSCALE);

	for(i = 0; i < N; i++){
		outR[i] = _smpy(fft_bufferR[i], window[i]) + _smpy(out_delayR[i], window[i + N]);
		outR[i] = outR[i] * 4;
		out_delayR[i] = fft_bufferR[i + N];													//out_delayR cuva drugih N odbiraka fft_buffer-a, za sledecu iteraciju
	}
	//*******************************************************************************************************************************************************//
}

Int16 sign(Int16 broj){
	if(broj < 0){
		return -1;
	}else return 1;
}
