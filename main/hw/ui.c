/*
 * ui.c
 *
 *  Created on: Nov 5, 2018
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *
 */

#include <ui.h>

#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/init.h>
#include <hw/signals.h>
#include <hw/leds.h>
#include <string.h>

#include <Interface.h>

int event_next_channel = 0;
int event_channel_options = 0;
int settings_menu_active = 0;

extern int use_alt_settings;
extern int16_t *mixed_sample_buffer;

int context_menu_active = 0;

#ifdef BOARD_WHALE
int play_button_cnt = 0, short_press_button_play, long_press_button_play,
		button_play = 0;
int play_and_plus_button_cnt = 0, long_press_button_play_and_plus;

int hidden_menu = 0;
int remapped_channels[MAIN_CHANNELS + 1], remapped_channels_found;

#define SHORT_PRESS_SEQ_MAX	10
int short_press_seq_cnt = 0;
int short_press_seq_ptr = 0;
char short_press_seq[SHORT_PRESS_SEQ_MAX];
int short_press_sequence = 0;

#endif

#ifdef BOARD_GECHO

int rst_button_cnt = 0, long_press_rst_button = 0, short_press_rst_button = 0;
int set_button_cnt = 0, long_press_set_button = 0, short_press_set_button = 0;

#endif

//shared for both Gecho and Glo
int volume_buttons_cnt = 0, button_volume_plus = 0, button_volume_minus = 0,
		button_volume_both = 0;
int short_press_volume_minus, short_press_volume_plus, short_press_volume_both,
		long_press_volume_both, long_press_volume_plus, long_press_volume_minus;
int buttons_ignore_timeout = 0;

int BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_DEFAULT;

settings_t global_settings;
persistent_settings_t persistent_settings;

int tempo_bpm;// = global_settings.TEMPO_BPM_DEFAULT;
int TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO;
int tempoCounter, melodyCounter;

double global_tuning;// = global_settings.TUNING_DEFAULT;

//-----------------------------------------------------------------------------------------------------------------------------

void clear_buttons_counters_and_events()
{
	volume_buttons_cnt = 0;
	button_volume_plus = 0;
	button_volume_minus = 0;
	button_volume_both = 0;

	short_press_volume_minus = 0;
	short_press_volume_plus = 0;
	short_press_volume_both = 0;
	long_press_volume_both = 0;
	long_press_volume_plus = 0;
	long_press_volume_minus = 0;

	#ifdef BOARD_WHALE
	button_play = 0;
	play_button_cnt = 0;
	play_and_plus_button_cnt = 0;

	short_press_button_play = 0;
	long_press_button_play = 0;
	long_press_button_play_and_plus = 0;

	short_press_seq_cnt = 0;
	short_press_seq_ptr = 0;
	short_press_sequence = 0;
	#endif
}

#ifdef BOARD_WHALE

