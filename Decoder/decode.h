//////////////////////////////////////////////////////////////////////////////
// * File name: decode.h
// *
// *
// * Description: Audio Decoder
// * Course: OAiS DSP1
// * Year: 2020
// *
// * Author: <TO DO - add student name and index>
// * Faculty of Technical Sciences, Novi Sad
// *
//////////////////////////////////////////////////////////////////////////////

#include "tistdtypes.h"

#ifndef DECODE_H_
#define DECODE_H_

// FRAME_SIZE = 2(kanala) * 2(dvostruka dužina okvira zbog preklopi-saberi) * AUDIO_IO_SIZE = 512
#define FRAME_SIZE (4*AUDIO_IO_SIZE)

void init_vorbis_window(Int16 N);

// TODO: Deklarisati funkciju za dekodovanje
void decode(Int16* in, Int16* outL, Int16* outR, Int16 N, Uint16 BL, Uint16 BR);

Int16 sign(Int16 broj);

#endif /* DECODE_H_ */
