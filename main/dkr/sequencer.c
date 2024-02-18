/**
 ******************************************************************************
 * File Name          : sequencer.c
 * Author			  : Xavier Halgand
 * Date               :
 * Description        :
 ******************************************************************************
 */

/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*/

#include "sequencer.h"
#include <stdio.h>
#include "glo_config.h"
#include "hw/midi.h"
#include "hw/leds.h"

int NUMBER_STEPS = DKR_NUMBER_STEPS_DEFAULT;
int INIT_TEMPO = DKR_INIT_TEMPO_DEFAULT;

uint8_t seq_filled = 0;
uint8_t	dkr_midi_override = 0;

extern /*static*/ float		f0;// _CCM_ ;
extern /*static*/ float 	vol;// _CCM_;
extern /*static*/ bool 		autoFilterON;// _CCM_;
extern ADSR_t				adsr;// _CCM_;
extern ResonantFilter 		SVFilter;// _CCM_ ;

/*--------------------------------------------------------------------------------------------*/
Sequencer_t 		seq _CCM_;
NoteGenerator_t 	noteG _CCM_;
/*--------------------------------------------------------------------------------------------*/

void seq_gateTime_set(uint8_t val) // val is a number of samples
{
	seq.gateTime = seq.steptime * ((0.9f - 0.1f) * val / MIDI_MAX + 0.1f); // from 10% to 90% of each step duration
}
/*-------------------------------------------------------*/
void seq_transpUp(void) // one tone up
{
	if (noteG.rootNote < (MAX_NOTE_INDEX - 12))
	{
		noteG.transpose = 2;
	}
}
/*-------------------------------------------------------*/
void seq_transp(int8_t semitone, uint8_t val)
{
	if (val == MIDI_MAX)
	{
		noteG.transpose = semitone;
	}
}
/*-------------------------------------------------------*/
void seq_transpDown(void) // one tone down
{
	if (noteG.rootNote > FIRST_NOTE)
	{
		noteG.transpose = -2;
	}
}

/*-------------------------------------------------------*/
void seq_chooseScale(uint8_t idx)
{
	uint8_t		*currentScale ;

	printf("seq_chooseScale(%d)\n",idx);

	switch (idx)
	{
	case 0 : 	currentScale = (uint8_t*)MIDIscale13;	break ;
	case 1 :  	currentScale = (uint8_t*)MIDIscale14;	break;
	case 2 : 	currentScale = (uint8_t*)MIDIscale07;	break;
	case 3 : 	currentScale = (uint8_t*)MIDIscale08;	break;
	case 4 : 	currentScale = (uint8_t*)MIDIscale09;	break;
	case 5 : 	currentScale = (uint8_t*)MIDIscale10;	break;
	case 6 : 	currentScale = (uint8_t*)MIDIscale04;	break;
	case 7 : 	currentScale = (uint8_t*)MIDIscale01;	break;
	case 8 : 	currentScale = (uint8_t*)MIDIscale03;	break;
	case 9 : 	currentScale = (uint8_t*)MIDIscale11;	break;
	case 10 : 	currentScale = (uint8_t*)MIDIscale02;	break;
	case 11 : 	currentScale = (uint8_t*)MIDIscale06;	break;
	case 12 : 	currentScale = (uint8_t*)MIDIscale05;	break;
	case 13 : 	currentScale = (uint8_t*)MIDIscale12;	break;
	case 14 : 	currentScale = (uint8_t*)MIDIscale11;	break;
	default :	currentScale = (uint8_t*)MIDIscale11; break ;
	}
	noteG.currentScale = currentScale;
	noteG.chRequested = true;
}
/*-------------------------------------------------------*/
void seq_nextScale(void)
{
	if (noteG.scaleIndex < MAX_SCALE_INDEX)
	{
		noteG.scaleIndex++;
		seq_chooseScale(noteG.scaleIndex);
	}
}
/*-------------------------------------------------------*/
void seq_prevScale(void)
{
	if (noteG.scaleIndex > 0)
	{
		noteG.scaleIndex--;
		seq_chooseScale(noteG.scaleIndex);
	}
}
/*-------------------------------------------------------*/
void seq_scale_set(uint8_t val)
{
	noteG.scaleIndex = (uint8_t) rintf(MAX_SCALE_INDEX / MIDI_MAX * val);
	seq_chooseScale(noteG.scaleIndex);
}
/*-------------------------------------------------------*/
void seq_automatic_or_manual(void)
{
	if (noteG.automaticON) noteG.automaticON = false;
	else noteG.automaticON = true;
}
/*-------------------------------------------------------*/
void seq_switchMovingSeq(uint8_t val)
{
	if (val > 63)
		noteG.automaticON = true;
	else
		noteG.automaticON= false;
}
/*-------------------------------------------------------*/
void seq_toggleGlide(void)
{
	if (noteG.glideON) noteG.glideON = false;
	else noteG.glideON = true;
}
/*-------------------------------------------------------*/
void seq_switchGlide(uint8_t val)
{
	switch (val)
	{
	case MIDI_MAXi : 	noteG.glideON = true; break;
	case 0 : 	noteG.glideON = false; break;
	}
}
/*-------------------------------------------------------*/
void seq_muteSomeNotes(void)
{
	if (noteG.someNotesMuted) noteG.someNotesMuted = false;
	else noteG.someNotesMuted = true;
}
/*-------------------------------------------------------*/
void seq_switchMute(uint8_t val)
{
	switch (val)
	{
	case MIDI_MAXi : 	noteG.someNotesMuted = true; break;
	case 0 : 	noteG.someNotesMuted = false; break;
	}
}
/*-------------------------------------------------------*/
void seq_doubleTempo(void)
{
	//if (noteG.freq <= 5) noteG.freq *= 2;
}
/*-------------------------------------------------------*/
void seq_halfTempo(void)
{
	//if (noteG.freq >= .05f) noteG.freq *= 0.5f;
}
/*-------------------------------------------------------*/
void seq_incTempo(void)
{
	//if (noteG.freq <= 5) noteG.freq += 0.01f;
}