void buttons_sequence_check(int event)
{
	if(context_menu_active)
	{
		return;
	}

	if(!event) //periodical check
	{
		if(!short_press_seq_cnt && short_press_seq_ptr) //too late, reset everything
		{
			printf("buttons_sequence_check(): reset\n");
			short_press_seq_ptr = 0;
			return;
		}

		printf("buttons_sequence_check(): check for sequence: [%s], len=%d, counter=%d\n", short_press_seq, short_press_seq_ptr, short_press_seq_cnt);

		//check for expected sequences
		if(short_press_seq_ptr==8 && !strncmp(short_press_seq,"abbbccca", 8))
		{
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: abbbccca\n");
			voice_say("this_is_glo");
			//voice_say("upgrade_your_perception");
			factory_reset();
		}
		//else if(!strncmp(short_press_seq,"bbbbcccc", 8))
		//{
		//	printf("buttons_sequence_check(): sequence detected: bbbbcccc\n");
			//while(1);
		//}
		else if(short_press_seq_ptr==6 && !strncmp(short_press_seq,"bcbcbc", 6))
		{
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bcbcbc\n");
			beeps_enabled = 1 - beeps_enabled;
			persistent_settings.BEEPS = beeps_enabled;
			persistent_settings.BEEPS_updated = 1;
			persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
			printf("beeps_enabled = %d\n", beeps_enabled);
		}
		else if(short_press_seq_ptr==5 && !strncmp(short_press_seq,"bbbbb", 5)) //increase analog volume by 6db, up to 0dB, maximum level
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bbbbb\n");
			persistent_settings.ANALOG_VOLUME -= ANALOG_VOLUME_STEP; //negative logic, means increase
			if(persistent_settings.ANALOG_VOLUME < 0)
			{
				persistent_settings.ANALOG_VOLUME = 0;
			}
			persistent_settings.ANALOG_VOLUME_updated = 1;
			codec_analog_volume = persistent_settings.ANALOG_VOLUME;
			codec_set_analog_volume();
			store_persistent_settings(&persistent_settings);
			printf("buttons_sequence_check(): ANALOG_VOLUME increased by %ddB, current value = %d(%ddB)\n", ANALOG_VOLUME_STEP/2, persistent_settings.ANALOG_VOLUME, -persistent_settings.ANALOG_VOLUME/2);
		}
		else if(short_press_seq_ptr==5 && !strncmp(short_press_seq,"ccccc", 5)) //decrease analog volume by 6db, down to minimum level
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: ccccc\n");
			persistent_settings.ANALOG_VOLUME += ANALOG_VOLUME_STEP;
			if(persistent_settings.ANALOG_VOLUME > CODEC_ANALOG_VOLUME_MIN)
			{
				persistent_settings.ANALOG_VOLUME = CODEC_ANALOG_VOLUME_MIN;
			}
			persistent_settings.ANALOG_VOLUME_updated = 1;
			codec_analog_volume = persistent_settings.ANALOG_VOLUME;
			codec_set_analog_volume();
			store_persistent_settings(&persistent_settings);
			printf("buttons_sequence_check(): ANALOG_VOLUME decreased by %ddB, current value = %d(%ddB)\n", ANALOG_VOLUME_STEP/2, persistent_settings.ANALOG_VOLUME, -persistent_settings.ANALOG_VOLUME/2);
		}

		else if(short_press_seq_ptr==6 && !strncmp(short_press_seq,"bbbbbb", global_settings.AGC_MAX_GAIN_STEP)) //increase AGC max gain by 3db
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bbbbbb\n");

			if(global_settings.AGC_MAX_GAIN<=global_settings.AGC_MAX_GAIN_LIMIT-global_settings.AGC_MAX_GAIN_STEP)
			{
				global_settings.AGC_MAX_GAIN += global_settings.AGC_MAX_GAIN_STEP;
			}

			codec_configure_AGC(global_settings.AGC_ENABLED, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
			printf("buttons_sequence_check(): AGC_MAX_GAIN increased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, global_settings.AGC_MAX_GAIN);
		}
		else if(short_press_seq_ptr==6 && !strncmp(short_press_seq,"cccccc", global_settings.AGC_MAX_GAIN_STEP)) //decrease AGC max gain by 3db
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: cccccc\n");

			if(global_settings.AGC_MAX_GAIN>=global_settings.AGC_MAX_GAIN_STEP)
			{
				global_settings.AGC_MAX_GAIN -= global_settings.AGC_MAX_GAIN_STEP;
			}

			codec_configure_AGC(global_settings.AGC_ENABLED, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
			printf("buttons_sequence_check(): AGC_MAX_GAIN decreased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, global_settings.AGC_MAX_GAIN);
		}

		else if(short_press_seq_ptr==9 && !strncmp(short_press_seq,"cbcbcbcba", 9))
		{
			short_press_button_play = 0;
			//printf("buttons_sequence_check(): sequence detected: cbcbcbcba\n");
			printf("buttons_sequence_check(): Unlocking all channels\n");
			voice_say("upgrade_your_perception");
			//voice_say("this_is_glo");
			persistent_settings.ALL_CHANNELS_UNLOCKED = 1;
			persistent_settings.ALL_CHANNELS_UNLOCKED_updated = 1;
			//persistent_settings.FINE_TUNING_updated = 0; //ignore pressing of last volume button

			//persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
			store_persistent_settings(&persistent_settings);

			channels_found = unlocked_channels_found;
			//Delay(10000);
			whale_restart();
			//codec_select_input(ADC_INPUT_OFF);
			//channel_running = 0;
		}
		else if(short_press_seq_ptr==1 && short_press_seq[0]=='b')
		{
			short_press_volume_plus = 1;
		}
		else if(short_press_seq_ptr==1 && short_press_seq[0]=='c')
		{
			short_press_volume_minus = 1;
		}
		else if(short_press_seq_ptr==2 && !strncmp(short_press_seq,"bb", 2))
		{
			short_press_sequence = 2;
		}
		else if(short_press_seq_ptr==2 && !strncmp(short_press_seq,"cc", 2))
		{
			short_press_sequence = -2;
		}
		else if(short_press_seq_ptr==2 && !strncmp(short_press_seq,"bc", 2))
		{
			short_press_sequence = SEQ_PLUS_MINUS;
		}
		else if(short_press_seq_ptr==2 && !strncmp(short_press_seq,"cb", 2))
		{
			short_press_sequence = SEQ_MINUS_PLUS;
		}
		else if(short_press_seq_ptr==3 && !strncmp(short_press_seq,"bbb", 3))
		{
			short_press_sequence = 3;
		}
		else if(short_press_seq_ptr==3 && !strncmp(short_press_seq,"ccc", 3))
		{
			short_press_sequence = -3;
		}
		else if(short_press_seq_ptr==4 && !strncmp(short_press_seq,"bbbb", 4))
		{
			short_press_sequence = 4;
		}
		else if(short_press_seq_ptr==4 && !strncmp(short_press_seq,"cccc", 4))
		{
			short_press_sequence = -4;
		}

		if(short_press_seq_cnt==1) //only reset if timeout reached
		{
			short_press_seq_ptr = 0;
		}
	}
	else //if(event>=1 && event<=3)
	{
		if(short_press_seq_ptr<SHORT_PRESS_SEQ_MAX)
		{
			short_press_seq[short_press_seq_ptr] = 'a' + event - 1;
			//printf("buttons_sequence_check(): new event: %c\n", short_press_seq[short_press_seq_ptr]);
			short_press_seq_ptr++;
			short_press_seq_cnt = BUTTONS_SEQUENCE_TIMEOUT;

			//if(short_press_seq_ptr == SHORT_PRESS_SEQ_MAX) //maximum length reached, decode the sequence now
			if(short_press_seq_ptr >= 8) //decode the sequence immediately if at least 8 keys recorded
			{
				buttons_sequence_check(0);
			}
		}
	}
}

