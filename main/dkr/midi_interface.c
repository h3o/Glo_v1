/**
 ******************************************************************************
 * File Name          	: midi_interface.c
 * Author				: Xavier Halgand
 * Date               	:
 * Description        	:
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

#include "midi_interface.h"
#include "Dekrispator.h"
#include "glo_config.h"

#include <stdio.h>

/*-----------------------------------------------------------------------------*/
void MagicFX(uint8_t val) /* random effects parameters */
{
	//dekrispator_patch_t patch;
	//int mf_params[MF_PARAMS];
	int *mf_params = dkr_patch->fx;
	int all_zeros = 1;

	if (val == MIDI_MAXi+2)
	{
		printf("MagicFX(%d): loaded params = ", val);
		for(int i=0;i<MF_PARAMS;i++)
		{
			printf("%d,",mf_params[i]);
			if(mf_params[i])
			{
				all_zeros = 0;
			}
		}
		printf("\n");
		if(all_zeros)
		{
			printf("MagicFX(%d): will not load, as all parameters are zero\n", val);
			return;
		}
	}
	else if (val == MIDI_MAXi)
	{
		printf("MagicFX(%d): new params = ", val);
		for(int i=0;i<MF_PARAMS;i++)
		{
			mf_params[i] = MIDIrandVal();
			printf("%d,",mf_params[i]);
		}
		printf("\n");
	}
	else //load from config
	{
		//int found =
		load_dekrispator_patch(val, dkr_patch);
		//printf("MagicFX(%d): loaded from config, found = %d\n", val, found);
		printf("MagicFX(%d): params = ", val);
		for(int i=0;i<MF_PARAMS;i++)
		{
			printf("%d,",dkr_patch->fx[i]);
		}
		printf("\n");
	}

	//if (val == MIDI_MAX)
	{
		/*
		Delay_switch(MIDI_MAXi);
		Delay_time_set(MIDIrandVal());
		DelayWet_set(MIDIrandVal());
		DelayFeedback_set(MIDIrandVal());
		*/

		Chorus_switch(MIDI_MAXi);
		ChorusRate_set(mf_params[0]);//MIDIrandVal());
		ChorusSecondRate_set(mf_params[1]);//MIDIrandVal());
		ChorusDelay_set(mf_params[2]);//MIDIrandVal());
		ChorusSweep_set(mf_params[3]);//MIDIrandVal());
		ChorusFeedback_set(mf_params[4]);//MIDIrandVal());
		ChorusMode_switch(mf_params[5]);//MIDIrandVal());
		ChorusFDBsign_switch(mf_params[6]);//MIDIrandVal());

		Phaser_switch(MIDI_MAXi);
		Phaser_Rate_set(mf_params[7]);//MIDIrandVal());
		Phaser_Feedback_set(mf_params[8]);//MIDIrandVal());
		Phaser_Wet_set(mf_params[9]);//MIDIrandVal());
	}
}
/*-----------------------------------------------------------------------------*/
void MagicPatch(uint8_t val) /* random sound parameters */
{
	//dekrispator_patch_t patch;
	//int mp_params[MP_PARAMS];
	int *mp_params = dkr_patch->patch;
	int all_zeros = 1;

	if (val == MIDI_MAXi+2)
	{
		printf("MagicPatch(%d): loaded params = ", val);
		for(int i=0;i<MP_PARAMS;i++)
		{
			printf("%d,",mp_params[i]);
			if(mp_params[i])
			{
				all_zeros = 0;
			}
		}
		printf("\n");
		if(all_zeros)
		{
			printf("MagicPatch(%d): will not load, as all parameters are zero\n", val);
			return;
		}
	}
	else if (val == MIDI_MAXi)
	{
		printf("MagicPatch(%d): new params = ", val);
		for(int i=0;i<MP_PARAMS;i++)
		{
			mp_params[i] = MIDIrandVal();
			printf("%d,",mp_params[i]);
		}
		printf("\n");
	}
	else //load from config
	{
		//int found =
		load_dekrispator_patch(val, dkr_patch);
		//printf("MagicPatch(%d): loaded from config, found = %d\n", val, found);

		printf("MagicPatch(%d): params = ", val);
		for(int i=0;i<MP_PARAMS;i++)
		{
			printf("%d,",dkr_patch->patch[i]);
		}
		printf("\n");
	}

	//if (val == MIDI_MAX)
	{
		//seq_tempo_set(mp_params[0]);//MIDIrandVal());
		seq_freqMax_set(mp_params[1]);//MIDIrandVal());
		//seq_scale_set(mp_params[2]);//MIDIrandVal());
		seq_switchMovingSeq(mp_params[3]);//MIDIrandVal());
		//seq_switchMute(mp_params[4]);//MIDIrandVal());
		//seq_gateTime_set(mp_params[5]);//MIDIrandVal());
		autoSound_set(mp_params[6]%3);//rand()%3);
		Sound_set(mp_params[7]);//MIDIrandVal());
		uint8_t snd = soundNumber_get();

		if (snd == FM2)
		{
			//STM_EVAL_LEDOn(LED_Blue);
			FM_OP1_freq_set(mp_params[8]);//MIDIrandVal());
			FM_OP1_modInd_set(mp_params[9]);//MIDIrandVal());
			FM_OP2_freq_set(mp_params[10]);//MIDIrandVal());
			FM_OP2_modInd_set(mp_params[11]);//MIDIrandVal());
			FM_OP3_freq_set(mp_params[12]);//MIDIrandVal());
			FM_OP3_modInd_set(mp_params[13]);//MIDIrandVal());
			FM_OP4_freq_set(mp_params[14]);//MIDIrandVal());
			FM_OP4_modInd_set(mp_params[15]);//MIDIrandVal());
		}
		else if (snd == DRIFTERS)
		{
			//STM_EVAL_LEDOn(LED_Green);
			Drifter_amp_set(mp_params[16]);//MIDIrandVal());
			Drifter_minFreq_set(mp_params[17]);//MIDIrandVal());
			Drifter_maxFreq_set(mp_params[18]);//MIDIrandVal());
			Drifter_centralFreq_set(mp_params[19]);//MIDIrandVal());
		}

		Filter1Freq_set(mp_params[20]);//MIDIrandVal());
		Filter1Res_set(mp_params[21]);//MIDIrandVal());
		Filter1Drive_set(mp_params[22]);//MIDIrandVal());
		Filter1Type_set(mp_params[23]);//MIDIrandVal());
		Filt1LFO_amp_set(mp_params[24]);//MIDIrandVal());
		Filt1LFO_freq_set(mp_params[25]);//MIDIrandVal());

		Filter2Freq_set(mp_params[26]);//MIDIrandVal());
		Filter2Res_set(mp_params[27]);//MIDIrandVal());
		Filter2Drive_set(mp_params[28]);//MIDIrandVal());
		Filter2Type_set(mp_params[29]);//MIDIrandVal());
		Filt2LFO_amp_set(mp_params[30]);//MIDIrandVal());
		Filt2LFO_freq_set(mp_params[31]);//MIDIrandVal());

		Filter_Random_switch(mp_params[32]);//MIDIrandVal());

		AttTime_set(mp_params[33]%(MIDI_MAXi/10));//(uint8_t)lrintf(frand_a_b(0 , MIDI_MAX/10)));
		DecTime_set(mp_params[34]);//MIDIrandVal());
		SustLevel_set(mp_params[35]);//MIDIrandVal());
		RelTime_set(mp_params[36]);//MIDIrandVal());

		VibratoAmp_set(mp_params[37]);//MIDIrandVal());
		VibratoFreq_set(mp_params[38]);//MIDIrandVal());

		AmpLFO_amp_set(mp_params[39]);//MIDIrandVal());
		AmpLFO_freq_set(mp_params[40]);//MIDIrandVal());

	}
}
/*******************************************************************************
 * This function is called by USBH_MIDI_Handle(...) from file "usbh_midi_core.c"
 *
 * To fix : The first 4 bytes only of the data buffer  are interpreted, but there might be
 * several MIDI events in that 64 bytes buffer....
 *
 * *****************************************************************************/
