	while(!event_next_channel || beats_selected)
    {
		//t_TIMING_BY_SAMPLE_EVERY_125_MS = TIMING_EVERY_125_MS;
		t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_EVERY_250_MS;

		if(ADC_LR_ENABLED_L)
		{
			sample_f[0] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;
		}
		else		
		{
			sample_f[0] = 0;
		}		

		if(use_bg_sample)
		{
			sample_f[0] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;

			if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
			{
				mixed_sample_buffer_ptr_L = 0;
				SD_card_sample_buffer_half = 2; //trigger reload of the 2nd buffer half
			}
			else if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH/2)
			{
				SD_card_sample_buffer_half = 1; //trigger reload of the 1st buffer half
			}			
		}
		else
		{
			r_noise = r_noise + 19;
			r_noise = r_noise * r_noise;
		}

		if(wind_voices)
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise_and_wind(
				sample_f[0],
				fil->iir2,
				ACTIVE_FILTERS_PAIRS,
				fil->fp.mixing_volumes,
				((uint16_t *)&r_noise)[0],
				noise_volume * noise_boost_by_sensor,
				wind_voices
			) * MAIN_VOLUME;
		}
		else
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
				sample_f[0],
				fil->iir2,
				ACTIVE_FILTERS_PAIRS,
				fil->fp.mixing_volumes,
				((uint16_t *)&r_noise)[0],
				noise_volume * noise_boost_by_sensor
			) * MAIN_VOLUME;
		}

		sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[0]) limiter_debug[0] = sample_i16;
		if(sample_i16>limiter_debug[1]) limiter_debug[1] = sample_i16;
		#endif

		//if(PROG_add_echo)
		//{
			filtered_channel_add_echo();
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[2]) limiter_debug[2] = sample_i16;
		if(sample_i16>limiter_debug[3]) limiter_debug[3] = sample_i16;
		#endif

    	if(use_reverb)
    	{
			if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 41) //4Hz
			{
				filtered_channel_adjust_reverb();
			}

			sample_i16 = (int16_t)((float)add_reverb(sample_i16) * reverb_volume  + (float)sample_i16 * (1.0f-reverb_volume));
    	}
    	else
    	{
			#ifdef DRY_MIX_AFTER_ECHO
    		sample_i16 += (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME);
			#endif
    	}

		sample32 = sample_i16 << 16;

		if(sample_i16 < -DYNAMIC_LIMITER_THRESHOLD && SAMPLE_VOLUME > 0)
		{
			SAMPLE_VOLUME -= DYNAMIC_LIMITER_STEPDOWN;
		}

		if(ADC_LR_ENABLED_R)
		{
			sample_f[1] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;
		}
		else		
		{
			sample_f[1] = 0;
		}		

		#ifdef USE_AUTOCORRELATION
		if(autocorrelation_rec_sample)
		{
			autocorrelation_buffer[--autocorrelation_rec_sample] = (int16_t)ADC_sample;
		}
		#endif

		if(use_bg_sample)
		{
			sample_f[1] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_R++]) / 32768.0f) * mixed_sample_volume;

			if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH)
			{
				mixed_sample_buffer_ptr_R = 0;
			}
		}
		else
		{
			i_noise = r_noise;
			r_noise = r_noise - i_noise;
		}

		if(wind_voices)
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise_and_wind(
				sample_f[1],
				fil->iir2 + FILTERS / 2,
				ACTIVE_FILTERS_PAIRS,
				fil->fp.mixing_volumes + FILTERS / 2,
				((uint16_t *)&r_noise)[1],
				noise_volume * noise_boost_by_sensor,
				wind_voices
			) * MAIN_VOLUME;
		}
		else
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
				sample_f[1],
				fil->iir2 + FILTERS / 2,
				ACTIVE_FILTERS_PAIRS,
				fil->fp.mixing_volumes + FILTERS / 2,
				((uint16_t *)&r_noise)[1],
				noise_volume * noise_boost_by_sensor
			) * MAIN_VOLUME;
		}

		sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[4]) limiter_debug[4] = sample_i16;
		if(sample_i16>limiter_debug[5]) limiter_debug[5] = sample_i16;
		#endif

		//if(PROG_add_echo)
		//{
			filtered_channel_add_echo();
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[6]) limiter_debug[6] = sample_i16;
		if(sample_i16>limiter_debug[7]) limiter_debug[7] = sample_i16;
		#endif

    	if(use_reverb)
    	{
			if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 43) //4Hz
			{
				filtered_channel_adjust_reverb();
			}

			sample_i16 = (int16_t)((float)add_reverb(sample_i16) * reverb_volume + (float)sample_i16 * (1.0f-reverb_volume));
    	}
    	else
    	{
			#ifdef DRY_MIX_AFTER_ECHO
    		sample_i16 += (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);
			#endif
    	}

    	sample32 += sample_i16;

		if(sample_i16 < -DYNAMIC_LIMITER_THRESHOLD && SAMPLE_VOLUME > 0)
		{
			SAMPLE_VOLUME -= DYNAMIC_LIMITER_STEPDOWN;
		}

		#ifdef BOARD_WHALE
		if(add_beep)
		{
			if(volume_ramp)
			{
				sample32 = 0;
			}
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
			}
		}
		#endif

	    i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
	    sd_write_sample(&sample32);

		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);

		if(use_direct_waves_filters)
		{
			//if (t_TIMING_BY_SAMPLE_EVERY_125_MS==21) //8Hz
			//if (t_TIMING_BY_SAMPLE_EVERY_250_MS==47) //4Hz
			if (TIMING_EVERY_100_MS==2467) //10Hz, 365th prime
			{
				if(!midi_override)
				{
					for (int i=0;i<FILTERS/2;i++)
					{
						param_i = PARAMS_ARRAY_BY_ALL_SENSORS * (float)direct_waves_range; //1000.0f;
						if (param_i < 10) 
						{
							param_i = 10;
						}
	
						direct_update_filters_id[i] = i;
						direct_update_filters_freq[i] = param_i;
						
						//printf("f(%d)=%d,",i,param_i);
					}
					//printf("\n");
	
					//printf("fil->start_update_filters_pairs(): updating %d filter pairs...\n",FILTERS/2);
					fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, FILTERS/2); //WAVES_FILTERS);
					//printf("fil->start_update_filters_pairs(): %d filter pairs updated!\n",FILTERS/2);
				}
			}
		}
		else
		{
			melodyCounter--;
			if(melodyCounter==0)
			{
				melodyCounter = MELODY_BY_SAMPLE;

				if(fil->fp.melody_filter_pair >= 0)
				{
					fil->start_next_melody_note();
				}
			}

			if((seconds % SHIFT_CHORD_INTERVAL == 0) && (tempoCounter==SHIFT_CHORD_TIMING_BY_SAMPLE))
			{
				if(!midi_override)
				{
					#ifdef BOARD_GECHO
					display_chord(fil->chord->get_current_chord_LED(), 3);
					#endif
					fil->start_next_chord(wind_voices);
				}
			}

			//if (TIMING_EVERY_100_MS == 13) //10Hz periodically, at given sample
			if (TIMING_EVERY_50_MS == 13) //20Hz periodically, at given sample
			{
				if(wind_voices)
				{
					wind_cutoff_sweep++;
					wind_cutoff_sweep %= WIND_FILTERS;

					wind_cutoff[wind_cutoff_sweep] += 0.005 - (float)(0.01 * ((i_noise) & 0x00000001));
					if (wind_cutoff[wind_cutoff_sweep] < wind_cutoff_limit[0])
					{
						wind_cutoff[wind_cutoff_sweep] = wind_cutoff_limit[0];
					}
					else if (wind_cutoff[wind_cutoff_sweep] > wind_cutoff_limit[1])
					{
						wind_cutoff[wind_cutoff_sweep] = wind_cutoff_limit[1];
					}

					fil->iir2[wind_cutoff_sweep]->setCutoff(wind_cutoff[wind_cutoff_sweep]);
				}
			}			
		}

		if(melodyCounter == SHIFT_ARP_TIMING_BY_SAMPLE)
		{
			if(!use_direct_waves_filters)
			{
				if(!MIDI_controllers_active_PB)
				{
					noise_boost_by_sensor = mixed_sample_volume;
					//printf("[1]noise_boost_by_sensor = mixed_sample_volume;\n");
				}

				if(fixed_arp_level || SENSOR_ARP_ACTIVE_1)
				{
					if(fil->fp.arpeggiator_filter_pair==-1)
					{
						fil->fp.arpeggiator_filter_pair = DEFAULT_ARPEGGIATOR_FILTER_PAIR;
					}
					
					if(midi_override)
					{
						fil->add_to_update_filters_pairs(
							fil->fp.arpeggiator_filter_pair,
							fil->chord->midi_chords_expanded[arpeggiator_loop],
							fil->fp.tuning_arp_l, 
							fil->fp.tuning_arp_r);
					}
					else
					{
						int next_chord = fil->chord->current_chord-1;
						if(next_chord > fil->chord->total_chords)
						{
							next_chord = 0;
						}
						else if(next_chord < 0)
						{
							next_chord = fil->chord->total_chords-1;
						}

						//BUG!!
						//fil->add_to_update_filters_pairs(0,fil->chord->chords[next_chord].freqs[arpeggiator_loop]);
						fil->add_to_update_filters_pairs(fil->fp.arpeggiator_filter_pair,fil->chord->chords[next_chord].freqs[arpeggiator_loop], fil->fp.tuning_arp_l, fil->fp.tuning_arp_r);
					}

					arpeggiator_loop++;
					if(arpeggiator_loop==CHORD_MAX_VOICES)
					{
						arpeggiator_loop=0;
					}

					if(fixed_arp_level || SENSOR_ARP_ACTIVE_2)
					{
						if(fixed_arp_level)
						{
							//new_mixing_vol = 1.0f + ((float)fixed_arp_level / FIXED_ARP_LEVEL_MAX) * 3.333f; //good for wind channel
							new_mixing_vol = 1.0f + ((float)fixed_arp_level / FIXED_ARP_LEVEL_MAX) * ARP_VOLUME_MULTIPLIER_MANUAL; //good for wind channel
						}
						else
						{
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 1.6f; //default for sea and forest
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 2.4f;
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 3.333f; //good for wind channel
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * ARP_VOLUME_MULTIPLIER_DEFAULT; //good for wind channel
							new_mixing_vol = 1.0f + SENSOR_ARP_INCREASE * ARP_VOLUME_MULTIPLIER_DEFAULT; //good for wind channel
						}

						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = new_mixing_vol;
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = new_mixing_vol;

						fil->iir2[fil->fp.arpeggiator_filter_pair]->setResonance(0.999f);
						fil->iir2[fil->fp.arpeggiator_filter_pair+FILTERS/2]->setResonance(0.999f);
					}
				}
				else
				{
					if(fil->fp.arpeggiator_filter_pair>=0)
					{
						//volume back to normal
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = 1.0f;
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = 1.0f;

						//back to default resonance
						fil->iir2[fil->fp.arpeggiator_filter_pair]->setResonanceKeepFeedback(0.99f);
						fil->iir2[fil->fp.arpeggiator_filter_pair+FILTERS/2]->setResonanceKeepFeedback(0.99f); //refactor

						fil->fp.arpeggiator_filter_pair = -1;
					}
				}
			}
		}
		
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 2083) //4Hz, 314th prime
		{
			if(!MIDI_controllers_active_PB)
			{
				//map accelerometer values of y-axis (-1..1) to sample mixing volume (0..1)
				mixed_sample_volume = SENSOR_SAMPLE_MIX_VOL * mixed_sample_volume_coef;
			}

			if(use_direct_waves_filters && !lock_sensors)
			{
				if(use_reverb)
    			{					
					if(SENSOR_REVERB_ACTIVE)
					{
						reverb_volume = 1.0f;
					}
					else
					{
						//acc_res[0] <= -0.2f -> from -1 to -0.2, needs to map to 0..1
						//reverb_volume = (acc_res[0] + 1.0f) * (1/0.8f);
						reverb_volume = SENSOR_REVERB_INCREASE * (1/0.8f);
					}
				}
				else if(!MIDI_controllers_active_PB)
				{
					noise_boost_by_sensor = 3 * (2 - SENSOR_SAMPLE_MIX_VOL);
					//printf("[2]noise_boost_by_sensor = %f\n", noise_boost_by_sensor);
				} 				
			}						
		}

		if (TIMING_EVERY_50_MS == 1399) //20Hz, 222nd prime
		{
			if(MIDI_controllers_active_PB)
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_PB_UPDATED)
				{
					noise_boost_by_sensor = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]/127.0f) * NOISE_BOOST_BY_MIDI_MAX;
					mixed_sample_volume = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]/127.0f) * SAMPLE_VOLUME_BY_MIDI_MAX;
					//printf("MIDI_WHEEL_CONTROLLER_PB => %d, noise_boost_by_sensor = %f, mixed_sample_volume = %f\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB], noise_boost_by_sensor, mixed_sample_volume);
					MIDI_controllers_updated = 0;
				}
			}
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 7919) //4Hz, 1000th prime
		{
			if(!lock_sensors && !echo_dynamic_loop_current_step) //if sensors not locked and a fixed delay is not set, use accelerometer or IR sensors
			{
				if(SENSOR_DELAY_ACTIVE)
				{
					if(SENSOR_DELAY_9)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 64;
					}
					else if(SENSOR_DELAY_8)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 32;
					}
					else if(SENSOR_DELAY_7)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 16;
					}
					else if(SENSOR_DELAY_6)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
					}
					else if(SENSOR_DELAY_5)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
					}
					else if(SENSOR_DELAY_4)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
					}
					else if(SENSOR_DELAY_3)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
					}
					else if(SENSOR_DELAY_2)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 3 * 2;
					}
					else
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO;
					}
					
					if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
					{
						echo_dynamic_loop_length /= 2;
					}
				}
				else
				{
					if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO;
					}
					else //if 120BPM or faster, can expand to 3/2 of the 1-sec buffer length
					{
						echo_dynamic_loop_length = (DELAY_BY_TEMPO * 3) / 2;
					}
				}
				//printf("echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);
			}

			ui_command = 0;

			#define FILTERED_UI_CMD_TUNING_440HZ		1
			#define FILTERED_UI_CMD_TUNING_432HZ		2
			#define FILTERED_UI_CMD_RANDOMIZE_SONG		3
			#define FILTERED_UI_CMD_RESERVED			4
			#define FILTERED_UI_CMD_FILTERS_HIGH_PASS	5
			#define FILTERED_UI_CMD_FILTERS_LOW_PASS	6
			#define FILTERED_UI_CMD_RESONANCE_INCREASE	7
			#define FILTERED_UI_CMD_RESONANCE_DECREASE	8
			#define FILTERED_UI_CMD_SET_RANGE			9
			#define FILTERED_UI_CMD_LOCK_SENSORS		10 
			#define FILTERED_UI_CMD_FILTERS_HIGH_LOW	11			

			//map UI commands
			#ifdef BOARD_WHALE
			
			if(!context_menu_active && !settings_menu_active)
			{
				if(short_press_volume_plus) { ui_command = FILTERED_UI_CMD_TUNING_440HZ; short_press_volume_plus = 0; }			
				if(short_press_volume_minus) { ui_command = FILTERED_UI_CMD_TUNING_432HZ; short_press_volume_minus = 0; }			
				if(short_press_sequence==2) { ui_command = FILTERED_UI_CMD_RANDOMIZE_SONG; short_press_sequence = 0; }
				//if(short_press_sequence==-2) { ui_command = FILTERED_UI_CMD_RESERVED; short_press_sequence = 0; }
				if(short_press_sequence==3) { ui_command = FILTERED_UI_CMD_FILTERS_HIGH_PASS; short_press_sequence = 0; }			
				if(short_press_sequence==-3) { ui_command = FILTERED_UI_CMD_FILTERS_LOW_PASS; short_press_sequence = 0; }
				if(short_press_sequence==4) { ui_command = FILTERED_UI_CMD_RESONANCE_INCREASE; short_press_sequence = 0; }
				if(short_press_sequence==-4) { ui_command = FILTERED_UI_CMD_RESONANCE_DECREASE; short_press_sequence = 0; }
			}
			
			#endif

			#ifdef BOARD_GECHO
			
			if(!context_menu_active && !settings_menu_active)
			{
				if(use_direct_waves_filters)
				{
					if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = FILTERED_UI_CMD_SET_RANGE; btn_event_ext = 0; }
					if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = FILTERED_UI_CMD_LOCK_SENSORS; btn_event_ext = 0; }
				}
				else
				{			
					if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = FILTERED_UI_CMD_FILTERS_HIGH_LOW; btn_event_ext = 0; }
					if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = FILTERED_UI_CMD_RANDOMIZE_SONG; btn_event_ext = 0; }
				}			
			
				//if(short_press_sequence==2) { ui_command = FILTERED_UI_CMD_TUNING_440HZ; short_press_volume_plus = 0;}
				//if(short_press_sequence==-2) { ui_command = FILTERED_UI_CMD_TUNING_432HZ; short_press_volume_minus = 0; }
				//if(short_press_sequence==3) { ui_command = FILTERED_UI_CMD_FILTERS_HIGH_PASS; short_press_sequence = 0; }			
				//if(short_press_sequence==-3) { ui_command = FILTERED_UI_CMD_FILTERS_LOW_PASS; short_press_sequence = 0; }
				//if(short_press_sequence==4) { ui_command = FILTERED_UI_CMD_RESONANCE_INCREASE; short_press_sequence = 0; }
				//if(short_press_sequence==-4) { ui_command = FILTERED_UI_CMD_RESONANCE_DECREASE; short_press_sequence = 0; }
			}
			
			#endif

			if(ui_command==FILTERED_UI_CMD_LOCK_SENSORS)
			{
				lock_sensors = 1 - lock_sensors;
			}

			if(ui_command==FILTERED_UI_CMD_FILTERS_HIGH_LOW)
			{
				if(fil->fp.filters_type & FILTERS_TYPE_HIGH_PASS)
				{
					fil->release();
					fil->setup(FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4);
					printf("fil->setup(): LOW_PASS\n");
				}
				else
				{
					fil->release();
					fil->setup(FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4);
					printf("fil->setup(): HIGH_PASS\n");
				}
			}			

			if(ui_command==FILTERED_UI_CMD_SET_RANGE)
			{
				direct_waves_range_ptr++;
				if(direct_waves_range_ptr==DIRECT_WAVES_RANGES)
				{
					direct_waves_range_ptr = 0;
				}
				direct_waves_range = direct_waves_ranges[direct_waves_range_ptr];
				printf("direct_waves_range => %d\n", direct_waves_range);
				
				//printf("fil->fp.melody_filter_pair = %d, fil->fp.arpeggiator_filter_pair = %d\n",fil->fp.melody_filter_pair, fil->fp.arpeggiator_filter_pair); 
			}
			
			if(ui_command==FILTERED_UI_CMD_TUNING_432HZ)
			{
				set_tuning(TUNING_ALL_432);
				global_tuning = 432.0f;
				printf("Tuning set to %f\n", global_tuning);
				persistent_settings_store_tuning();
			}
			if(ui_command==FILTERED_UI_CMD_TUNING_440HZ)
			{
				set_tuning(TUNING_ALL_440);
				global_tuning = 440.0f;
				printf("Tuning set to %f\n", global_tuning);
				persistent_settings_store_tuning();
			}
			if(ui_command==FILTERED_UI_CMD_RANDOMIZE_SONG)
			{
				codec_set_mute(1); //this takes a while so will stop the sound so there is no buzzing
				randomize_song_and_melody();
				codec_set_mute(0);
				printf("randomize_song_and_melody() done\n");
			}
			if(ui_command==FILTERED_UI_CMD_FILTERS_LOW_PASS)
			{
				fil->release();
				fil->setup(FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4);
				printf("fil->setup(): LOW_PASS\n");
			}
			if(ui_command==FILTERED_UI_CMD_FILTERS_HIGH_PASS)
			{
				fil->release();
				fil->setup(FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4);
				printf("fil->setup(): HIGH_PASS\n");
			}
			if(ui_command==FILTERED_UI_CMD_RESONANCE_DECREASE)
			{
				fil->fp.resonance -= FILTERS_RESONANCE_STEP;
				fil->update_resonance();
				printf("fil->setup(): resonance=%f\n", fil->fp.resonance);
			}
			if(ui_command==FILTERED_UI_CMD_RESONANCE_INCREASE)
			{
				fil->fp.resonance += FILTERS_RESONANCE_STEP;
				if(fil->fp.resonance > FILTERS_RESONANCE_HIGHEST)
				{
					fil->fp.resonance = FILTERS_RESONANCE_HIGHEST;
				}
				fil->update_resonance();
				printf("fil->setup(): resonance=%f\n", fil->fp.resonance);
			}
			
			if(load_SD_card_sample == SD_CARD_SAMPLE_OPENING)
			{				
				printf("FilteredChannels: load_SD_card_sample == SD_CARD_SAMPLE_OPENING\n");
				load_SD_card_sample = load_recorded_sample();
				printf("FilteredChannels: load_SD_card_sample => %d\n", load_SD_card_sample);
			}
		}
		
		/*
		if (TIMING_EVERY_10_MS == 19) //100Hz periodically, at given sample
		{
			if(SD_card_sample_buffer_half && load_SD_card_sample)
			{
				load_next_recorded_sample_sector();
			}
		} 
		*/			

		#ifdef FILTERS_BLOCK_CONTROL_IRS
		if (TIMING_EVERY_100_MS == 17) //10Hz periodically, at given sample
		{
			if(MIDI_controllers_active_CC)
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_CC_UPDATED)
				{
					//fil->fp.resonance = FILTERS_RESONANCE_DEFAULT + ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]/127.0f) * (FILTERS_RESONANCE_ULTRA - FILTERS_RESONANCE_DEFAULT);

					//at half the level we will get full resonance
					fil->fp.resonance = FILTERS_RESONANCE_DEFAULT + ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]/64.0f) * (FILTERS_RESONANCE_ULTRA - FILTERS_RESONANCE_DEFAULT);
					if(fil->fp.resonance>FILTERS_RESONANCE_ULTRA)
					{
						fil->fp.resonance = FILTERS_RESONANCE_ULTRA;						
					}
					
					//and the 2nd half will control arpeggiator
					fixed_arp_level = (float)(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]-64)/64.0f * (float)FIXED_ARP_LEVEL_MAX;
					if(fixed_arp_level<0)
					{
						fixed_arp_level = 0;
					}					

					//printf("MIDI_WHEEL_CONTROLLER_CC => %d, resonance = %f, arp = %d\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC], fil->fp.resonance, fixed_arp_level);
					MIDI_controllers_updated = 0;
					fil->update_resonance();
				}
			}
			else if(!lock_sensors)
			{			
				if(SENSOR_RESO_ACTIVE_5)
				{
					fil->fp.resonance = FILTERS_RESONANCE_ULTRA;
					reso[1] = 5;
				}
				else if(SENSOR_RESO_ACTIVE_4)
				{
					fil->fp.resonance = FILTERS_RESONANCE_HIGHEST;
					reso[1] = 4;
				}			
				else if(SENSOR_RESO_ACTIVE_3)
				{
					fil->fp.resonance = FILTERS_RESONANCE_HIGHER;
					reso[1] = 3;
				}			
				else if(SENSOR_RESO_ACTIVE_2)
				{
					fil->fp.resonance = FILTERS_RESONANCE_HIGH;
					reso[1] = 2;
				}			
				else if(SENSOR_RESO_ACTIVE_1)
				{
					fil->fp.resonance = FILTERS_RESONANCE_MEDIUM;
					reso[1] = 1;
				}			
				else
				{
					fil->fp.resonance = FILTERS_RESONANCE_DEFAULT;
					reso[1] = 0;
				}			
	
				if(reso[1] != reso[0])
				{
					fil->update_resonance();
					//printf("fil->setup(): resonance=%f\n", fil->fp.resonance);
					reso[0] = reso[1];
				}
			}
		}
		#endif

		if (TIMING_EVERY_100_MS == 613) //10Hz periodically, at given sample
		{
			if(!midi_override && MIDI_keys_pressed)
			{
				printf("MIDI active, will receive chords\n");
				midi_override = 1;
				wind_voices = 0;
				fil->fp.melody_filter_pair = -1;
			}		
		}
		
		//if (TIMING_EVERY_40_MS == 47) //25Hz
		if (TIMING_EVERY_20_MS == 47) //50Hz
		{
			if(midi_override && MIDI_notes_updated)
			{
				MIDI_notes_updated = 0;

				LED_W8_all_OFF();
				LED_B5_all_OFF();

				MIDI_to_LED(MIDI_last_chord[0], 1);
				MIDI_to_LED(MIDI_last_chord[1], 1);
				MIDI_to_LED(MIDI_last_chord[2], 1);
				MIDI_to_LED(MIDI_last_chord[3], 1);

				fil->start_next_chord_MIDI(wind_voices, MIDI_last_chord);
			}		
		}		
		
		//if (TIMING_EVERY_10_MS==27) //100Hz
		if (TIMING_EVERY_5_MS==137) //200Hz, 33rd prime
		{
			fil->progress_update_filters(fil, false);

			if(SAMPLE_VOLUME < SAMPLE_VOLUME_DEFAULT)
			{
				SAMPLE_VOLUME += DYNAMIC_LIMITER_RECOVER;
			}
		}

        if(tempoCounter%(TEMPO_BY_SAMPLE/SEQUENCER_INDICATOR_DIV)==0)
        {
        	if(!use_direct_waves_filters)
        	{
        		LED_sequencer_indicators(); //fil->chord->current_chord, fil->chord->total_chords);
        	}
        }
        
        if(beats_selected && TIMING_EVERY_40_MS==17) //25Hz
        {
	        if(!sensor_active[1] /*&& !menu_indicator_active*/ /*&& !settings_indication_in_progress*/ && !context_menu_active) //indicate binaural beats program
			{
				LED_O4_set(beats_selected-1, (binaural_LEDs_timing_seq++)%2);
			}
		}        

		sampleCounter++;
        tempoCounter--;        

		if(tempoCounter==0) //one full second passed
    	{
    		seconds++;
    		sampleCounter = 0;
			tempoCounter = TEMPO_BY_SAMPLE;

			#ifdef BOARD_WHALE
			check_auto_power_off();
			#endif

			#ifdef LIMITER_DEBUG
			printf("limiter_debug: %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\n",
				limiter_debug[0],
				limiter_debug[1],
				limiter_debug[2],
				limiter_debug[3],
				limiter_debug[4],
				limiter_debug[5],
				limiter_debug[6],
				limiter_debug[7],
				SAMPLE_VOLUME);

			limiter_debug[0] = 32760;
			limiter_debug[1] = -32760;
			limiter_debug[2] = 32760;
			limiter_debug[3] = -32760;
			limiter_debug[4] = 32760;
			limiter_debug[5] = -32760;
			limiter_debug[6] = 32760;
			limiter_debug[7] = -32760;
			#endif
    	}

    } //end skip channel cnt