void process_buttons_controls_whale(void *pvParameters)
{
	printf("process_buttons_controls_whale(): started on core ID=%d\n",
			xPortGetCoreID());

	while (1)
	{
		Delay(BUTTONS_CNT_DELAY);

		if (channel_running)
		{
			//printf("process_buttons_controls_whale(): running on core ID=%d\n", xPortGetCoreID());

			if(!BUTTON_U1_ON && !BUTTON_U2_ON && !BUTTON_U3_ON) //no buttons pressed at the moment
			{
				buttons_ignore_timeout = 0;
			}

			if(buttons_ignore_timeout)
			{
				buttons_ignore_timeout--;
				continue;
			}

			if (volume_ramp && !settings_menu_active && !context_menu_active)
			{
				//printf("process_buttons_controls_whale(): volume_ramp\n");

				/*
				 if(codec_analog_volume>CODEC_ANALOG_VOLUME_DEFAULT) //actually means lower, as the value meaning is reversed
				 {
				 codec_analog_volume--; //actually means increment, as the value meaning is reversed
				 codec_set_analog_volume();
				 }
				 */
				if (codec_digital_volume > codec_volume_user) //actually means lower, as the value meaning is reversed
				{
					codec_digital_volume--; //actually means increment, as the value meaning is reversed
					codec_set_digital_volume();
				}
				else
				{
					volume_ramp = 0;
					printf("process_buttons_controls_whale(): volume_ramp finished\n");
				}
			}

			if (BUTTON_U1_ON) //button play
			{
				button_play++;
			}

			else if (BUTTON_U2_ON && !BUTTON_U3_ON) //button plus only
			{
				button_volume_plus++;
				/*
				 SAMPLE_VOLUME += 0.2f;
				 if(SAMPLE_VOLUME >= SAMPLE_VOLUME_MAX)
				 {
				 SAMPLE_VOLUME = SAMPLE_VOLUME_MAX;
				 }
				 */

				/*
				 if(codec_analog_volume>CODEC_ANALOG_VOLUME_MAX) //actually means lower, as the value meaning is reversed
				 {
				 codec_analog_volume--; //actually means increment, as the value meaning is reversed
				 }
				 codec_set_analog_volume();
				 */
				volume_buttons_cnt++;

				if ((volume_buttons_cnt > VOLUME_BUTTONS_CNT_THRESHOLD)
						&& (volume_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
						&& !settings_menu_active && !context_menu_active)
				{
					if (codec_digital_volume > CODEC_DIGITAL_VOLUME_MAX) //actually means lower, as the value meaning is reversed
					{
						codec_digital_volume--; //actually means increment, as the value meaning is reversed
						codec_set_digital_volume();
						persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
						persistent_settings.DIGITAL_VOLUME_updated = 1;
						persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
					}
				}

				if (volume_buttons_cnt == PLUS_OR_MINUS_BUTTON_LONG_PRESS)
				{
					printf("button_volume_plus(BUTTONS_LONG_PRESS) detected\n");
					long_press_volume_plus = 1;
				}
			}

			else if (BUTTON_U3_ON && !BUTTON_U2_ON) //button minus only
			{
				button_volume_minus++;
				/*
				 SAMPLE_VOLUME -= 0.2f;
				 if(SAMPLE_VOLUME <= SAMPLE_VOLUME_MIN)
				 {
				 SAMPLE_VOLUME = SAMPLE_VOLUME_MIN;
				 }
				 */

				/*
				 if(codec_analog_volume<CODEC_ANALOG_VOLUME_MIN) //actually means higher, as the value meaning is reversed
				 {
				 codec_analog_volume++; //actually means decrement, as the value meaning is reversed
				 }
				 codec_set_analog_volume();
				 */

				volume_buttons_cnt++;

				if ((volume_buttons_cnt > VOLUME_BUTTONS_CNT_THRESHOLD)
						&& (volume_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
						&& !settings_menu_active && !context_menu_active)
				{
					if (codec_digital_volume < CODEC_DIGITAL_VOLUME_MIN) //actually means higher, as the value meaning is reversed
					{
						codec_digital_volume++; //actually means decrement, as the value meaning is reversed
						codec_set_digital_volume();
						persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
						persistent_settings.DIGITAL_VOLUME_updated = 1;
						persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
					}
				}

				if (volume_buttons_cnt == PLUS_OR_MINUS_BUTTON_LONG_PRESS)
				{
					printf("button_volume_minus(BUTTONS_LONG_PRESS) detected\n");
					long_press_volume_minus = 1;
				}
			}
			else if (BUTTON_U2_ON && BUTTON_U3_ON) //button plus and minus at the same time
			{
				button_volume_both++;
				if (button_volume_both == VOLUME_BUTTONS_BOTH_LONG)
				{
					//special action
					long_press_volume_both = 1;
					buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
					printf("button_volume_both(VOLUME_BUTTONS_BOTH_LONG) detected\n");
				}
			}

			else //no button pressed at the moment
			{
				volume_buttons_cnt = 0;

				//check if any button was just released recently but not held long enough for volume adjustment or channel skip
				if (button_play > BUTTONS_SHORT_PRESS && button_play < PLAY_BUTTON_LONG_PRESS)
				{
					printf("button_play(BUTTONS_SHORT_PRESS) detected\n");
					if(!context_menu_active) { beep(3); }
					short_press_button_play = 1;
					buttons_sequence_check(1);
				}
				else if (button_volume_plus > BUTTONS_SHORT_PRESS && button_volume_plus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_plus(BUTTONS_SHORT_PRESS) detected\n");
					if(!context_menu_active) { beep(4); buttons_sequence_check(2); } else { short_press_volume_plus = 1; }
				}
				else if (button_volume_minus > BUTTONS_SHORT_PRESS && button_volume_minus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_minus(BUTTONS_SHORT_PRESS) detected\n");
					if(!context_menu_active) { beep(5); buttons_sequence_check(3); } else { short_press_volume_minus = 1; }
				}
				else if (button_volume_both > BUTTONS_SHORT_PRESS && button_volume_both < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_both(BUTTONS_SHORT_PRESS) detected\n");
					//beep(2);
					short_press_volume_both = 1;
				}

				button_volume_plus = 0;
				button_volume_minus = 0;
				button_volume_both = 0;
				button_play = 0;
			}

			if (BUTTON_U1_ON && !BUTTON_U2_ON && !BUTTON_U3_ON) //button play only
			{
				play_button_cnt++;
				if (play_button_cnt == PLAY_BUTTON_LONG_PRESS)
				{
					printf("button_play(BUTTONS_LONG_PRESS) detected\n");
					beep(6);
					long_press_button_play = 1;
				}
			}
			else
			{
				play_button_cnt = 0;
			}

			if (BUTTON_U1_ON && BUTTON_U2_ON && !BUTTON_U3_ON) //button play and plus
			{
				play_and_plus_button_cnt++;
				if (play_and_plus_button_cnt == PLAY_AND_PLUS_BUTTON_LONG_PRESS)
				{
					printf("button_play_and_plus(BUTTONS_LONG_PRESS) detected\n");
					long_press_button_play_and_plus = 1;
					buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
				}
			}
			else
			{
				play_and_plus_button_cnt = 0;
			}

#ifdef DISABLE_MCLK_1_SEC
			if(mclk_enabled && ms10_counter==100)
			{
				stop_MCLK();
			}
#endif

			ms10_counter++;
			if(persistent_settings.update)
			{
				if(persistent_settings.update==1)
				{
					//good time to save settings
					printf("process_buttons_controls_whale(): store_persistent_settings()\n");
					store_persistent_settings(&persistent_settings);
				}
				persistent_settings.update--;
			}

			//if(ms10_counter % BUTTONS_SEQUENCE_INTERVAL == 0) //every 1200ms
			if(short_press_seq_cnt)
			{
				short_press_seq_cnt--;
				if(short_press_seq_cnt==1)
				{
					buttons_sequence_check(0);
				}
			}
		}

		//---------------------------------------------------------------------------
		//actions for button events

		if (long_press_volume_both && !context_menu_active)
		{
			long_press_volume_both = 0;
			context_menu_active++;
			beep(2);
			Delay(50);
			clear_buttons_counters_and_events();
			buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
		}

		if(short_press_volume_both && !context_menu_active)
		{
			//printf("if(short_press_volume_both && !context_menu_active)\n");
			short_press_volume_both = 0;
			beep(4);
			if(mics_off) {
				mics_off = 0;
				codec_select_input(ADC_INPUT_MIC);
				printf("codec_select_input(ADC_INPUT_MIC);\n");
			} else {
				mics_off = 1;
				codec_select_input(ADC_INPUT_OFF);
				printf("codec_select_input(ADC_INPUT_OFF);\n");
			}
		}

		if (short_press_button_play && context_menu_active)
		{
			short_press_button_play = 0;
			context_menu_active++;

			if (context_menu_active == CONTEXT_MENU_ITEMS + 1)
			{
				beep(3); //same as short_press_button_play
				beep(5);
				context_menu_active = 0;
				clear_buttons_counters_and_events();
			}
			else
			{
				for (int i = 0; i < context_menu_active; i++)
				{
					beep(2);
					Delay(50);
				}
			}
		}

		if (context_menu_active)
		{
			if (short_press_volume_plus)
			{
				context_menu_action(context_menu_active, 1); //increase the value
				short_press_volume_plus = 0;
			}
			else if (short_press_volume_minus)
			{
				context_menu_action(context_menu_active, -1); //decrease the value
				short_press_volume_minus = 0;
			}
			else if (long_press_volume_both)
			{
				beep(6543);
				context_menu_action(context_menu_active, 0); //reset the value to default
				long_press_volume_both = 0;
				clear_buttons_counters_and_events();
				buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
			}
			else if (long_press_button_play) //exit context menu
			{
				beep(5);
				context_menu_active = 0;
				long_press_button_play = 0;
				clear_buttons_counters_and_events();
				buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
			}
		}
		else
		{
			if (long_press_button_play_and_plus)
			{
				beep(456456);
				move_channel_to_front(channel_loop);
				channel_loop = 0;
				long_press_button_play_and_plus = 0;
				buttons_ignore_timeout = BUTTONS_IGNORE_TIMEOUT_LONG;
			}

			if (long_press_button_play)
			{
				if (use_alt_settings)
				{
					event_next_channel = 1;
				}
				else
				{
					event_channel_options = 1;
				}
				long_press_button_play = 0;
			}

			if (short_press_button_play)
			{
				if (use_alt_settings)
				{
					event_channel_options = 1;
				}
				else
				{
					event_next_channel = 1;
				}
				short_press_button_play = 0;
			}
		}

		//---------------------------------------------------------------------------

	}
}
#endif

#ifdef BOARD_GECHO
void process_buttons_controls_gecho(void *pvParameters)
{
	printf("process_buttons_controls_gecho(): started on core ID=%d\n", xPortGetCoreID());

	while(1)
	{
		if(channel_running)
		{
			//printf("process_buttons_controls_gecho(): running on core ID=%d\n", xPortGetCoreID());

			if(volume_ramp && !settings_menu_active && !context_menu_active)
			{
				//printf("process_buttons_controls_gecho(): volume_ramp\n");

				if(codec_digital_volume>codec_volume_user)//actually means lower, as the value meaning is reversed
				{
					codec_digital_volume--; //actually means increment, as the value meaning is reversed
					codec_set_digital_volume();
				}
				else
				{
					volume_ramp = 0;
					printf("process_buttons_controls_gecho(): volume_ramp finished\n");
				}
			}

			if(BUTTON_U2_ON && !BUTTON_U1_ON) //button plus only
			{
				button_volume_plus++;
				volume_buttons_cnt++;

				if((volume_buttons_cnt > VOLUME_BUTTONS_CNT_THRESHOLD) && (volume_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0) && !settings_menu_active && !context_menu_active)
				{
					if(codec_digital_volume>CODEC_DIGITAL_VOLUME_MAX) //actually means lower, as the value meaning is reversed
					{
						codec_digital_volume--; //actually means increment, as the value meaning is reversed
						codec_set_digital_volume();
					}
				}

				if(volume_buttons_cnt == PLUS_OR_MINUS_BUTTON_LONG_PRESS)
				{
					printf("button_volume_plus(BUTTONS_LONG_PRESS) detected\n");
					long_press_volume_plus = 1;
				}
			}

			else if(BUTTON_U1_ON && !BUTTON_U2_ON) //button minus only
			{
				button_volume_minus++;
				volume_buttons_cnt++;

				if((volume_buttons_cnt > VOLUME_BUTTONS_CNT_THRESHOLD) && (volume_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0) && !settings_menu_active && !context_menu_active)
				{
					if(codec_digital_volume<CODEC_DIGITAL_VOLUME_MIN) //actually means higher, as the value meaning is reversed
					{
						codec_digital_volume++; //actually means decrement, as the value meaning is reversed
						codec_set_digital_volume();
					}
				}

				if(volume_buttons_cnt == PLUS_OR_MINUS_BUTTON_LONG_PRESS)
				{
					printf("button_volume_minus(BUTTONS_LONG_PRESS) detected\n");
					long_press_volume_minus = 1;
				}
			}
			else if(BUTTON_U1_ON && BUTTON_U2_ON) //button plus and minus at the same time
			{
				button_volume_both++;
				if(button_volume_both == VOLUME_BUTTONS_BOTH_LONG)
				{
					//special action
					long_press_volume_both = 1;
					printf("button_volume_both(VOLUME_BUTTONS_BOTH_LONG) detected\n");
				}
			}

			else if(BUTTON_RST_ON)
			{
				rst_button_cnt++;
				if(rst_button_cnt == RST_BUTTON_LONG_PRESS)
				{
					printf("rst_button(RST_BUTTON_LONG_PRESS) detected\n");
					long_press_rst_button = 1;
				}
			}

			else if(BUTTON_SET_ON)
			{
				set_button_cnt++;
				if(set_button_cnt == SET_BUTTON_LONG_PRESS)
				{
					printf("set_button(SET_BUTTON_LONG_PRESS) detected\n");
					long_press_set_button = 1;
				}
			}

			else //no button pressed at the moment
			{
				//check if any button was just released recently but not held long enough for long press event

				if(rst_button_cnt > RST_BUTTON_SHORT_PRESS && rst_button_cnt < RST_BUTTON_LONG_PRESS)
				{
					printf("rst_button(RST_BUTTON_SHORT_PRESS) detected\n");
					short_press_rst_button = 1;
				}
				else if(set_button_cnt > SET_BUTTON_SHORT_PRESS && set_button_cnt < SET_BUTTON_LONG_PRESS)
				{
					printf("set_button(SET_BUTTON_SHORT_PRESS) detected\n");
					short_press_set_button = 1;
				}
				else if(button_volume_plus > BUTTONS_SHORT_PRESS && button_volume_plus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_plus(BUTTONS_SHORT_PRESS) detected\n");
					short_press_volume_plus = 1;
				}
				else if(button_volume_minus > BUTTONS_SHORT_PRESS && button_volume_minus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_minus(BUTTONS_SHORT_PRESS) detected\n");
					short_press_volume_minus = 1;
				}
				else if(button_volume_both > BUTTONS_SHORT_PRESS && button_volume_both < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_both(BUTTONS_SHORT_PRESS) detected\n");
					short_press_volume_both = 1;
				}

				button_volume_plus = 0;
				button_volume_minus = 0;
				button_volume_both = 0;
				volume_buttons_cnt = 0;
				set_button_cnt = 0;
				rst_button_cnt = 0;
			}

			ms10_counter++;
		}
		else
		{
			if(BUTTON_RST_ON)
			{
				rst_button_cnt++;
				if(rst_button_cnt == RST_BUTTON_LONG_PRESS)
				{
					printf("rst_button(RST_BUTTON_LONG_PRESS) detected\n");
					esp_restart();
				}
			}
			else
			{
				rst_button_cnt = 0;
			}
		}

		//---------------------------------------------------------------------------
		//actions for button events

		if(context_menu_active)
		{

		}
		else
		{
			if(short_press_rst_button)
			{
				event_next_channel = 1;
				short_press_rst_button = 0;
			}
			/*
			 if(long_press_rst_button)
			 {
			 event_next_channel = 1;
			 long_press_rst_button = 0;
			 }
			 */
			if(short_press_set_button)
			{
				event_channel_options = 1;
				short_press_set_button = 0;
			}

		}

		//---------------------------------------------------------------------------

		Delay(BUTTONS_CNT_DELAY);
	}
}
#endif

void context_menu_action(int level, int direction)
{
	//printf("context_menu_action(level=%d,direction=%d)\n", level, direction);

	if (level == CONTEXT_MENU_BASS_TREBLE)
	{
		//printf("context_menu_action(): adjusting BASS / TREBLE\n");

		if (!direction)
		{
			EQ_bass_setting = EQ_BASS_DEFAULT;
			EQ_treble_setting = EQ_TREBLE_DEFAULT;
		}
		else if (direction == -1)
		{
			EQ_bass_setting += EQ_BASS_STEP;

			if (EQ_bass_setting > EQ_BASS_MAX)
			{
				EQ_bass_setting = EQ_BASS_MIN;
			}
		}
		else if (direction == 1)
		{
			EQ_treble_setting += EQ_TREBLE_STEP;

			if (EQ_treble_setting > EQ_TREBLE_MAX)
			{
				EQ_treble_setting = EQ_TREBLE_MIN;
			}
		}
		codec_set_eq();
		persistent_settings_store_eq();
	}
	else if (level == CONTEXT_MENU_TUNING)
	{
		//printf("context_menu_action(): adjusting TUNING\n");

		if (!direction)
		{
			set_tuning(TUNING_ALL_440);
			global_tuning = 440.0f;
		}
		else
		{
			if (direction == 1)
			{
				global_tuning *= global_settings.TUNING_INCREASE_COEFF; //TUNING_INCREASE_COEFF_10_CENTS;
			}
			else if (direction == -1)
			{
				global_tuning /= global_settings.TUNING_INCREASE_COEFF; //TUNING_INCREASE_COEFF_10_CENTS;
			}

			if (global_tuning < global_settings.TUNING_MIN) //GLOBAL_TUNING_MIN
			{
				global_tuning = global_settings.TUNING_MIN; //GLOBAL_TUNING_MIN
			}
			if (global_tuning > global_settings.TUNING_MAX) //GLOBAL_TUNING_MAX
			{
				global_tuning = global_settings.TUNING_MAX; //GLOBAL_TUNING_MAX
			}
		}

		//printf("context_menu_action(): new TUNING = %f\n", global_tuning);
		set_tuning_all(global_tuning);
		persistent_settings_store_tuning();
	}
	else if (level == CONTEXT_MENU_TEMPO)
	{
		//printf("context_menu_action(): adjusting TEMPO\n");

		if (!direction)
		{
			tempo_bpm = global_settings.TEMPO_BPM_DEFAULT;
		}
		else
		{
			tempo_bpm += direction * global_settings.TEMPO_BPM_STEP;

			if (tempo_bpm < global_settings.TEMPO_BPM_MIN)
			{
				tempo_bpm = global_settings.TEMPO_BPM_MIN;
			}
			if (tempo_bpm > global_settings.TEMPO_BPM_MAX)
			{
				tempo_bpm = global_settings.TEMPO_BPM_MAX;
			}
		}

		TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
		MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;
		DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

		//printf("context_menu_action(): new BPM=%d, TEMPO_BY_SAMPLE=%d, MELODY_BY_SAMPLE=%d, DELAY_BY_TEMPO=%d\n", tempo_bpm, TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO);

		persistent_settings_store_tempo();
	}
	else if (level == CONTEXT_MENU_ECHO_ARP)
	{
		//printf("context_menu_action(): adjusting ECHO / ARPEGGIATOR\n");

		if (!direction)
		{
			fixed_arp_level = 0;
			echo_dynamic_loop_current_step = 0;
		}
		else if (direction == 1) //plus button adjusts echo
		{
			echo_dynamic_loop_current_step++;
			if (echo_dynamic_loop_current_step == ECHO_DYNAMIC_LOOP_STEPS)
			{
				echo_dynamic_loop_current_step = 0;
			}
			echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
		}
		else if (direction == -1) //minus button adjusts arpeggiator
		{
			fixed_arp_level += FIXED_ARP_LEVEL_STEP;
			if (fixed_arp_level > FIXED_ARP_LEVEL_MAX)
			{
				fixed_arp_level = 0;
			}
		}
	}
}

#ifdef BOARD_WHALE
void voice_menu_say(const char *item, voice_menu_t *items, int total_items)
{
	for (int i = 0; i < total_items; i++)
	{
		if (!strncmp(items[i].name, item, strlen(item)))
		{
			printf("voice_menu_say(): playing item %s (%d - %d)\n", items[i].name, items[i].position_s, items[i].length_s);

			codec_set_mute(0); //unmute the codec

			for (int sample_ptr = 0; sample_ptr < items[i].length_s / 2; sample_ptr++)
			{
				sample32 = mixed_sample_buffer[(items[i].position_s / 2) + sample_ptr] / 8;
				sample32 <<= 16;
				sample32 &= 0xffff0000;
				sample32 += mixed_sample_buffer[(items[i].position_s / 2) + sample_ptr] / 8;

				/* swapped endians - total distortion

				 sample32 <<= 16;
				 sample32 &= 0x00ff0000;
				 sample32 += mixed_sample_buffer[(items[i].position_s/2)+sample_ptr];
				 sample32 <<= 8;
				 ((uint16_t*)(&sample32))[0] = ((uint16_t*)(&sample32))[1];
				 */

				i2s_push_sample(I2S_NUM, (char *) &sample32, portMAX_DELAY);
			}

			codec_set_mute(1); //mute the codec
			printf("voice_menu_say(): playing done\n");
		}
	}
}
#endif

uint16_t button_PWR_state = 0;
uint8_t button_user_state[4] = { 0, 0, 0, 0 };

int BUTTON_Ux_ON(int x)
{
	if (x == 0)
		return BUTTON_U1_ON;
	if (x == 1)
		return BUTTON_U2_ON;
	if (x == 2)
		return BUTTON_U3_ON;
#ifdef BOARD_GECHO
	if(x==3)return BUTTON_U4_ON;
#endif
	return -1;
}

#ifdef BOARD_GECHO
int scan_buttons()
{
	for(int i=0;i<4;i++)
	{
		if(BUTTON_Ux_ON(i))
		{
			if(!button_user_state[i])
			{
				button_user_state[i]=1;
				return i;
			}
		}
		else
		{
			button_user_state[i]=0;
		}
	}

	if(BUTTON_SET_ON) //simpler
	{
		button_PWR_state++;

		if(button_PWR_state >= PWR_BUTTON_THRESHOLD_2)
		{
			return 5;
		}
		else if(button_PWR_state == PWR_BUTTON_THRESHOLD_3)
		{
			return 6;
		}
	}
	else
	{
		if(button_PWR_state >= PWR_BUTTON_THRESHOLD_1)
		{
			return 4;
		}

		button_PWR_state = 0;
	}

	return -1;
}

uint64_t get_user_buttons_sequence(int *press_order_ptr, int *press_order)
{
	uint64_t res = 0;
	if(press_order_ptr[0] > 0) //if some program chosen, run it!
	{
		LED_RDY_ON; //set RDY so the sensors will work later
		//return binary_status_O4;
		for(int i=0;i<press_order_ptr[0];i++)
		{
			res *=10;
			res += press_order[i]+1;
		}
	}
	return res;
}

void user_button_pressed(int button, int *binary_status_O4, int *press_order_ptr, int *press_order)
{
	binary_status_O4[0] ^= (0x01 << button);
	if(binary_status_O4[0] & (0x01 << button))
	{
		LED_O4_set(button, 1); //set n-th LED on
	}
	else
	{
		LED_O4_set(button, 0); //set n-th LED off
	}
	if(press_order_ptr[0]<PRESS_ORDER_MAX)
	{
		press_order_ptr[0]++;
	}
	press_order[press_order_ptr[0]-1] = button;
}

uint64_t select_channel()
{
	int button;
	int binary_status_O4 = 0;
	int loop = 0;
	int press_order[PRESS_ORDER_MAX], press_order_ptr = 0;
	uint64_t result = 0; //2^64 = 18446744073709551616 -> max 20 digits (or 19 if can start with 4)

	button_PWR_state = 0;

	while(1)
	{
		//new_random_value(); //shuffle the random value for better entropy

		if(loop % (1000/SCAN_BUTTONS_LOOP_DELAY) == 0)
		{
			LED_RDY_ON;
		}
		if(loop % (2000/SCAN_BUTTONS_LOOP_DELAY) == (1000/SCAN_BUTTONS_LOOP_DELAY))
		{
			LED_RDY_OFF;
		}
		loop++;

		if((button = scan_buttons()) > -1) //some button was just pressed
		{
			//printf("select_channel(): scan_buttons() returned code = %d\n", button);

			//if(button==0) LED_O1_ON; else LED_O1_OFF;
			//if(button==1) LED_O2_ON; else LED_O2_OFF;
			//if(button==2) LED_O3_ON; else LED_O3_OFF;
			//if(button==3) LED_O4_ON; else LED_O4_OFF;

			if(button<4)
			{
				printf("select_channel(): button %d pressed\n", button+1);
				user_button_pressed(button, &binary_status_O4, &press_order_ptr, press_order);
			}
			else if(button==4) //PWR pressed for 100ms
			{
				result = get_user_buttons_sequence(&press_order_ptr, press_order);
				if(result>0)
				{
					//since the channel was selected manually and not via serial command, will block serial
					//GPIO_Deinit_USART1();
					return result;
				}
			}
			else if(button==5) //PWR pressed for 1 second
			{
				if(press_order_ptr == 0) //if no program chosen, turn off
				{
					return 0; //mcu_and_codec_shutdown();
				}
				else
				{
					result = get_user_buttons_sequence(&press_order_ptr, press_order);
					//since the channel was selected manually and not via serial command, will block serial
					//GPIO_Deinit_USART1();
					return result * 10 + 5;//append 5 to the decimal expansion of the number
				}
			}
			else if(button==6) //PWR pressed for 3 seconds, power off anyway
			{
				return 0; //mcu_and_codec_shutdown();
			}
		}

		Delay(SCAN_BUTTONS_LOOP_DELAY);

		//new_random_value(); //get some entropy
	}
}
#endif

int get_tempo_by_BPM(int bpm)
{
	//int b = 2*SAMPLE_RATE_DEFAULT; //120BPM (16s / loop);
	int b = SAMPLE_RATE_DEFAULT; //120BPM (16s / loop);

	if (bpm == 120)
	{
		return b;
	}

	//b = ((int)((2*(double)SAMPLE_RATE_DEFAULT*120.0f/(double)bpm)/4))*4; //must be divisible by 4 and rounded down
	b = ((int) ((double) SAMPLE_RATE_DEFAULT * 120.0f / (double) bpm) / 4) * 4; //must be divisible by 4 and rounded down
	return b;
}

int get_delay_by_BPM(int bpm)
{
	int buffer_length = get_tempo_by_BPM(bpm);
	int buffer_limit;

	if (tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
	{
		buffer_limit = ECHO_BUFFER_LENGTH_DEFAULT;
	}
	else //in filtered channels, if BPM >= 120, echo can go up to 1.5 times DELAY_BY_TEMPO value
	{
		buffer_limit = (ECHO_BUFFER_LENGTH_DEFAULT * 2) / 3;
	}

	while (buffer_length > buffer_limit)
	{
		buffer_length /= 2;
	}
	return buffer_length;
}

#ifdef BOARD_WHALE
void check_auto_power_off()
{
	auto_power_off--;

	//printf("check_auto_power_off(): timer = %d\n", auto_power_off);

	if(auto_power_off < AUTO_POWER_OFF_VOLUME_RAMP)
	{
		codec_digital_volume = codec_volume_user + (int)((1.0f - (float)(auto_power_off) / (float)AUTO_POWER_OFF_VOLUME_RAMP) * ((float)(CODEC_DIGITAL_VOLUME_MIN) / 1.5f - (float)(codec_volume_user)));
		codec_set_digital_volume();
	}

	if(auto_power_off==0)
	{
		whale_shutdown();
	}
}
#endif