/*-------------------------------------------------------*/
void seq_decTempo(void)
{
	//if (noteG.freq >= .05f) noteG.freq -= 0.01f;
}
/*-------------------------------------------------------*/
void seq_incMaxFreq(void)
{
	if (noteG.octaveSpread < 8)
	{
		noteG.octaveSpread++;
		noteG.chRequested = true;
	}
}
/*-------------------------------------------------------*/
void seq_decMaxFreq(void)
{
	if (noteG.octaveSpread > 0)
	{
		noteG.octaveSpread--;
		noteG.chRequested = true;
	}
}
/*-------------------------------------------------------*/
void seq_freqMax_set(uint8_t val)
{
	noteG.octaveSpread = (uint8_t)(8/MIDI_MAX * val);
	noteG.chRequested = true;
}
/*-------------------------------------------------------*/
void seq_tempo_set(uint8_t val)
{
	seq.tempo = (float)(500.f * val / MIDI_MAX + 20); // unit : bpm
	seq.steptime = lrintf(Fs * 60 / seq.tempo);
}
/*--------------------------------------------------------------------------------------------*/
/*
#ifdef USE_FF_SEQUENCE
//15,27,39=C
//int seq1[NUMBER_STEPS] = {15,27,39,51,63,75,15,27,39,51,63,75,15,27,39,51,63,75,15,27,39,51,63,75,15,27,39,51,63,75,87,99};
//int seq1[NUMBER_STEPS] = {15,15,17,18,15,15,17,18,27,27,29,30,27,27,29,30,15,15,17,18,22,22,25,27,22,22,25,27,39,39,41,42};
//int seq1[NUMBER_STEPS] = {27,29,31,34,39,41,43,46,51,46,43,41,39,34,31,29,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx};

const int seq1[NUMBER_STEPS] = {

	27,29,31,34, //cdegc2
	39,41,43,46,
	51,53,55,58,
	63,65,67,70,
	75,70,67,65,
	63,58,55,53,
	51,46,43,41,
	39,34,31,29,

	//abcea2
	24,26,27,31,
	24+12,26+12,27+12,31+12,
	24+12*2,26+12*2,27+12*2,31+12*2,
	24+12*3,26+12*3,27+12*3,31+12*3,
	31+12*3+5,31+12*3,27+12*3,26+12*3,
	24+12*3,31+12*2,27+12*2,26+12*2,
	24+12*2,31+12,27+12,26+12,
	24+12,31,27,26,

	27,29,31,34, //cdegc2 ..............x2
	39,41,43,46,
	51,53,55,58,
	63,65,67,70,
	75,70,67,65,
	63,58,55,53,
	51,46,43,41,
	39,34,31,29,

	//abcea2 ..............x2
	24,26,27,31,
	24+12,26+12,27+12,31+12,
	24+12*2,26+12*2,27+12*2,31+12*2,
	24+12*3,26+12*3,27+12*3,31+12*3,
	31+12*3+5,31+12*3,27+12*3,26+12*3,
	24+12*3,31+12*2,27+12*2,26+12*2,
	24+12*2,31+12,27+12,26+12,
	24+12,31,27,26,

	//d#fga#d2# +2semi -> fgacf2
	24-4,26-4,28-4,31-4,
	24+12-4,26+12-4,28+12-4,31+12-4,
	24+12*2-4,26+12*2-4,28+12*2-4,31+12*2-4,
	24+12*3-4,26+12*3-4,28+12*3-4,31+12*3-4,
	31+12*3+5-4,31+12*3-4,28+12*3-4,26+12*3-4,
	24+12*3-4,31+12*2-4,28+12*2-4,26+12*2-4,
	24+12*2-4,31+12-4,28+12-4,26+12-4,
	24+12-4,31-4,28-4,26-4,

	// -> gabdg2
	24-2,26-2,28-2,31-2,
	24+12-2,26+12-2,28+12-2,31+12-2,
	24+12*2-2,26+12*2-2,28+12*2-2,31+12*2-2,
	24+12*3-2,26+12*3-2,28+12*3-2,31+12*3-2,
	31+12*3+5-2,31+12*3-2,28+12*3-2,26+12*3-2,
	24+12*3-2,31+12*2-2,28+12*2-2,26+12*2-2,
	24+12*2-2,31+12-2,28+12-2,26+12-2,
	24+12-2,31-2,28-2,26-2,

	//f#a#c#f +2semi -> g#cd#g
	23,27,30,34,
	23+12,27+12,30+12,34+12,
	23+12*2,27+12*2,30+12*2,34+12*2,
	23+12*3,27+12*3,30+12*3,34+12*3,
	35+12*3,34+12*3,30+12*3,27+12*3,
	23+12*3,34+12*2,30+12*2,27+12*2,
	23+12*2,34+12,30+12,27+12,
	23+12,34,30,27,

	//g#cd#g +2semi -> a#dfa
	25,29,32,36,
	25+12,29+12,32+12,36+12,
	25+12*2,29+12*2,32+12*2,36+12*2,
	25+12*3,29+12*3,32+12*3,36+12*3,
	37+12*3,36+12*3,32+12*3,29+12*3,
	25+12*3,36+12*2,32+12*2,29+12*2,
	25+12*2,36+12,32+12,29+12,
	25+12,36,32,29
};
#endif
*/

