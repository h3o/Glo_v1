/*
 * ui.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Nov 5, 2018
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include <ui.h>

#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/init.h>
#include <hw/signals.h>
#include <hw/leds.h>
#include <hw/sdcard.h>
#include <hw/midi.h>
#include <hw/sync.h>
#include <string.h>

#include <Interface.h>

int select_channel_RDY_idle_blink = 1;
int event_next_channel = 0;
int event_channel_options = 0;
int settings_menu_active = 0;
//int menu_indicator_active = 0;
int context_menu_enabled = 1;
int context_menu_active = 0;
int main_menu_active = 0;
int main_menu_restart_on_close = 0;
int last_settings_level = 0;
//int default_setting_cycle = 0;
int ui_command = 0;
int ui_ignore_events = 0;
int ui_button3_enabled = 1;
int ui_button4_enabled = 1;

#ifdef BOARD_WHALE

extern int use_alt_settings;
//extern int16_t *mixed_sample_buffer;

int volume_buttons_cnt = 0;
int play_button_cnt = 0, short_press_button_play, long_press_button_play, button_play = 0;
int play_and_plus_button_cnt = 0, long_press_button_play_and_plus;

int hidden_menu = 0;
int remapped_channels[MAIN_CHANNELS + 1], remapped_channels_found;

#define SHORT_PRESS_SEQ_MAX	10
int short_press_seq_cnt = 0;
int short_press_seq_ptr = 0;
char short_press_seq[SHORT_PRESS_SEQ_MAX];
int short_press_sequence = 0;

int button_volume_plus = 0, button_volume_minus = 0,
		button_volume_both = 0;
int short_press_volume_minus, short_press_volume_plus, short_press_volume_both,
		long_press_volume_both, long_press_volume_plus, long_press_volume_minus;
int buttons_ignore_timeout = 0;

int BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_DEFAULT;

#endif

#ifdef BOARD_GECHO

int user_buttons_cnt = 0;
int use_acc_or_ir_sensors = PARAMETER_CONTROL_SENSORS_ACCELEROMETER; //0 = accelerometer, 1 = IR sensors
//float normalized_params[4];

int btn_counters[6] = {0,0,0,0,0,0};

int btn_event = 0, btn_event_ext = 0;
int next_event_lock = 0;

uint8_t load_SD_card_sample = SD_CARD_SAMPLE_NOT_USED;
uint8_t SD_card_sample_buffer_half = 0;

#endif

settings_t global_settings;
persistent_settings_t persistent_settings;

int tempo_bpm;// = global_settings.TEMPO_BPM_DEFAULT;
int TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO;
int tempoCounter, melodyCounter;
int MIDI_SYNC_derived_tempo = 0;

double global_tuning;// = global_settings.TUNING_DEFAULT;
uint8_t midi_sync_mode, midi_polyphony;
uint8_t sensors_active = 0, accelerometer_active = 0;
uint32_t sensors_override = 0;

//indicators to show status of ADC_input_select (controlled by button #4 during play)
//const uint8_t input_select_indicators[ADC_INPUT_MODES] = {0x18, 0x81, 0x66, 0xe7, 0x86, 0x61, 0xa6, 0x65, 0xa7, 0xe5};
//line inputs are actually swapped in hardware
const uint8_t input_select_indicators[ADC_INPUT_MODES] = {0x18, 0x81, 0x66, 0xe7, 0x61, 0x86, 0x65, 0xa6, 0xe5, 0xa7};

#define SETTINGS_INDICATION_TIMEOUT 100
int settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;

//-----------------------------------------------------------------------------------------------------------------------------

#ifdef BOARD_GECHO
void process_buttons_controls_gecho(void *pvParameters)
{
	printf("process_buttons_controls_gecho(): started on core ID=%d\n", xPortGetCoreID());

	int auto_shutdown_timeout_counter = 0;
	//int seconds_lapsed = 0;

	while(1)
	{
		Delay(BUTTONS_CNT_DELAY);

		//printf("ms10_counter=%d,main_menu_active=%d,context_menu_active=%d\n",ms10_counter,main_menu_active,context_menu_active);

		#ifdef BOARD_GECHO
		auto_shutdown_timeout_counter++;
		if(auto_shutdown_timeout_counter==100)
		{
			auto_shutdown_timeout_counter = 0;
			//printf("%d sec\n", seconds_lapsed++);

			if(channel_running)
			{
				auto_power_off--;
				//printf("auto_power_off=%d\n",auto_power_off);

				if (auto_power_off < AUTO_POWER_OFF_VOLUME_RAMP)
				{
					//codec_digital_volume = codec_volume_user + (int)((1.0f - (float)(auto_power_off) / (float)AUTO_POWER_OFF_VOLUME_RAMP) * ((float)(CODEC_DIGITAL_VOLUME_MIN) / 1.5f - (float)(codec_volume_user)));
					codec_digital_volume = codec_volume_user + ((CODEC_DIGITAL_VOLUME_MIN * 2 * (AUTO_POWER_OFF_VOLUME_RAMP-auto_power_off)) / 3) / AUTO_POWER_OFF_VOLUME_RAMP;
					if(codec_digital_volume > CODEC_DIGITAL_VOLUME_MIN) codec_digital_volume = CODEC_DIGITAL_VOLUME_MIN;
					//printf("auto_power_off=%d, codec_volume_user=%d, codec_digital_volume=%d\n", auto_power_off, codec_volume_user, codec_digital_volume);
					codec_set_digital_volume();
				}

				if (auto_power_off==0)
				{
					printf("Shutting down\n");
					codec_set_mute(1); //mute the codec
					sensors_active = 0;
					accelerometer_active = 0;
					Delay(100);
					codec_reset();
					stop_MCLK();
					LEDs_all_OFF();
					IR_DRV1_OFF;
					IR_DRV2_OFF;
					LED_RDY_OFF;
					Delay(10);
					esp_deep_sleep_start();
				}
			}
		}
		#endif

		if(settings_indication_in_progress)
		{
			settings_indication_in_progress--;
			if(!settings_indication_in_progress)
			{
				LED_R8_all_OFF();
			}
		}

		if(main_menu_active)
		{
			continue;
		}

		ms10_counter++;
		if (persistent_settings.update)
		{
			if (persistent_settings.update==1)
			{
				//good time to save settings
				printf("process_buttons_controls_whale(): store_persistent_settings()\n");
				store_persistent_settings(&persistent_settings);
			}
			persistent_settings.update--;
		}

		//printf("process_buttons_controls_gecho(): running on core ID=%d\n", xPortGetCoreID());

		if (volume_ramp && !settings_menu_active && !context_menu_active)
		{
			//printf("process_buttons_controls_gecho(): volume_ramp\n");

			if (codec_digital_volume>codec_volume_user)//actually means lower, as the value meaning is reversed
			{
				codec_digital_volume--; //actually means increment, as the value meaning is reversed
				codec_set_digital_volume();
			}
			else
			{
				volume_ramp = 0;
				printf("process_buttons_controls_gecho(): volume_ramp finished\n");
			}
			continue;
		}
		if (next_event_lock)
		{
			//wait till all buttons released
			while(ANY_BUTTON_ON) { Delay(10); }
			printf("process_buttons_controls_gecho(): next_event_lock -> all buttons released\n");
			next_event_lock = 0;
		}

		if (!channel_running)
		{
			continue;
		}

		if (NO_BUTTON_ON)
		{
			//printf("process_buttons_controls_gecho(): NO_BUTTON_ON -> volume_buttons_cnt = 0;\n");
			user_buttons_cnt = 0;
		}

		for(int b=0;b<BUTTONS_N;b++)
		{
			if (BUTTON_ON(b))
			{
				btn_counters[b]++;

				if (btn_counters[b] > BUTTON_LONG_THRESHOLD(b))
				{
					btn_event = BUTTON_EVENT_LONG_PRESS + b;
				}
			}
			else if (btn_counters[b])
			{
				if (IS_USER_BTN(b) && (btn_counters[BUTTON_SET] || btn_counters[BUTTON_RST]))
				{
					if (btn_counters[BUTTON_SET])
					{
						btn_event = BUTTON_EVENT_SET_PLUS + b;
					}
					else
					{
						btn_event = BUTTON_EVENT_RST_PLUS + b;
					}
					btn_counters[BUTTON_SET] = 0;
					btn_counters[BUTTON_RST] = 0;
					next_event_lock = 1;
				}
				else if (b==BUTTON_RST && btn_counters[BUTTON_SET])
				{
					btn_event = BUTTON_EVENT_SET_PLUS + BUTTON_RST;
					btn_counters[BUTTON_SET] = 0;
					btn_counters[BUTTON_RST] = 0;
					next_event_lock = 1;
				}
				else if (btn_counters[b] > BUTTONS_SHORT_PRESS && btn_counters[b] < BUTTON_LONG_THRESHOLD(b))
				{
					btn_event = BUTTON_EVENT_SHORT_PRESS + b;
				}
				else if (btn_counters[b] > BUTTON_LONG_THRESHOLD(b))
				{
					btn_event = BUTTON_EVENT_NONE;
				}
				btn_counters[b] = 0;
			}
		}

		if(ui_ignore_events)
		{
			btn_event_ext = btn_event;
			btn_event = 0;
			continue;
		}

		if (btn_event)
		{
			//printf("btn_event = %d\n", btn_event);

			//---------------------------------------------------------------------------
			//actions for button events

			if (btn_event == BUTTON_EVENT_LONG_PRESS+BUTTON_RST)
			{
				printf("rst_button(RST_BUTTON_LONG_PRESS) detected, restarting\n");
				esp_restart();
			}

			if (context_menu_active)
			{
				if (btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_RST)
				{
					LED_O4_all_OFF();
					LED_R8_all_OFF();
					context_menu_active = 0;
					printf("context_menu_active = %d\n", context_menu_active);
					codec_mute_inputs(1); //0 = mute, 1 = unmute
				}
				if (btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_SET)
				{
					LED_O4_all_OFF();
					LED_R8_all_OFF();
					last_settings_level = 0;
					context_menu_active++;
					{
						if (context_menu_active>4)
						{
							context_menu_active = 0;
							codec_mute_inputs(1); //0 = mute, 1 = unmute
						}
						else
						{
							LED_O4_set(context_menu_active-1, 1);
							if(CONTEXT_MENU_WITH_MUTED_ADC)
							{
								codec_mute_inputs(0); //0 = mute, 1 = unmute
							}
						}
						printf("context_menu_active = %d\n", context_menu_active);
					}
				}

				if (context_menu_active && btn_event >= BUTTON_EVENT_SHORT_PRESS + BUTTON_1 && btn_event <= BUTTON_EVENT_SHORT_PRESS + BUTTON_4)
				{
					last_settings_level = 2*context_menu_active - ((btn_event > BUTTON_EVENT_SHORT_PRESS + BUTTON_2)?0:1);
					context_menu_action(last_settings_level, 1-2*(btn_event%2));
				}
				if (context_menu_active && btn_event >= BUTTON_EVENT_LONG_PRESS + BUTTON_1 && btn_event <= BUTTON_EVENT_LONG_PRESS + BUTTON_4)
				{
					last_settings_level = 2*context_menu_active - ((btn_event > BUTTON_EVENT_LONG_PRESS + BUTTON_2)?0:1);
					context_menu_action(last_settings_level, 2-2*(btn_event%2));
				}
			}
			else
			{
				if (btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_RST)
				{
					event_next_channel = 1;
				}
				else if (btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_SET)
				{
					event_channel_options = 1;
				}
				else if (btn_event == BUTTON_EVENT_LONG_PRESS+BUTTON_2 && !BUTTON_SET_ON && !BUTTON_RST_ON)
				{
					user_buttons_cnt++;

					if (user_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
					{
						if (codec_digital_volume>CODEC_DIGITAL_VOLUME_MAX) //actually means lower, as the value meaning is reversed
						{
							codec_digital_volume--; //actually means increment, as the value meaning is reversed
							codec_set_digital_volume();
							if(auto_power_off > AUTO_POWER_OFF_VOLUME_RAMP)
							{
								codec_volume_user = codec_digital_volume;
								persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
								persistent_settings.DIGITAL_VOLUME_updated = 1;
								persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
							}
						}
					}
				}
				else if (btn_event == BUTTON_EVENT_LONG_PRESS+BUTTON_1 && !BUTTON_SET_ON && !BUTTON_RST_ON)
				{
					user_buttons_cnt++;

					if (user_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
					{
						if (codec_digital_volume<CODEC_DIGITAL_VOLUME_MIN) //actually means higher, as the value meaning is reversed
						{
							codec_digital_volume++; //actually means decrement, as the value meaning is reversed
							codec_set_digital_volume();
							if(auto_power_off > AUTO_POWER_OFF_VOLUME_RAMP)
							{
								codec_volume_user = codec_digital_volume;
								persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
								persistent_settings.DIGITAL_VOLUME_updated = 1;
								persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
							}
						}
					}
				}
				else if (btn_event == BUTTON_EVENT_LONG_PRESS+BUTTON_4 && !BUTTON_SET_ON && !BUTTON_RST_ON)
				{
					user_buttons_cnt++;

					if (user_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
					{
						codec_adjust_ADC_gain(1);
					}
					Delay(50);
				}
				else if (btn_event == BUTTON_EVENT_LONG_PRESS+BUTTON_3 && !BUTTON_SET_ON && !BUTTON_RST_ON)
				{
					user_buttons_cnt++;

					if (user_buttons_cnt % VOLUME_BUTTONS_CNT_DIV == 0)
					{
						codec_adjust_ADC_gain(-1);
					}
					Delay(50);
				}

				if(!settings_menu_active) //sub-menu that is invoked by pressing SET in some channels (e.g. Dekrispator)
				{
					if ((btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_3 && !BUTTON_SET_ON && !BUTTON_RST_ON)
					|| (btn_event == BUTTON_EVENT_RST_PLUS + BUTTON_3))
					{
						if(ui_button3_enabled)
						{
							echo_dynamic_loop_current_step++;
							if (echo_dynamic_loop_current_step == ECHO_DYNAMIC_LOOP_STEPS)
							{
								echo_dynamic_loop_current_step = 0;
							}
							if (btn_event == BUTTON_EVENT_RST_PLUS + BUTTON_3)
							{
								echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_LENGTH_ECHO_OFF;
							}
							DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);
							//echo_dynamic_loop_length = (float)echo_dynamic_loop_steps[echo_dynamic_loop_current_step] * (float)DELAY_BY_TEMPO/(float)I2S_AUDIOFREQ;

							uint32_t delay_len; //temp value so the calculation does not overflow int range
							//DELAY_BY_TEMPO = 50780, delay_len[1] = 3867912600 delay_len[2] = 76170
							//gets up to 3,867,912,600 max int 2,147,483,647 max uint 4,294,967,295

							delay_len = echo_dynamic_loop_steps[echo_dynamic_loop_current_step] * DELAY_BY_TEMPO;
							//printf("DELAY_BY_TEMPO = %d, delay_len[1] = %u\n", DELAY_BY_TEMPO, delay_len);
							delay_len /= I2S_AUDIOFREQ;
							//printf("delay_len[2] = %u\n", delay_len);

							echo_dynamic_loop_length = delay_len;

							while (echo_dynamic_loop_length > ECHO_BUFFER_LENGTH)
							{
								echo_dynamic_loop_length /= 2;
							}

							printf("process_buttons_controls_gecho(): echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);

							settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
							if(!echo_dynamic_loop_length)
							{
								indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 1, 50);
							}
							else
							{
								//LED_R8_set_byte(1<<(echo_dynamic_loop_current_step%8));
								LED_R8_set_byte(echo_dynamic_loop_indicator[echo_dynamic_loop_current_step]);
							}
							settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
						}
					}
					else if ((btn_event == BUTTON_EVENT_SHORT_PRESS+BUTTON_4 && !BUTTON_SET_ON && !BUTTON_RST_ON)
						 || (btn_event == BUTTON_EVENT_RST_PLUS + BUTTON_4))
					{
						if(ui_button4_enabled)
						{
							ADC_input_select++;
							if (ADC_input_select == ADC_INPUT_MODES)
							{
								ADC_input_select = 0;
							}
							if (btn_event == BUTTON_EVENT_RST_PLUS + BUTTON_4)
							{
								ADC_input_select = ADC_INPUT_OFF;
							}
							//codec_select_input(ADC_INPUT_OFF);
							printf("process_buttons_controls_gecho(): input_select = %d\n", ADC_input_select);
							codec_select_input(ADC_input_select);

							persistent_settings.ADC_INPUT_SELECT = ADC_input_select;
							persistent_settings.ADC_INPUT_SELECT_updated = 1;
							persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;

							settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
							//menu_indicator_active = 1;
							LED_O4_set_byte(0x08);
							indicate_context_setting(input_select_indicators[ADC_input_select], 6, 40);
							LED_O4_all_OFF();
							//menu_indicator_active = 0;
							settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
						}
					}
					else if (btn_event == BUTTON_EVENT_LONG_PRESS + BUTTON_SET)
					{
						start_stop_recording();
						next_event_lock = 1;
					}
					else if (btn_event == BUTTON_EVENT_SET_PLUS + BUTTON_RST)
					{
						use_recorded_sample();
						next_event_lock = 1;
					}
				}
			}

			if(context_menu_enabled)
			if (btn_event >= BUTTON_EVENT_SET_PLUS + BUTTON_1 && btn_event <= BUTTON_EVENT_SET_PLUS + BUTTON_4)
			{
				context_menu_active = btn_event - BUTTON_EVENT_SET_PLUS;
				printf("context_menu_active = %d\n", context_menu_active);
				LEDs_all_OFF();
				LED_O4_set(context_menu_active-1, 1);
				last_settings_level = 0;
				//default_setting_cycle = 0;

				if(CONTEXT_MENU_WITH_MUTED_ADC)
				{
					codec_mute_inputs(0); //0 = mute, 1 = mute
				}
				else
				{
					codec_mute_inputs(1); //0 = mute, 1 = unmute
				}
			}
			//---------------------------------------------------------------------------

			btn_event_ext = btn_event;
			btn_event = 0;
		}
		/*
		//blink the orange LED to indicate which context menu is active
		else if (context_menu_active)
		{
			LED_O4_set(context_menu_active-1, (ms10_counter/12)%2);
		}
		*/
	}
}
#endif

