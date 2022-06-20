//////////////////////////////////////////////////////////////////////////////
// * File name: main.c
// *
// *
// * Description: Audio Encoder
// * Course: OAiS DSP1
// * Year: 2021
// *
// * Author: <Pavle Vukovic RA135/2019>
// * Faculty of Technical Sciences, Novi Sad
// *
//////////////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2c.h"
#include "aic3204.h"
#include "ezdsp5535_aic3204_dma.h"
#include "ezdsp5535_i2s.h"
#include "Dsplib.h"
#include "encode.h"
#include "WAVheader.h"
#include "DSPHeader.h"
#include "quant.h"
#include "encode.h"

static WAV_HEADER inputWAVhdr;
static DSP_HEADER outputHeader = {{'D','S','P','1'}, 0, 8, 0, 0};					//MS je na pocetku 0, kasnije se menja u zavisnosti od unosa korisnika

#define SAMPLE_RATE 48000L
#define GAIN 0

#pragma DATA_ALIGN(InputBufferL,4)
Int16 InputBufferL[AUDIO_IO_SIZE];
#pragma DATA_ALIGN(InputBufferR,4)
Int16 InputBufferR[AUDIO_IO_SIZE];

#pragma DATA_ALIGN(M,4)
Int16 M[AUDIO_IO_SIZE];
#pragma DATA_ALIGN(S,4)
Int16 S[AUDIO_IO_SIZE];

#pragma DATA_ALIGN(CompressedBuffer,4)
Int16 CompressedBuffer[FRAME_SIZE];

int kodovanje;

/*
 *
 *  main( )
 *
 */
void main( void )
 {
	Int32 i;
	Int32 j;
	Int32 number_of_blocks;

    /* Initialize BSL */
    EZDSP5535_init( );

    printf("\n OAiS DSP 1 - Audio Encoder \n");
		
    /* Initialise hardware interface and I2C for code */
    aic3204_hardware_init();

    aic3204_set_input_filename("../15.wav");
	aic3204_set_output_filename("../out_encoder.dsp1");								//namenjeno je za huffmanEnc
	
	printf("Izaberite opciju kodovanja:\n");
	printf("Ako zelite konvencionalno kodovanje, unesite 1:\n");
	printf("Ako zelite zdruzeno kodovanje unesite 2:\n");
	scanf("%d", &kodovanje);

    /* Initialise the AIC3204 codec */
	aic3204_init();
          
    aic3204_dma_init();
    
    aic3204_read_wav_header(&inputWAVhdr);

    number_of_blocks = inputWAVhdr.data.SubChunk2Size/
    		(inputWAVhdr.fmt.NumChannels*inputWAVhdr.fmt.BitsPerSample*AUDIO_IO_SIZE/8);

    outputHeader.size = number_of_blocks*FRAME_SIZE*2;

    //TODO Postaviti preostale parametre zaglavlja
    Uint16 BL = outputHeader.quantBL;
    Uint16 BR = outputHeader.quantBR;
    Uint16 MS;

    //ovo moze lepse preko do while
    if(kodovanje == 1){
    	MS = outputHeader.MS;
    }else if(kodovanje == 2){
    	outputHeader.MS = 1;
    	MS = outputHeader.MS;
    }else{
    	printf("Unet je pogresan parametar!");
    	return;
    }

    aic3204_write_dsp1_header(&outputHeader);

    init_vorbis_window(AUDIO_IO_SIZE);

    for(i = 0; i < number_of_blocks; i++)
	{
		aic3204_read_block(InputBufferL, InputBufferR);
		
		if(MS == 1){
			for(j = 0; j < AUDIO_IO_SIZE; j++)
			{
				M[j] = (InputBufferL[j] + InputBufferR[j])/2;
				S[j] = (InputBufferL[j] - InputBufferR[j])/2;
				InputBufferL[j] = M[j];
				InputBufferR[j] = S[j];
			}
		}

		encode(InputBufferL, InputBufferR, CompressedBuffer, AUDIO_IO_SIZE, BL, BR);

		aic3204_write_data(CompressedBuffer, FRAME_SIZE);											//formira se zaglavlje izlazne datoteke, i izlazna datoteka out_encoder.dsp1
	}

	/* Disable I2S and put codec into reset */ 
    aic3204_disable();

    printf( "\n***Program has Terminated***\n" );
	SW_BREAKPOINT;
}

