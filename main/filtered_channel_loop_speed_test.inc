	int use_bg_sample = 0;
	int use_reverb = 0;
	int use_direct_waves_filters = 0;

	//static volatile double r_noise = NOISE_SEED;
	static volatile float r_noise = NOISE_SEED;
	static volatile int i_noise;

	float t0 = micros();

	noise_volume  = 1;
	noise_boost_by_sensor = 1;

	sd_write_buf = (uint32_t*)malloc(SD_WRITE_BUFFER);

	while(!event_next_channel)// || beats_selected)
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

		/*
		if(use_bg_sample)
		{
			sample_f[0] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;
			if(load_SD_card_sample==SD_CARD_SAMPLE_OPENED)
			{
				mixed_sample_buffer_ptr_L++;
			}
	
			//if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
			if(mixed_sample_buffer_ptr_L >= MIXED_SAMPLE_BUFFER_LENGTH)
			{
				mixed_sample_buffer_ptr_L = 0;
				//SD_card_sample_buffer_half = 2; //trigger reload of the 2nd buffer half
				if(load_SD_card_sample==SD_CARD_SAMPLE_OPENED)
				{
					mixed_sample_buffer_ptr_L = 1;
				}
			}
			//else if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH/2)
			//{
			//	SD_card_sample_buffer_half = 1; //trigger reload of the 1st buffer half
			//}						
		}
		else
		{
		*/
			r_noise = r_noise + 19;
			r_noise = r_noise * r_noise;
		//}

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
			//filtered_channel_add_echo();
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[2]) limiter_debug[2] = sample_i16;
		if(sample_i16>limiter_debug[3]) limiter_debug[3] = sample_i16;
		#endif

    	if(use_reverb)
    	{
			if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 41) //4Hz
			{
				//filtered_channel_adjust_reverb();
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

		/*
		if(use_bg_sample)
		{
			sample_f[1] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_R++]) / 32768.0f) * mixed_sample_volume;
			if(load_SD_card_sample==SD_CARD_SAMPLE_OPENED)
			{
				mixed_sample_buffer_ptr_R++;
			}

			if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH)
			//if(mixed_sample_buffer_ptr_R >= MIXED_SAMPLE_BUFFER_LENGTH)
			{
				SD_card_sample_buffer_half = 2; //trigger reload of the 2nd buffer half
				mixed_sample_buffer_ptr_R = 0;
				//if(load_SD_card_sample==SD_CARD_SAMPLE_OPENED)
				//{
				//	mixed_sample_buffer_ptr_R = 1;
				//}
			}
			else if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH/2)
			{
				SD_card_sample_buffer_half = 1; //trigger reload of the 1st buffer half
			}						
		}
		else
		{
		*/
			i_noise = r_noise;
			r_noise = r_noise - i_noise;
		//}

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
			//filtered_channel_add_echo();
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[6]) limiter_debug[6] = sample_i16;
		if(sample_i16>limiter_debug[7]) limiter_debug[7] = sample_i16;
		#endif

    	if(use_reverb)
    	{
			if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 43) //4Hz
			{
				//filtered_channel_adjust_reverb();
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

	    //register size_t i2s_bytes_rw;

	    //i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);

		//#ifdef SD_WRITE_ENABLED
	    sd_write_sample(&sample32);
		//#endif

	    if(sampleCounter%10==0)
	    {
	    	//printf("c=%d,s=%d\n", sampleCounter, sample32);
	    	tempoCounter--;
	    }
		//i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);

		/*
		if(use_direct_waves_filters)
		{
			//if (t_TIMING_BY_SAMPLE_EVERY_125_MS==21) //8Hz
			//if (t_TIMING_BY_SAMPLE_EVERY_250_MS==47) //4Hz
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
		*/

		sampleCounter++;
        tempoCounter--;        

		//if(tempoCounter==0) //one full second passed
        if(sampleCounter>=I2S_AUDIOFREQ)
    	{
    		seconds++;
    		sampleCounter = 0;
			tempoCounter = TEMPO_BY_SAMPLE;

			printf("s=%d, 1s worth of samples generated in: %f\n", sample32, micros() - t0);
			t0 = micros();
    	}

    } //end skip channel cnt