void seq_sequence_new(void)
{
	if(seq_filled)
	{
		return;
	}

	printf("seq_sequence_new()\n");

	//NUMBER_STEPS = load_dekrispator_sequence(1, seq.track1.note, &INIT_TEMPO);
	//printf("seq_sequence_new(): sequence %d loaded, found %d notes, tempo = %d\n", 1, NUMBER_STEPS, INIT_TEMPO);

	#ifdef USE_FF_SEQUENCE
	for(uint16_t i=0; i < NUMBER_STEPS; i++)
	{
		seq.track1.note[i] = seq1[i]; //c as per https://musescore.com/user/205967/scores/373116
		//seq.track1.note[i] = seq1[i]-2; //c -> a#
	}
	#else
	int16_t relativeNote;
	int16_t octaveShift;
	int16_t index;

	for (uint16_t i = 0; i < NUMBER_STEPS; i++)
	{
		relativeNote = noteG.currentScale[lrintf(frand_a_b(1 , noteG.currentScale[0]))];
		octaveShift = 12 * lrintf(frand_a_b(0 , noteG.octaveSpread));
		index = noteG.rootNote + octaveShift + relativeNote - FIRST_NOTE;

		//printf("seq_sequence_new(): i=%d, relativeNote=%d, octaveShift=%d, index=%d\n", i, relativeNote, octaveShift, index);

		while (index > MAX_NOTE_INDEX) index -= 12;
		while (index < 0) index += 12;
		seq.track1.note[i] = index; // note frequency is in notesFreq[index]
	}
	#endif

	printf("seq_sequence_new(): ");
	for(uint16_t i = 0; i < NUMBER_STEPS; i++)
	{
		printf("%d(%f),",seq.track1.note[i],notesFreq[seq.track1.note[i]]);
	}
	printf("\n");

	seq_filled = 1;
}
/*--------------------------------------------------------------------------------------------*/
void seq_transpose(void)
{
	int16_t  noteIndex;

	for (uint16_t i = 0; i < NUMBER_STEPS; i++)
	{
		noteIndex = seq.track1.note[i] + noteG.transpose;
		while (noteIndex > MAX_NOTE_INDEX) noteIndex -= 12;
		while (noteIndex < 0) noteIndex += 12;
		seq.track1.note[i] =  noteIndex;
	}
	noteG.transpose = 0;
}
/*--------------------------------------------------------------------------------------------*/
void sequencer_init(void)
{
	printf("sequencer_init()\n");

	if(seq.track1.note==NULL)
	{
		seq.track1.note = (uint8_t*)malloc(DKR_MAX_SEQUENCE);
	}

	seq.tempo = INIT_TEMPO;

	seq.steptime = lrintf(Fs * 60 / seq.tempo);
	seq.smp_count = 0;
	seq.step_idx = 0;
	seq.gateTime = 0.5f * seq.steptime ;

	noteG.transpose = 0;
	noteG.automaticON = false;
	noteG.glideON = false;
	noteG.chRequested = false;
	noteG.someNotesMuted = false;
	noteG.scaleIndex = 0;
	noteG.octaveSpread = 4;
	noteG.rootNote = 36;
	noteG.currentScale = (uint8_t*)MIDIscale13;

	seq_sequence_new();
}
/*--------------------------------------------------------------------------------------------*/
void sequencer_process(void) // To be called at each sample treatment
{
	if(!dkr_midi_override)
	{
		if(MIDI_keys_pressed)
		{
			printf("MIDI active, will receive chords\n");
			dkr_midi_override = 1;
		}
		else
		{
			/* If we have reached a new step ....  */
			if (seq.smp_count-- <= 0)
			{
				/* If we are at the begining of a new sequence .... */
				if (seq.step_idx == 0)
				{
					//sequencer_newSequence_action();
				}
				sequencer_newStep_action();

				seq.smp_count = seq.steptime; // reload the counter
				seq.step_idx++;
				if(seq.step_idx >= NUMBER_STEPS) seq.step_idx = 0;
			}
		}
	}
	else
	{
		if(MIDI_notes_updated)
		{
			//printf("MIDI_notes_updated\n");
			MIDI_notes_updated = 0;

			LED_W8_all_OFF();
			LED_B5_all_OFF();

			if(MIDI_keys_pressed)
			{
				//printf("MIDI keys pressed, ADSR key On;\n");
				MIDI_to_LED(MIDI_last_chord[0], 1);
				ADSR_keyOn(&adsr);

				f0 = notesFreq[MIDI_last_chord[0] + 2];
				vol = frand_a_b(0.4f , .8f); // slightly random volume for each note

				//if (autoFilterON) SVF_directSetFilterValue(&SVFilter, Ts * 600.f * powf(5000.f / 600.f, frand_a_b(0 , 1)));
			}
			else
			{
				//printf("no MIDI keys pressed, ADSR key Off;\n");
				ADSR_keyOff(&adsr);
			}
		}
	}
}
/*--------------------------------------------------------------------------------------------*/