void context_menu_action(int level, int direction)
{
	printf("context_menu_action(level=%d,direction=%d)\n", level, direction);

	if (level == CONTEXT_MENU_ANALOG_VOLUME)
	{
		//if short press, update every time, if long press, only at certain intervals
		user_buttons_cnt++;
		if (direction != -1 && direction != 1 && user_buttons_cnt % VOLUME_BUTTONS_CNT_DIV != 0)
		{
			Delay(VOLUME_BUTTONS_CONTEXT_MENU_DELAY);
			return;
		}

		if (direction >= 1) //increase analog volume, up to 0dB, maximum level
		{
			persistent_settings.ANALOG_VOLUME -= ANALOG_VOLUME_STEP; //negative logic, means increase
			if (persistent_settings.ANALOG_VOLUME < 0)
			{
				persistent_settings.ANALOG_VOLUME = 0;
				indicate_context_setting(0xe0, 8, 40);
			}
			else
			{
				indicate_context_setting(direction>=1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
				persistent_settings.ANALOG_VOLUME_updated = 1;
				codec_analog_volume = persistent_settings.ANALOG_VOLUME;
				codec_set_analog_volume();
				//store_persistent_settings(&persistent_settings);
				persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
				printf("context_menu_action(): ANALOG_VOLUME increased, current value = %d(%.1fdB)\n", persistent_settings.ANALOG_VOLUME, ((double)-persistent_settings.ANALOG_VOLUME)/2);
			}
		}
		else if (direction <= 0) //decrease analog volume, down to minimum level
		{
			persistent_settings.ANALOG_VOLUME += ANALOG_VOLUME_STEP; //negative logic, means decrease
			if (persistent_settings.ANALOG_VOLUME > CODEC_ANALOG_VOLUME_MIN)
			{
				persistent_settings.ANALOG_VOLUME = CODEC_ANALOG_VOLUME_MIN;
				indicate_context_setting(0x07, 8, 40);
			}
			else
			{
				indicate_context_setting(direction>=1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
				persistent_settings.ANALOG_VOLUME_updated = 1;
				codec_analog_volume = persistent_settings.ANALOG_VOLUME;
				codec_set_analog_volume();
				//store_persistent_settings(&persistent_settings);
				persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
				printf("context_menu_action(): ANALOG_VOLUME decreased, current value = %d(%.1fdB)\n", persistent_settings.ANALOG_VOLUME, ((double)-persistent_settings.ANALOG_VOLUME)/2);
			}
		}
	}
	else if (level == CONTEXT_MENU_AGC_MAX_GAIN)
	{
		if (direction == 1) //button #2
		{
			if (persistent_settings.AGC_MAX_GAIN<=global_settings.AGC_MAX_GAIN_LIMIT-global_settings.AGC_MAX_GAIN_STEP)
			{
				persistent_settings.AGC_MAX_GAIN += global_settings.AGC_MAX_GAIN_STEP; //increase AGC max gain by default step (e.g. 6 = 3dB)

				codec_configure_AGC(persistent_settings.AGC_ENABLED_OR_PGA, persistent_settings.AGC_MAX_GAIN, persistent_settings.AGC_ENABLED_OR_PGA);
				printf("context_menu_action(): AGC_MAX_GAIN increased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, persistent_settings.AGC_MAX_GAIN);

				indicate_context_setting(direction>=1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
				persistent_settings.AGC_MAX_GAIN_updated = 1;
				persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
			}
			else
			{
				indicate_context_setting(0xe0, 8, 40);
			}
		}
		else if (direction == -1) //button #1
		{
			if (persistent_settings.AGC_MAX_GAIN>=global_settings.AGC_MAX_GAIN_STEP)
			{
				persistent_settings.AGC_MAX_GAIN -= global_settings.AGC_MAX_GAIN_STEP; //decrease AGC max gain by default step (e.g. 6 = 3dB)

				codec_configure_AGC(persistent_settings.AGC_ENABLED_OR_PGA, persistent_settings.AGC_MAX_GAIN, persistent_settings.AGC_ENABLED_OR_PGA);
				printf("context_menu_action(): AGC_MAX_GAIN decreased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, persistent_settings.AGC_MAX_GAIN);

				indicate_context_setting(direction>=1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
				persistent_settings.AGC_MAX_GAIN_updated = 1;
				persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
			}
			else
			{
				indicate_context_setting(0x07, 8, 40);
			}
		}
		else if (!direction || direction == 2)
		{
			persistent_settings.AGC_MAX_GAIN = global_settings.AGC_MAX_GAIN;
			codec_configure_AGC(persistent_settings.AGC_ENABLED_OR_PGA, persistent_settings.AGC_MAX_GAIN, persistent_settings.AGC_ENABLED_OR_PGA);
			printf("context_menu_action(): AGC_MAX_GAIN set to default value %ddB\n", persistent_settings.AGC_MAX_GAIN);

			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
			indicate_context_setting(0x18, 3, 100);
			LED_R8_set_byte(0x18);
			persistent_settings.AGC_MAX_GAIN_updated = 1;
			persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
		}
	}
	else if (level == CONTEXT_MENU_BASS_TREBLE)
	{
		//printf("context_menu_action(): adjusting BASS / TREBLE\n");

		if (!direction || direction == 2)
		{
			#ifdef BOARD_WHALE
			EQ_bass_setting = EQ_BASS_DEFAULT;
			EQ_treble_setting = EQ_TREBLE_DEFAULT;
			#else
			if (!direction)
			{
				EQ_bass_setting = EQ_BASS_DEFAULT;
			}
			else
			{
				EQ_treble_setting = EQ_TREBLE_DEFAULT;
			}
			#endif
			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
			indicate_context_setting(0x18, 3, 100);
			LED_R8_set_byte(0x18);
		}
		else if (direction == -1)
		{
			EQ_bass_setting += EQ_BASS_STEP;

			if (EQ_bass_setting > EQ_BASS_MAX)
			{
				EQ_bass_setting = EQ_BASS_MIN;
			}
			indicate_eq_bass_setting(EQ_bass_setting, SETTINGS_INDICATOR_EQ_BASS);
		}
		else if (direction == 1)
		{
			EQ_treble_setting += EQ_TREBLE_STEP;

			if (EQ_treble_setting > EQ_TREBLE_MAX)
			{
				EQ_treble_setting = EQ_TREBLE_MIN;
			}
			indicate_eq_bass_setting(EQ_treble_setting, SETTINGS_INDICATOR_EQ_TREBLE);
		}
		codec_set_eq();
		persistent_settings_store_eq();
	}
	else if (level == CONTEXT_MENU_TUNING)
	{
		//printf("context_menu_action(): adjusting TUNING\n");

		if (!direction || direction == 2)
		{
			#ifdef BOARD_WHALE
			set_tuning(TUNING_ALL_440);
			global_tuning = 440.0f;
			#else
			if (!direction)
			{
				set_tuning(TUNING_ALL_432);
				global_tuning = 432.0f;
				indicate_context_setting(0x0f, 3, 100);
				LED_R8_set_byte(0x0f);
			}
			else
			{
				set_tuning(TUNING_ALL_440);
				global_tuning = 440.0f;
				indicate_context_setting(0xf0, 3, 100);
				LED_R8_set_byte(0xf0);
			}
			#endif
			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
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
				indicate_context_setting(0x07, 8, 40);
			}
			else if (global_tuning > global_settings.TUNING_MAX) //GLOBAL_TUNING_MAX
			{
				global_tuning = global_settings.TUNING_MAX; //GLOBAL_TUNING_MAX
				indicate_context_setting(0xe0, 8, 40);
			}
			else
			{
				indicate_context_setting(direction==1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
			}
		}

		//printf("context_menu_action(): new TUNING = %f\n", global_tuning);
		set_tuning_all(global_tuning);
		persistent_settings_store_tuning();
	}
	else if (level == CONTEXT_MENU_TEMPO)
	{
		//printf("context_menu_action(): adjusting TEMPO\n");

		if (!direction || direction == 2)
		{
			settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
			tempo_bpm = global_settings.TEMPO_BPM_DEFAULT;
			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
			//indicate_context_setting(0x81, 2, 50);
			indicate_context_setting(0x42, 2, 50);
			indicate_context_setting(0x24, 2, 50);
			indicate_context_setting(0x18, 2, 50);
			settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
		}
		else
		{
			tempo_bpm += direction * global_settings.TEMPO_BPM_STEP;

			if (tempo_bpm < global_settings.TEMPO_BPM_MIN)
			{
				tempo_bpm = global_settings.TEMPO_BPM_MIN;
				indicate_context_setting(0x07, 8, 40);
			}
			else if (tempo_bpm > global_settings.TEMPO_BPM_MAX)
			{
				tempo_bpm = global_settings.TEMPO_BPM_MAX;
				indicate_context_setting(0xe0, 8, 40);
			}
			else
			{
				indicate_context_setting(direction==1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);
			}
		}

		TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
		MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;
		DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

		//printf("context_menu_action(): new BPM=%d, TEMPO_BY_SAMPLE=%d, MELODY_BY_SAMPLE=%d, DELAY_BY_TEMPO=%d\n", tempo_bpm, TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO);

		persistent_settings_store_tempo();
	}
	else if (level == CONTEXT_MENU_TRANSPOSE)
	{
		//printf("context_menu_action(): adjusting TRANSPOSE\n");

		if (!direction || direction == 2)
		{
			settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;

			transpose_song(-persistent_settings.TRANSPOSE);
			persistent_settings.TRANSPOSE = 0;

			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
			indicate_context_setting(0x42, 2, 50);
			indicate_context_setting(0x24, 2, 50);
			indicate_context_setting(0x18, 2, 50);
			settings_indication_in_progress = SETTINGS_INDICATION_TIMEOUT;
		}
		else
		{
			persistent_settings.TRANSPOSE += direction;
			indicate_context_setting(direction==1?SETTINGS_INDICATOR_ANIMATE_RIGHT_4:SETTINGS_INDICATOR_ANIMATE_LEFT_4, 1, 50);

			transpose_song(direction);
		}
		//printf("context_menu_action(): new TRANSPOSE=%d\n", persistent_settings.TRANSPOSE);
		persistent_settings.TRANSPOSE_updated = 1;
		persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER_LONG;
	}
	#ifdef BOARD_GECHO
	else if (level == CONTEXT_MENU_S1_S2)
	{
		//printf("context_menu_action(): adjusting override S1/S2\n");

		if (!direction || direction == 2) //long press button #1 or #2
		{
			sensors_override &= 0xffff0000;
		}
		else if (direction == 1) //short press button #2
		{
			//S2 - orange
			uint8_t b = sensors_override>>8;
			if(!b)
			{
				b = 1;
			}
			else
			{
				b <<= 1;
				if(b & 0x10)
				{
					b = 0;
				}
			}
			sensors_override = (sensors_override & 0xffff00ff) | (((uint32_t)b)<<8);
			printf("sensors_override = %x\n", sensors_override);
		}
		else if (direction == -1) //short press button #1
		{
			//S1 - red
			uint8_t b = sensors_override;
			if(!b)
			{
				b = 1;
			}
			else
			{
				b <<= 1;
			}
			sensors_override = (sensors_override & 0xffffff00) | b;
			printf("sensors_override = %x\n", sensors_override);
		}
	}
	else if (level == CONTEXT_MENU_S3_S4)
	{
		//printf("context_menu_action(): adjusting override S3/S4\n");

		if (!direction || direction == 2) //long press button #3 or #4
		{
			sensors_override &= 0x0000ffff;
		}
		else if (direction == 1) //short press button #4
		{
			//S4 - white
			uint8_t b = sensors_override>>24;
			if(!b)
			{
				b = 1;
			}
			else
			{
				b <<= 1;
			}
			sensors_override = (sensors_override & 0x00ffffff) | (((uint32_t)b)<<24);
			printf("sensors_override = %x\n", sensors_override);
		}
		else if (direction == -1) //short press button #3
		{
			//S3 - blue
			uint8_t b = sensors_override>>16;
			if(!b)
			{
				b = 1;
			}
			else
			{
				b <<= 1;
				if(b & 0x20)
				{
					b = 0;
				}
			}
			sensors_override = (sensors_override & 0xff00ffff) | (((uint32_t)b)<<16);
			printf("sensors_override = %x\n", sensors_override);
		}
	}
	#endif

	#ifdef BOARD_WHALE
	else if (level == CONTEXT_MENU_ECHO_ARP)
	{
		//printf("context_menu_action(): adjusting ECHO / ARPEGGIATOR\n");

		if (!direction || direction == 2)
		{
			#ifdef BOARD_WHALE
			fixed_arp_level = 0;
			echo_dynamic_loop_current_step = 0;
			#else
			if (!direction)
			{
				fixed_arp_level = 0;
			}
			else
			{
				echo_dynamic_loop_current_step = 0;
			}
			#endif
			next_event_lock = 1; //wait till all buttons released to not trigger duplicate reset to default event
			indicate_context_setting(0x18, 3, 100);
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
	#endif
}

int main_menu_action(int level, int button)
{
	printf("main_menu_action(level=%d, button=%d)\n",level,button);
	LED_R8_all_OFF();

	if(level==MAIN_MENU_MIDI_CTRL_SETUP_LEVEL && button==MAIN_MENU_MIDI_CTRL_SETUP_BTN)
	{
		printf("main_menu_action(): MAIN_MENU_MIDI_CTRL_SETUP\n");
		setup_MIDI_controls();
	}

	if(level==MAIN_MENU_MIDI_CTRL_RESET_LEVEL && button==MAIN_MENU_MIDI_CTRL_RESET_BTN)
	{
		printf("main_menu_action(): MAIN_MENU_MIDI_CTRL_RESET\n");
		reset_MIDI_controls();
	}

	if(level==MAIN_MENU_MIDI_SYNC_MODE_LEVEL && button==MAIN_MENU_MIDI_SYNC_MODE_BTN)
	{
		printf("main_menu_action(): MAIN_MENU_MIDI_SYNC_MODE\n");

		midi_sync_mode++;
		if(midi_sync_mode==MIDI_SYNC_MODES)
		{
			midi_sync_mode = 0;
			indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 1, 50);
		}
		else
		{
			LED_R8_set_byte(1<<(midi_sync_mode-1));
		}
		persistent_settings.MIDI_SYNC_MODE = midi_sync_mode;
		persistent_settings.MIDI_SYNC_MODE_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down
		main_menu_restart_on_close = 1;
	}

	if(level==MAIN_MENU_MIDI_POLYPHONY_LEVEL && button==MAIN_MENU_MIDI_POLYPHONY_BTN)
	{
		printf("main_menu_action(): MAIN_MENU_MIDI_POLYPHONY\n");

		midi_polyphony++;
		if(midi_polyphony==MIDI_POLYPHONY_MODES)
		{
			midi_polyphony = 0;
		}
		indicate_context_setting(SETTINGS_INDICATOR_MIDI_POLYPHONY+midi_polyphony, MIDI_POLYPHONY_HOLD_TYPE?8:1, MIDI_POLYPHONY_HOLD_TYPE?50:0);

		persistent_settings.MIDI_POLYPHONY = midi_polyphony;
		persistent_settings.MIDI_POLYPHONY_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down
		//main_menu_restart_on_close = 1;
	}

	if(level==MAIN_MENU_AGC_ENABLE_LEVEL && button==MAIN_MENU_AGC_ENABLE_BTN)
	{
		printf("main_menu_action(): MAIN_MENU_AGC_ENABLE\n");

		//find the correct pointer location
		int AGC_levels_ptr = 0;
		for(int i=0;i<AGC_LEVELS;i++)
		{
			if(AGC_levels[i]==persistent_settings.AGC_ENABLED_OR_PGA)
			{
				AGC_levels_ptr = i;
				break;
			}
		}

		//move on to the next one
		AGC_levels_ptr++;

		if(AGC_levels_ptr==AGC_LEVELS)
		{
			AGC_levels_ptr = 0;
		}

		persistent_settings.AGC_ENABLED_OR_PGA = AGC_levels[AGC_levels_ptr];

		if(persistent_settings.AGC_ENABLED_OR_PGA)
		{
			LED_R8_set_byte(AGC_levels_indication[AGC_levels_ptr]);
		}
		else //AGC off
		{
			LED_R8_set_byte(0xff);
			Delay(100);
			LED_R8_set_byte(0x7f);
			Delay(100);
			LED_R8_set_byte(0x3f);
			Delay(100);
			LED_R8_set_byte(0x1f);
			Delay(100);
			LED_R8_set_byte(0x0f);
			Delay(100);
			LED_R8_set_byte(0x07);
			Delay(100);
			LED_R8_set_byte(0x03);
			Delay(100);
			LED_R8_set_byte(0x00);
		}

		//codec_configure_AGC(persistent_settings.AGC_ENABLED_OR_PGA, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
		codec_configure_AGC(persistent_settings.AGC_ENABLED_OR_PGA, persistent_settings.AGC_MAX_GAIN, persistent_settings.AGC_ENABLED_OR_PGA);
		printf("main_menu_action(): AGC_ENABLED_OR_PGA set to %d\n", persistent_settings.AGC_ENABLED_OR_PGA);

		persistent_settings.AGC_ENABLED_OR_PGA_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down
		//main_menu_restart_on_close = 1;
	}

	if(level==MAIN_MENU_SENSORS_CONFIG_LEVEL && button==MAIN_MENU_SENSORS_CONFIG_BTN)
	{
		printf("main_menu_action(): use_acc_or_ir_sensors\n");
		if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
		{
			set_controls(PARAMETER_CONTROL_SENSORS_IRS, 0); //switch to IR sensors, do not update NVS immediately
		}
		else
		{
			set_controls(PARAMETER_CONTROL_SENSORS_ACCELEROMETER, 0); //switch to accelerometer, do not update NVS immediately
		}
		return 1; //restore sub-menu indication
	}

	if(level==MAIN_MENU_LEDS_OFF_LEVEL && button==MAIN_MENU_LEDS_OFF_BTN)
	{
		printf("main_menu_action(): persistent_settings.ALL_LEDS_OFF\n");
		if(persistent_settings.ALL_LEDS_OFF)
		{
			persistent_settings.ALL_LEDS_OFF = 0;
			Delay(2*EXPANDERS_TIMING_DELAY);
			indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_ON, 1, 50);
		}
		else
		{
			indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_OFF, 1, 50);

			LEDs_all_OFF();
			LED_RDY_OFF;
			LED_SIG_OFF;
			Delay(EXPANDERS_TIMING_DELAY);
			LEDs_all_OFF();
			LED_RDY_OFF;
			LED_SIG_OFF;
			Delay(EXPANDERS_TIMING_DELAY);
			persistent_settings.ALL_LEDS_OFF = 1;
		}

		//persistent_settings.ALL_LEDS_OFF_updated = 1; //do not store to NVS
		//persistent_settings.update = 0; //to avoid duplicate update if timer already running down
		//main_menu_restart_on_close = 1;

		printf("main_menu_action(): ALL_LEDS_OFF set to %d\n", persistent_settings.ALL_LEDS_OFF);

		return 1; //restore sub-menu indication
	}

	if(level==MAIN_MENU_SAMPLING_RATE_LEVEL && button==MAIN_MENU_SAMPLING_RATE_BTN)
	{
		printf("main_menu_action(): persistent_settings.SAMPLING_RATE\n");

		sampling_rates_ptr++;
		if(sampling_rates_ptr == SAMPLING_RATES)
		{
			sampling_rates_ptr = 0;
		}
		persistent_settings.SAMPLING_RATE = sampling_rates[sampling_rates_ptr];
		set_sampling_rate(persistent_settings.SAMPLING_RATE);
		LED_R8_set_byte(sampling_rates_indication[sampling_rates_ptr]);

		persistent_settings.SAMPLING_RATE_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down

		printf("main_menu_action(): SAMPLING_RATE => %u\n", persistent_settings.SAMPLING_RATE);
	}

	if(level==MAIN_MENU_AUTO_POWER_OFF_LEVEL && button==MAIN_MENU_AUTO_POWER_OFF_BTN)
	{
		printf("main_menu_action(): persistent_settings.AUTO_POWER_OFF\n");

		persistent_settings.AUTO_POWER_OFF++;
		if(persistent_settings.AUTO_POWER_OFF > global_settings.AUTO_POWER_OFF_TIMEOUT/600)
		{
			persistent_settings.AUTO_POWER_OFF = 0;
			indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 1, 50);
		}
		else
		{
			LED_R8_set_byte(0x01<<(persistent_settings.AUTO_POWER_OFF-1));
		}

		persistent_settings.AUTO_POWER_OFF_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down

		printf("main_menu_action(): AUTO_POWER_OFF => %u (%u sec)\n", persistent_settings.AUTO_POWER_OFF, persistent_settings.AUTO_POWER_OFF*600);
	}

	if(level==MAIN_MENU_SD_CARD_SPEED_LEVEL && button==MAIN_MENU_SD_CARD_SPEED_BTN)
	{
		printf("main_menu_action(): persistent_settings.SD_CARD_SPEED\n");

		persistent_settings.SD_CARD_SPEED = 1 - persistent_settings.SD_CARD_SPEED;
		LED_R8_set_byte(0x0f>>(1-persistent_settings.SD_CARD_SPEED)*2);

		persistent_settings.SD_CARD_SPEED_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down

		printf("main_menu_action(): SD_CARD_SPEED => %d (%d MHz)\n", persistent_settings.SD_CARD_SPEED, 20+20*persistent_settings.SD_CARD_SPEED);
	}

	if(level==MAIN_MENU_ACC_ORIENTATION_LEVEL && button==MAIN_MENU_ACC_ORIENTATION_BTN)
	{
		printf("main_menu_action(): persistent_settings.ACC_ORIENTATION\n");

		persistent_settings.ACC_ORIENTATION++;
		if(persistent_settings.ACC_ORIENTATION >= ACC_ORIENTATION_MODES)
		{
			persistent_settings.ACC_ORIENTATION = 0;
		}
		//LED_R8_set_byte(0x07<<persistent_settings.ACC_ORIENTATION);
		LED_R8_set_byte(acc_orientation_indication[persistent_settings.ACC_ORIENTATION]);

		persistent_settings.ACC_ORIENTATION_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down

		printf("main_menu_action(): ACC_ORIENTATION => %u\n", persistent_settings.ACC_ORIENTATION);
	}

	if(level==MAIN_MENU_ACC_INVERT_LEVEL && button==MAIN_MENU_ACC_INVERT_BTN)
	{
		printf("main_menu_action(): persistent_settings.ACC_INVERT\n");

		persistent_settings.ACC_INVERT++;
		if(persistent_settings.ACC_INVERT >= ACC_INVERT_MODES)
		{
			persistent_settings.ACC_INVERT = 0;
		}
		LED_R8_set_byte(acc_invert_indication[persistent_settings.ACC_INVERT]);

		persistent_settings.ACC_INVERT_updated = 1;
		persistent_settings.update = 0; //to avoid duplicate update if timer already running down

		printf("main_menu_action(): ACC_INVERT => %u\n", persistent_settings.ACC_INVERT);
	}

	return 0;
}

uint8_t button_SET_state = 0;
uint8_t button_RST_state = 0;
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
	if (x==3)return BUTTON_U4_ON;
#endif
	return -1;
}

#ifdef BOARD_GECHO

#define SCAN_BUTTONS_RESULT_NONE		-1
#define SCAN_BUTTONS_RESULT_BTN1		0
#define SCAN_BUTTONS_RESULT_BTN2		1
#define SCAN_BUTTONS_RESULT_BTN3		2
#define SCAN_BUTTONS_RESULT_BTN4		3
#define SCAN_BUTTONS_RESULT_SET_100MS	4
#define SCAN_BUTTONS_RESULT_SET_1S		5
#define SCAN_BUTTONS_RESULT_SET_3S		6
#define SCAN_BUTTONS_RESULT_RST_100MS	7
#define SCAN_BUTTONS_RESULT_RST_1S		8
#define SCAN_BUTTONS_RESULT_RST_3S		9
#define SCAN_BUTTONS_RESULT_SET_RST		10
#define SCAN_BUTTONS_RESULT_RST_SET		11

int scan_buttons()
{
	for(int i=0;i<4;i++)
	{
		if (BUTTON_Ux_ON(i))
		{
			if (!button_user_state[i])
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

	if (BUTTON_SET_ON) //simpler
	{
		button_SET_state++;

		if (button_SET_state >= SET_RST_BUTTON_THRESHOLD_1s)
		{
			button_SET_state = 0;
			return SCAN_BUTTONS_RESULT_SET_1S;
		}
		else if (button_SET_state == SET_RST_BUTTON_THRESHOLD_3s)
		{
			button_SET_state = 0;
			return SCAN_BUTTONS_RESULT_SET_3S;
		}
	}
	else
	{
		if (button_SET_state >= SET_RST_BUTTON_THRESHOLD_100ms)
		{
			button_SET_state = 0;
			if(button_RST_state)
			{
				while(BUTTON_RST_ON) { Delay(10); } //wait till RST released as well
				button_RST_state = 0;
				return SCAN_BUTTONS_RESULT_RST_SET;
			}
			else
			{
				return SCAN_BUTTONS_RESULT_SET_100MS;
			}
		}

		button_SET_state = 0;
	}

	if (BUTTON_RST_ON)
	{
		button_RST_state++;

		if (button_RST_state >= SET_RST_BUTTON_THRESHOLD_1s)
		{
			button_RST_state = 0;
			return SCAN_BUTTONS_RESULT_RST_1S;
		}
		else if (button_RST_state == SET_RST_BUTTON_THRESHOLD_3s)
		{
			button_RST_state = 0;
			return SCAN_BUTTONS_RESULT_RST_3S;
		}
	}
	else
	{
		if (button_RST_state >= SET_RST_BUTTON_THRESHOLD_100ms)
		{
			button_RST_state = 0;
			if(button_SET_state)
			{
				while(BUTTON_SET_ON) { Delay(10); } //wait till SET released as well
				button_SET_state = 0;
				return SCAN_BUTTONS_RESULT_SET_RST;
			}
			else
			{
				return SCAN_BUTTONS_RESULT_RST_100MS;
			}
		}

		button_RST_state = 0;
	}

	return SCAN_BUTTONS_RESULT_NONE;
}

uint64_t get_user_buttons_sequence(int *press_order_ptr, int *press_order)
{
	uint64_t res = 0;
	if (press_order_ptr[0] > 0) //if some program chosen, run it!
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
	if (binary_status_O4[0] & (0x01 << button))
	{
		LED_O4_set(button, 1); //set n-th LED on
	}
	else
	{
		LED_O4_set(button, 0); //set n-th LED off
	}
	if (press_order_ptr[0]<PRESS_ORDER_MAX)
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

	printf("select_channel(): tempo_detect_success  = %d, start_channel_by_sync_pulse = %d\n", tempo_detect_success, start_channel_by_sync_pulse);

	button_SET_state = 0;

	while(1)
	{
		//new_random_value(); //shuffle the random value for better entropy

		if (select_channel_RDY_idle_blink)
		{
			if (loop % (1000/SCAN_BUTTONS_LOOP_DELAY) == 0)
			{
				LED_RDY_ON;
			}
			if (loop % (2000/SCAN_BUTTONS_LOOP_DELAY) == (1000/SCAN_BUTTONS_LOOP_DELAY))
			{
				LED_RDY_OFF;
			}
		}

		loop++;

		if(press_order_ptr && tempo_detect_success && start_channel_by_sync_pulse)
		{
			result = get_user_buttons_sequence(&press_order_ptr, press_order);
			printf("select_channel(): start by sync, result = %llu\n", result);
			return result;
		}

		if ((button = scan_buttons()) > -1) //some button was just pressed
		{
			//printf("select_channel(): scan_buttons() returned code = %d\n", button);

			if(tempo_detect_success)
			{
				start_channel_by_sync_pulse = 0;
			}

			if (button <= SCAN_BUTTONS_RESULT_BTN4) //user button 1-4 pressed
			{
				printf("select_channel(): button %d pressed\n", button+1);
				user_button_pressed(button, &binary_status_O4, &press_order_ptr, press_order);
			}
			else if (button == SCAN_BUTTONS_RESULT_SET_100MS) //SET pressed for 100ms
			{
				result = get_user_buttons_sequence(&press_order_ptr, press_order);
				if (result>0)
				{
					printf("select_channel(): result = %llu\n", result);
					return result;
				}
				else
				{
					printf("select_channel(): entering main menu\n");
					main_menu_active = MAIN_MENU_LEVEL_0;
					while(main_menu_active)
					{
						loop++;
						if(main_menu_active == MAIN_MENU_LEVEL_0)
						{
							if(loop%2==0)
							{
								LED_O4_set_byte(0x0f);
							}
							else
							{
								LED_O4_all_OFF();
							}
						}
						Delay(50);
						if ((button = scan_buttons()) > -1) //some button was just pressed
						{
							if (button == SCAN_BUTTONS_RESULT_RST_100MS
							|| (main_menu_active == MAIN_MENU_LEVEL_0 && button == SCAN_BUTTONS_RESULT_SET_100MS)) //RST pressed for 100ms, or SET while in level 0
							{
								main_menu_active = 0;
								LEDs_all_OFF();
								printf("select_channel(): storing persistent settings\n");
								store_persistent_settings(&persistent_settings);
								printf("select_channel(): exiting main menu\n");
								if(main_menu_restart_on_close)
								{
									printf("select_channel(): restart required after a main menu setting updated\n");
									esp_restart();
								}
							}
							else if (main_menu_active != MAIN_MENU_LEVEL_0 && button == SCAN_BUTTONS_RESULT_SET_100MS) //SET pressed for 100ms
							{
								main_menu_active = MAIN_MENU_LEVEL_0;
								LED_R8_all_OFF();
								LED_B5_all_OFF();
								LED_W8_all_OFF();
							}
							else if (button <= SCAN_BUTTONS_RESULT_BTN4) //user button 1-4 pressed
							{
								if(main_menu_active == MAIN_MENU_LEVEL_0)
								{
									main_menu_active = button + 1;
									LED_O4_set_byte(0x01<<button);
									LED_R8_all_OFF();
									printf("select_channel(): main menu %d selected\n", main_menu_active);
								}
								else //a command within selected sub-menu
								{
									printf("select_channel(): main menu = %d, command = %d\n", main_menu_active, button);
									if(main_menu_action(main_menu_active, button+1)) //button codes are numbered from 0 here
									{
										//restore sub-menu indication
										LEDs_all_OFF();
										LED_O4_set_byte(0x01<<(main_menu_active-1));
									}
								}
							}
						}
					}
				}
			}
			else if (button == SCAN_BUTTONS_RESULT_SET_RST) //SET-RST combination pressed
			{
				result = global_settings.IDLE_SET_RST_SHORTCUT;
				printf("select_channel(): result = %llu\n", result);
				return result;
			}
			else if (button == SCAN_BUTTONS_RESULT_RST_SET) //RST-SET combination pressed
			{
				result = global_settings.IDLE_RST_SET_SHORTCUT;
				printf("select_channel(): result = %llu\n", result);
				return result;
			}

			else if (button == SCAN_BUTTONS_RESULT_SET_1S) //SET pressed for 1 second
			{
				result = global_settings.IDLE_LONG_SET_SHORTCUT;
				printf("select_channel(): result = %llu\n", result);
				return result;
			}
			/*
			else if (button == SCAN_BUTTONS_RESULT_SET_3S) //SET pressed for 3 seconds
			{
				return 0; //mcu_and_codec_shutdown();
			}
			*/
			else if (button == SCAN_BUTTONS_RESULT_RST_100MS) //RST pressed for 100ms
			{
				printf("rst_button(SCAN_BUTTONS_RESULT_RST_100MS) detected, resetting channel number\n");
				binary_status_O4 = 0;
				press_order_ptr = 0;
				LED_O4_all_OFF();
			}
			else if (button == SCAN_BUTTONS_RESULT_RST_1S) //RST pressed for 1 second, restart
			{
				printf("rst_button(SCAN_BUTTONS_RESULT_RST_1S) detected, restarting\n");
				codec_set_mute(1); //mute the codec
				Delay(100);
				codec_reset();
				stop_MCLK();
				Delay(10);
				esp_restart();
			}
		}

		Delay(SCAN_BUTTONS_LOOP_DELAY);

		//new_random_value(); //get some entropy
	}
}

uint64_t get_user_number()
{
	int button;
	int binary_status_O4 = 0;
	int press_order[PRESS_ORDER_MAX], press_order_ptr = 0;
	uint64_t result = 0; //2^64 = 18446744073709551616 -> max 20 digits (or 19 if can start with 4)

	button_SET_state = 0;

	for(int i=0;i<3;i++)
	{
		LED_O4_set_byte(0x0f);
		Delay(100);
		LED_O4_set_byte(0x00);
		Delay(100);
	}

	while(1)
	{
		if ((button = scan_buttons()) > -1) //some button was just pressed
		{
			printf("get_user_number(): scan_buttons() returned code = %d\n", button);

			if (button <= SCAN_BUTTONS_RESULT_BTN4) //user button 1-4 pressed
			{
				printf("get_user_number(): button %d pressed\n", button+1);
				user_button_pressed(button, &binary_status_O4, &press_order_ptr, press_order);

				LED_R8_set_byte(0x00ff>>(8-(press_order_ptr%8)));
			}
			else if (button == SCAN_BUTTONS_RESULT_SET_100MS) //SET pressed for 100ms
			{
				result = get_user_buttons_sequence(&press_order_ptr, press_order);
				printf("get_user_number(): result = %llu\n", result);
				return result;
			}
			else if (button == SCAN_BUTTONS_RESULT_RST_100MS) //RST pressed for 100ms
			{
				printf("get_user_number(): RST detected, cancelling\n");
				binary_status_O4 = 0;
				press_order_ptr = 0;
				LED_O4_all_OFF();
				return GET_USER_NUMBER_CANCELED;
			}
			else if (button == SCAN_BUTTONS_RESULT_RST_1S) //RST pressed for 1 second, restart
			{
				printf("rst_button(SCAN_BUTTONS_RESULT_RST_1S) detected, restarting\n");
				esp_restart();
			}
		}

		Delay(SCAN_BUTTONS_LOOP_DELAY);
	}
}
#endif

int get_tempo_by_BPM(int bpm)
{
	int b = SAMPLE_RATE_DEFAULT; //120BPM (16s / loop);

	if (bpm == 120)
	{
		return b;
	}

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

	button_play = 0;
	play_button_cnt = 0;
	play_and_plus_button_cnt = 0;

	short_press_button_play = 0;
	long_press_button_play = 0;
	long_press_button_play_and_plus = 0;

	short_press_seq_cnt = 0;
	short_press_seq_ptr = 0;
	short_press_sequence = 0;
}

void buttons_sequence_check(int event)
{
	if (context_menu_active)
	{
		return;
	}

	if (!event) //periodical check
	{
		if (!short_press_seq_cnt && short_press_seq_ptr) //too late, reset everything
		{
			printf("buttons_sequence_check(): reset\n");
			short_press_seq_ptr = 0;
			return;
		}

		printf("buttons_sequence_check(): check for sequence: [%s], len=%d, counter=%d\n", short_press_seq, short_press_seq_ptr, short_press_seq_cnt);

		//check for expected sequences
		if (short_press_seq_ptr==8 && !strncmp(short_press_seq,"abbbccca", 8))
		{
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: abbbccca\n");
			voice_say("this_is_glo");
			//voice_say("upgrade_your_perception");
			factory_reset();
		}
		//else if (!strncmp(short_press_seq,"bbbbcccc", 8))
		//{
		//	printf("buttons_sequence_check(): sequence detected: bbbbcccc\n");
			//while(1);
		//}
		else if (short_press_seq_ptr==6 && !strncmp(short_press_seq,"bcbcbc", 6))
		{
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bcbcbc\n");
			beeps_enabled = 1 - beeps_enabled;
			persistent_settings.BEEPS = beeps_enabled;
			persistent_settings.BEEPS_updated = 1;
			persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
			printf("beeps_enabled = %d\n", beeps_enabled);
		}
		else if (short_press_seq_ptr==5 && !strncmp(short_press_seq,"bbbbb", 5)) //increase analog volume by 6db, up to 0dB, maximum level
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bbbbb\n");
			persistent_settings.ANALOG_VOLUME -= ANALOG_VOLUME_STEP; //negative logic, means increase
			if (persistent_settings.ANALOG_VOLUME < 0)
			{
				persistent_settings.ANALOG_VOLUME = 0;
			}
			persistent_settings.ANALOG_VOLUME_updated = 1;
			codec_analog_volume = persistent_settings.ANALOG_VOLUME;
			codec_set_analog_volume();
			store_persistent_settings(&persistent_settings);
			printf("buttons_sequence_check(): ANALOG_VOLUME increased by %ddB, current value = %d(%ddB)\n", ANALOG_VOLUME_STEP/2, persistent_settings.ANALOG_VOLUME, -persistent_settings.ANALOG_VOLUME/2);
		}
		else if (short_press_seq_ptr==5 && !strncmp(short_press_seq,"ccccc", 5)) //decrease analog volume by 6db, down to minimum level
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: ccccc\n");
			persistent_settings.ANALOG_VOLUME += ANALOG_VOLUME_STEP;
			if (persistent_settings.ANALOG_VOLUME > CODEC_ANALOG_VOLUME_MIN)
			{
				persistent_settings.ANALOG_VOLUME = CODEC_ANALOG_VOLUME_MIN;
			}
			persistent_settings.ANALOG_VOLUME_updated = 1;
			codec_analog_volume = persistent_settings.ANALOG_VOLUME;
			codec_set_analog_volume();
			store_persistent_settings(&persistent_settings);
			printf("buttons_sequence_check(): ANALOG_VOLUME decreased by %ddB, current value = %d(%ddB)\n", ANALOG_VOLUME_STEP/2, persistent_settings.ANALOG_VOLUME, -persistent_settings.ANALOG_VOLUME/2);
		}

		else if (short_press_seq_ptr==6 && !strncmp(short_press_seq,"bbbbbb", 6)) //increase AGC max gain by 3db
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: bbbbbb\n");

			if (global_settings.AGC_MAX_GAIN<=global_settings.AGC_MAX_GAIN_LIMIT-global_settings.AGC_MAX_GAIN_STEP)
			{
				global_settings.AGC_MAX_GAIN += global_settings.AGC_MAX_GAIN_STEP;
			}

			codec_configure_AGC(global_settings.AGC_ENABLED, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
			printf("buttons_sequence_check(): AGC_MAX_GAIN increased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, global_settings.AGC_MAX_GAIN);
		}
		else if (short_press_seq_ptr==6 && !strncmp(short_press_seq,"cccccc", 6)) //decrease AGC max gain by 3db
		{
			short_press_seq_ptr = 0;
			short_press_button_play = 0;
			printf("buttons_sequence_check(): sequence detected: cccccc\n");

			if (global_settings.AGC_MAX_GAIN>=global_settings.AGC_MAX_GAIN_STEP)
			{
				global_settings.AGC_MAX_GAIN -= global_settings.AGC_MAX_GAIN_STEP;
			}

			codec_configure_AGC(global_settings.AGC_ENABLED, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
			printf("buttons_sequence_check(): AGC_MAX_GAIN decreased by %ddB, current value = %ddB\n", global_settings.AGC_MAX_GAIN_STEP, global_settings.AGC_MAX_GAIN);
		}

		else if (short_press_seq_ptr==9 && !strncmp(short_press_seq,"cbcbcbcba", 9))
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
		else if (short_press_seq_ptr==1 && short_press_seq[0]=='b')
		{
			short_press_volume_plus = 1;
		}
		else if (short_press_seq_ptr==1 && short_press_seq[0]=='c')
		{
			short_press_volume_minus = 1;
		}
		else if (short_press_seq_ptr==2 && !strncmp(short_press_seq,"bb", 2))
		{
			short_press_sequence = 2;
		}
		else if (short_press_seq_ptr==2 && !strncmp(short_press_seq,"cc", 2))
		{
			short_press_sequence = -2;
		}
		else if (short_press_seq_ptr==2 && !strncmp(short_press_seq,"bc", 2))
		{
			short_press_sequence = SEQ_PLUS_MINUS;
		}
		else if (short_press_seq_ptr==2 && !strncmp(short_press_seq,"cb", 2))
		{
			short_press_sequence = SEQ_MINUS_PLUS;
		}
		else if (short_press_seq_ptr==3 && !strncmp(short_press_seq,"bbb", 3))
		{
			short_press_sequence = 3;
		}
		else if (short_press_seq_ptr==3 && !strncmp(short_press_seq,"ccc", 3))
		{
			short_press_sequence = -3;
		}
		else if (short_press_seq_ptr==4 && !strncmp(short_press_seq,"bbbb", 4))
		{
			short_press_sequence = 4;
		}
		else if (short_press_seq_ptr==4 && !strncmp(short_press_seq,"cccc", 4))
		{
			short_press_sequence = -4;
		}

		if (short_press_seq_cnt==1) //only reset if timeout reached
		{
			short_press_seq_ptr = 0;
		}
	}
	else //if (event>=1 && event<=3)
	{
		if (short_press_seq_ptr<SHORT_PRESS_SEQ_MAX)
		{
			short_press_seq[short_press_seq_ptr] = 'a' + event - 1;
			//printf("buttons_sequence_check(): new event: %c\n", short_press_seq[short_press_seq_ptr]);
			short_press_seq_ptr++;
			short_press_seq_cnt = BUTTONS_SEQUENCE_TIMEOUT;

			//if (short_press_seq_ptr == SHORT_PRESS_SEQ_MAX) //maximum length reached, decode the sequence now
			if (short_press_seq_ptr >= 8) //decode the sequence immediately if at least 8 keys recorded
			{
				buttons_sequence_check(0);
			}
		}
	}
}

void process_buttons_controls_whale(void *pvParameters)
{
	printf("process_buttons_controls_whale(): started on core ID=%d\n", xPortGetCoreID());

	while (1)
	{
		Delay(BUTTONS_CNT_DELAY);

		if (channel_running)
		{
			//printf("process_buttons_controls_whale(): running on core ID=%d\n", xPortGetCoreID());

			if (!BUTTON_U1_ON && !BUTTON_U2_ON && !BUTTON_U3_ON) //no buttons pressed at the moment
			{
				buttons_ignore_timeout = 0;
			}

			if (buttons_ignore_timeout)
			{
				buttons_ignore_timeout--;
				continue;
			}

			if (volume_ramp && !settings_menu_active && !context_menu_active)
			{
				//printf("process_buttons_controls_whale(): volume_ramp\n");

				/*
				 if (codec_analog_volume>CODEC_ANALOG_VOLUME_DEFAULT) //actually means lower, as the value meaning is reversed
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
				 if (SAMPLE_VOLUME >= SAMPLE_VOLUME_MAX)
				 {
				 SAMPLE_VOLUME = SAMPLE_VOLUME_MAX;
				 }
				 */

				/*
				 if (codec_analog_volume>CODEC_ANALOG_VOLUME_MAX) //actually means lower, as the value meaning is reversed
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
						if(auto_power_off > AUTO_POWER_OFF_VOLUME_RAMP)
						{
							codec_volume_user = codec_digital_volume;
							persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
							persistent_settings.DIGITAL_VOLUME_updated = 1;
							persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
						}
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
				 if (SAMPLE_VOLUME <= SAMPLE_VOLUME_MIN)
				 {
				 SAMPLE_VOLUME = SAMPLE_VOLUME_MIN;
				 }
				 */

				/*
				 if (codec_analog_volume<CODEC_ANALOG_VOLUME_MIN) //actually means higher, as the value meaning is reversed
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
						if(auto_power_off > AUTO_POWER_OFF_VOLUME_RAMP)
						{
							codec_volume_user = codec_digital_volume;
							persistent_settings.DIGITAL_VOLUME = codec_digital_volume;
							persistent_settings.DIGITAL_VOLUME_updated = 1;
							persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
						}
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
					if (!context_menu_active) { beep(3); }
					short_press_button_play = 1;
					buttons_sequence_check(1);
				}
				else if (button_volume_plus > BUTTONS_SHORT_PRESS && button_volume_plus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_plus(BUTTONS_SHORT_PRESS) detected\n");
					if (!context_menu_active) { beep(4); buttons_sequence_check(2); } else { short_press_volume_plus = 1; }
				}
				else if (button_volume_minus > BUTTONS_SHORT_PRESS && button_volume_minus < VOLUME_BUTTONS_CNT_THRESHOLD)
				{
					printf("button_volume_minus(BUTTONS_SHORT_PRESS) detected\n");
					if (!context_menu_active) { beep(5); buttons_sequence_check(3); } else { short_press_volume_minus = 1; }
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
			if (mclk_enabled && ms10_counter==100)
			{
				stop_MCLK();
			}
			#endif

			ms10_counter++;
			if (persistent_settings.update)
			{
				if (persistent_settings.update==1)
				{
					//good time to save settings
					printf("process_buttons_controls_whale(): store_persistent_settings()\n");
					store_persistent_settings(&persistent_settings);
				}
				persistent_settings.update--;
			}

			//if (ms10_counter % BUTTONS_SEQUENCE_INTERVAL == 0) //every 1200ms
			if (short_press_seq_cnt)
			{
				short_press_seq_cnt--;
				if (short_press_seq_cnt==1)
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

		if (short_press_volume_both && !context_menu_active)
		{
			//printf("if (short_press_volume_both && !context_menu_active)\n");
			short_press_volume_both = 0;
			beep(4);
			if (mics_off) {
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

void check_auto_power_off()
{
	auto_power_off--;

	//printf("check_auto_power_off(): timer = %d\n", auto_power_off);

	if (auto_power_off < AUTO_POWER_OFF_VOLUME_RAMP)
	{
		//codec_digital_volume = codec_volume_user + (int)((1.0f - (float)(auto_power_off) / (float)AUTO_POWER_OFF_VOLUME_RAMP) * ((float)(CODEC_DIGITAL_VOLUME_MIN) / 1.5f - (float)(codec_volume_user)));
		codec_digital_volume = codec_volume_user + ((CODEC_DIGITAL_VOLUME_MIN * 2 * (AUTO_POWER_OFF_VOLUME_RAMP-auto_power_off)) / 3) / AUTO_POWER_OFF_VOLUME_RAMP;
		if(codec_digital_volume > CODEC_DIGITAL_VOLUME_MIN) codec_digital_volume = CODEC_DIGITAL_VOLUME_MIN;
		//printf("auto_power_off=%d, codec_volume_user=%d, codec_digital_volume=%d\n", auto_power_off, codec_volume_user, codec_digital_volume);
		codec_set_digital_volume();
	}

	if (auto_power_off==0)
	{
		whale_shutdown();
	}
}
#endif