//void MIDI_Decode(uint8_t * outBuf)
//{
	/*
	uint8_t val = 0;

	if (outBuf[1] != 0x00) start_LED_On(LED_Blue, 8);

	//If the midi message is a Control Change...
	if ((outBuf[1] & 0xF0) == 0xB0)
	{
		val = outBuf[3];

		switch(outBuf[2])
		{
		case 3 	: 	seq_tempo_set(val); 		break ;	// tempo
		case 13 : 	Volume_set(val); 			break;	// master volume
		case 31 :	SynthOut_switch(val); 		break;  // toggle synth output
		case 4  : 	seq_freqMax_set(val);		break;	// max frequency

		case 67 :	DemoMode_toggle(val);		break;
		//case 76 :	DemoMode_freeze(val);		break;

		case 39 :	MagicPatch(val);			break;	// random settings except effects
		case 38 :	MagicFX(val);				break;	// random settings for effects
		case 82 :	MagicPatch(val);			break;	//
		case 81 :	MagicFX(val);				break;	//

		case 5 :	seq_scale_set(val);			break; 	// scale
		case 6 :	Sound_set(val);				break;	// waveform
		case 76 :	RandSound1(val);			break;
		case 77 :	RandSound2(val);			break;

		case 8 :	Filter1Freq_set(val);		break;	//
		case 9 :	Filter1Res_set(val);		break;	//
		case 53 :	Filter1Freq_set(val);		break;	//
		case 54 :	Filter1Res_set(val);		break;	//
		case 12 :	Filter1Drive_set(val);		break;	//
		case 55 :	Filter1Drive_set(val);		break;	//
		case 56 :	Filter1Type_set(val);		break;	//

		case 23 :	Delay_switch(val);			break;	// Delay ON/OFF
		case 14 :	Delay_time_set(val);		break;	// Delay time
		case 15 :	DelayFeedback_set(val);		break;	// Delay feedback
		case 2 :	DelayWet_set(val);			break;	// Delay wet signal amplitude
		case 40 :	Delay_time_dec(val);		break;
		case 41 :	Delay_time_inc(val);		break;

		case 16 :	VibratoAmp_set(val);		break;	// Vibrato amplitude
		case 17 :	VibratoFreq_set(val);		break;	// Vibrato frequency

		case 27 :	Filter_Random_switch(val);	break;	// Filter ON/OFF
		case 63 :	SynthOut_amp_set(val);		break;	// Distorsion

		case 28 :	Chorus_switch(val);			break;	// Chorus ON/OFF
		case 37 :	Chorus_reset(val);			break;
		case 18 :	ChorusRate_set(val);		break;	// Chorus rate
		case 22 :	ChorusSecondRate_set(val);	break;	// Chorus relative rate for LFO right
		case 19 :	ChorusDelay_set(val);		break;	// Chorus delay
		case 20 :	ChorusSweep_set(val);		break;	// Chorus sweep
		case 21 :	ChorusFeedback_set(val);	break;	// Chorus feedback
		case 29 :	ChorusMode_switch(val);		break;	// Chorus mode
		case 30 :	ChorusFDBsign_switch(val);	break;	// Chorus fdb sign

		case 24 :	Phaser_switch(val);			break;
		case 102 :	Phaser_Rate_set(val);		break;
		case 103 :	Phaser_Feedback_set(val);	break;
		case 89 :	Phaser_Wet_set(val);		break;

		case 26 :	seq_switchMovingSeq(val);	break;	// toggle sequence mode
		//case 24 :	seq_switchGlide(val);		break; 	// toggle glissando
		case 25 :	seq_switchMute(val);		break;	// toggle muted notes

		case 33 :	seq_transp(-2, val);		break;	// transpose 1 tone down
		case 34 :	seq_transp(2, val);			break;	// transpose 1 tone up
		case 35 :	seq_transp(-7, val);		break;	//
		case 36 :	seq_transp( 7, val);		break;	//

		case 42 :	seq_gateTime_set(val);		break;	//

		case 43 :	AttTime_set(val);			break;	//
		case 50 :	DecTime_set(val);			break;	//
		case 51 :	SustLevel_set(val);			break;	//
		case 52 :	RelTime_set(val);			break;	//

		case 65 :	Filt1LFO_amp_set(val);		break;	//
		case 66 :	Filt1LFO_freq_set(val);		break;	//

		case 57 :	AmpLFO_amp_set(val);		break;	//
		case 58 :	AmpLFO_freq_set(val);		break;	//

		case 90 :	Filter2Freq_set(val);		break;	//
		case 91 :	Filter2Res_set(val);		break;	//
		case 92 :	Filter2Drive_set(val);		break;	//
		case 93 :	Filter2Type_set(val);		break;	//

		case 105 :	Filt2LFO_amp_set(val);		break;	//
		case 106 :	Filt2LFO_freq_set(val);		break;	//

		case 85 : 	FM_OP1_freq_set(val);		break;
		case 94 :	FM_OP1_modInd_set(val);		break;

		case 86 : 	FM_OP2_freq_set(val);		break;
		case 95 :	FM_OP2_modInd_set(val);		break;

		case 108 : 	FM_OP2_freqMul_inc(val);	break;
		case 117 :	FM_OP2_freqMul_dec(val);	break;

		case 87 : 	FM_OP3_freq_set(val);		break;
		case 96 :	FM_OP3_modInd_set(val);		break;

		case 109 : 	FM_OP3_freqMul_inc(val);	break;
		case 118 :	FM_OP3_freqMul_dec(val);	break;

		case 88 : 	FM_OP4_freq_set(val);		break;
		case 97 :	FM_OP4_modInd_set(val);		break;

		case 110 : 	FM_OP4_freqMul_inc(val);	break;
		case 119 :	FM_OP4_freqMul_dec(val);	break;

		case 59 : 	Drifter_amp_set(val);		break;
		case 60 : 	Drifter_minFreq_set(val);	break;
		case 61 : 	Drifter_maxFreq_set(val);	break;
		case 62 : 	Drifter_centralFreq_set(val);	break;

		}
	}
	*/
//}
