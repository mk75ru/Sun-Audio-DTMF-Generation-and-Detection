#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "const.h"
#include "audio.h"
#include "dtmf.h"
#include "dtmf_static.h"
#include "goertzel.h"
#include "debug.h"

#include "misc.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 */

/**
 * DTMF generation main function.
 * DTMF events are read (in textual tab-separated format) from the specified
 * input stream and audio data of a specified duration is written to the specified
 * output stream.  The DTMF events must be non-overlapping, in increasing order of
 * start index, and must lie completely within the specified duration.
 * The sample produced at a particular index will either be zero, if the index
 * does not lie between the start and end index of one of the DTMF events, or else
 * it will be a synthesized sample of the DTMF tone corresponding to the event in
 * which the index lies.
 *
 *  @param events_in  Stream from which to read DTMF events.
 *  @param audio_out  Stream to which to write audio header and sample data.
 *  @param length  Number of audio samples to be written.
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length) {
	// TO BE IMPLEMENTED
	if (events_in == NULL || audio_out == NULL) return EOF;
	uint32_t count = 0;
	AUDIO_HEADER header;
	header.magic_number = AUDIO_MAGIC;
	header.data_offset = AUDIO_DATA_OFFSET;
	header.data_size = length * 2;
	header.encoding = PCM16_ENCODING;
	header.sample_rate = AUDIO_FRAME_RATE;
	header.channels = AUDIO_CHANNELS;
	if (audio_write_header(audio_out, &header)) return EOF;
	FILE *noiseFile;
	if (noise_file != NULL) {
		if ((noiseFile = fopen(noise_file, "r")) == NULL) return EOF;
		AUDIO_HEADER noiseHeader;
		if (audio_read_header(noiseFile, &noiseHeader)) {
			fclose(noiseFile);
			return EOF;
		}
		//fclose(noiseFile);
	}
	uint32_t startIndex = 0, endIndex = 0;
	int symbol, Fr, Fc, noiseEnd = 0;
	double sampleValue = 0;
	double Const10 = pow(10, (double)noise_level / 10);
	double w = Const10 / (Const10 + 1);
	while (count < length) {
		if (count == endIndex) {
			if (getNextDTMFEvent(events_in, &startIndex, &endIndex, &symbol)) {
				if (!feof(events_in)) {
					if (noise_file != NULL) fclose(noiseFile);
					return EOF;
				}
			} else {
				if (startIndex > length || endIndex > length) {
					if (noise_file != NULL) fclose(noiseFile);
					return EOF;
				}
				if (getDTMFBySymbol(symbol, &Fr, &Fc)) {
					if (noise_file != NULL) fclose(noiseFile);
					return EOF;
				}
			}
		}
		if (count >= startIndex && count < endIndex) {
			double tempVal = 2.0 * M_PI * count / AUDIO_FRAME_RATE;
			double val1 = cos(tempVal * Fr);
			double val2 = cos(tempVal * Fc);
			double val = val1 * 0.5 + val2 * 0.5;
			sampleValue = val * INT16_MAX;
			//debug("%f\n", sampleValue);
		} else {
			sampleValue = 0;
		}
		if (noise_file != NULL) {
			int16_t noiseSample = 0;
			//if ((noiseFile = fopen(noise_file, "r")) == NULL) return EOF;
			if (!noiseEnd && audio_read_sample(noiseFile, &noiseSample)) {
				if (feof(noiseFile)) {
					noiseEnd = 1;
				} else {
					if (noise_file != NULL) fclose(noiseFile);
					return EOF;
				}
			}
			sampleValue = sampleValue * (1 - w) + noiseSample * w;
		}
		if (audio_write_sample(audio_out, (int16_t)sampleValue)) {
			if (noise_file != NULL) fclose(noiseFile);
			return EOF;
		}
		count++;
	}
	if (noise_file != NULL) fclose(noiseFile);
	debug("success\n");
	return 0;
}

/**
 * DTMF detection main function.
 * This function first reads and validates an audio header from the specified input stream.
 * The value in the data size field of the header is ignored, as is any annotation data that
 * might occur after the header.
 *
 * This function then reads audio sample data from the input stream, partititions the audio samples
 * into successive blocks of block_size samples, and for each block determines whether or not
 * a DTMF tone is present in that block.  When a DTMF tone is detected in a block, the starting index
 * of that block is recorded as the beginning of a "DTMF event".  As long as the same DTMF tone is
 * present in subsequent blocks, the duration of the current DTMF event is extended.  As soon as a
 * block is encountered in which the same DTMF tone is not present, either because no DTMF tone is
 * present in that block or a different tone is present, then the starting index of that block
 * is recorded as the ending index of the current DTMF event.  If the duration of the now-completed
 * DTMF event is greater than or equal to MIN_DTMF_DURATION, then a line of text representing
 * this DTMF event in tab-separated format is emitted to the output stream. If the duration of the
 * DTMF event is less that MIN_DTMF_DURATION, then the event is discarded and nothing is emitted
 * to the output stream.  When the end of audio input is reached, then the total number of samples
 * read is used as the ending index of any current DTMF event and this final event is emitted
 * if its length is at least MIN_DTMF_DURATION.
 *
 *   @param audio_in  Input stream from which to read audio header and sample data.
 *   @param events_out  Output stream to which DTMF events are to be written.
 *   @return 0  If reading of audio and writing of DTMF events is sucessful, EOF otherwise.
 */
int dtmf_detect(FILE *audio_in, FILE *events_out) {
	// TO BE IMPLEMENTED
	if (audio_in == NULL || events_out == NULL) return EOF;
	AUDIO_HEADER header;
	if (audio_read_header(audio_in, &header)) return EOF;
	debug("%u\n", header.data_offset);
	int16_t sample;
	uint32_t startIndex = 0, endIndex = 0;
	int currentSymbol = 0;
	double kVal = (double)block_size / 8000;
	double minDuration = MIN_DTMF_DURATION * 8000;
	while (!feof(audio_in)) {
		for (int i = 0; i < NUM_DTMF_FREQS; i++) {
			goertzel_init(goertzel_state + i, block_size, kVal * (*(dtmf_freqs + i)));
		}
		for (int i = 0; i < block_size - 1; i++) {
			if (audio_read_sample(audio_in, &sample)) {
				if (feof(audio_in)) {
					if (currentSymbol != 0 && ((endIndex - startIndex) >= minDuration)) {
						if (writeNextDTMFEvent(events_out, startIndex, endIndex, currentSymbol))return EOF;
					}
					if (i != 0) return EOF;
					debug("%d %d %c\n", startIndex, endIndex, currentSymbol);
					debug("success\n");
					return 0;
				} else {
					return EOF;
				}
			}
			for (int j = 0; j < NUM_DTMF_FREQS; j++) {
				goertzel_step(goertzel_state + j, (double) sample / INT16_MAX);
			}
		}
		//if (feof(audio_in)) return EOF;
		if (audio_read_sample(audio_in, &sample)) return EOF;

		double r0 = goertzel_strength(goertzel_state, (double) sample / INT16_MAX);
		double r1 = goertzel_strength(goertzel_state + 1, (double) sample / INT16_MAX);
		double r2 = goertzel_strength(goertzel_state + 2, (double) sample / INT16_MAX);
		double r3 = goertzel_strength(goertzel_state + 3, (double) sample / INT16_MAX);
		double r4 = goertzel_strength(goertzel_state + 4, (double) sample / INT16_MAX);
		double r5 = goertzel_strength(goertzel_state + 5, (double) sample / INT16_MAX);
		double r6 = goertzel_strength(goertzel_state + 6, (double) sample / INT16_MAX);
		double r7 = goertzel_strength(goertzel_state + 7, (double) sample / INT16_MAX);
		/*for (int i = 0; i < NUM_DTMF_FREQS; i++) {
			debug("%d: %f\n", i, goertzel_strength(goertzel_state + i, (double) sample / INT16_MAX));
		}*/
		int Fr, Fc, newSymbol, statusCode;
		if ((statusCode = getStrongestFrequencyIndexes(r0, r1, r2, r3, r4, r5, r6, r7, &Fr, &Fc))) {
			//Increment startIndex for no detection
			newSymbol = 0;
		} else {
			//Detection succeeds
			newSymbol = *(*(dtmf_symbol_names + Fr) + Fc);
		}
		if (newSymbol != currentSymbol || newSymbol == 0) {
			if (currentSymbol != 0 && ((endIndex - startIndex) >= minDuration)) {
				if (writeNextDTMFEvent(events_out, startIndex, endIndex, currentSymbol)) return EOF;
				debug("%d %d %c\n", startIndex, endIndex, currentSymbol);
				startIndex = endIndex;
			}
			currentSymbol = newSymbol;
		}
		if (statusCode)startIndex += block_size;
		endIndex += block_size;//endIndex always increments after each block
	}
	return EOF;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the operation mode of the program (help, generate,
 * or detect) will be recorded in the global variable `global_options`,
 * where it will be accessible elsewhere in the program.
 * Global variables `audio_samples`, `noise file`, `noise_level`, and `block_size`
 * will also be set, either to values derived from specified `-t`, `-n`, `-l` and `-b`
 * options, or else to their default values.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected program operation mode, and global variables `audio_samples`,
 * `noise file`, `noise_level`, and `block_size` to contain values derived from
 * other option settings.
 */
int validargs(int argc, char **argv)
{
	// TO BE IMPLEMENTED
	if (argc > 1) {

		char **argvc = argv + 1;

		if (stringCompare(*argvc, "-h") == 0) {
			debug("-h detected\n");
			global_options = HELP_OPTION;
			return 0;
		}

		int mode = 0;

		int tIntegerResult = 1000;
		int lIntegerResult = 0;
		char* nNameResult = NULL;

		int bIntegerResult = DEFAULT_BLOCK_SIZE;

		int isOverflow = 0;

		int tCheck = 0, nCheck = 0, lCheck = 0, bCheck = 0;

		for (int count = 1; count < argc; count++) {
			switch (mode) {
			case 0:
				//if(count==1){
				if (stringCompare(*argvc, "-g") == 0) {
					debug("-g detected\n-------------------------------------\n");
					mode = 1;
					argvc++;
					continue;
				} else if (stringCompare(*argvc, "-d") == 0) {
					debug("-d detected\n-------------------------------------\n");
					mode = 2;
					argvc++;
					continue;
				}
				return -1;
			//}
			case 1:
				if (count % 2 == 0) {
					if (stringCompare(*argvc, "-t") == 0) {
						debug("-t detected\n");
						if (tCheck || count + 1 >= argc || !isStringInteger(*(argvc + 1))) return -1;
						tIntegerResult = stringToInteger(*(argvc + 1), &isOverflow);
						if (isOverflow || tIntegerResult < 0 || tIntegerResult > 268435455) return -1;
						//printf("-t value: %d\n", tIntegerResult);
						tCheck = 1;
					} else if (stringCompare(*argvc, "-n") == 0) {
						debug("-n detected\n");
						char firstLetter = (count + 1 < argc) ? **(argvc + 1) : '\0';
						if (nCheck || firstLetter == '-' || firstLetter == '\0') return -1;
						nNameResult = *(argvc + 1);
						//printf("-n value: %s\n", *(argvc + 1));
						nCheck = 1;
					} else if (stringCompare(*argvc, "-l") == 0) {
						debug("-l detected\n");
						if (lCheck || count + 1 >= argc || !isStringInteger(*(argvc + 1))) return -1;
						lIntegerResult = stringToInteger(*(argvc + 1), &isOverflow);
						if (isOverflow || lIntegerResult < -30 || lIntegerResult > 30) return -1;
						//printf("-l value: %d\n", lIntegerResult);
						lCheck = 1;
					} else {
						return -1;
					}
					count++;
					argvc += 2;
					continue;
				}
				break;
			case 2:
				if (count % 2 == 0) {
					if (stringCompare(*argvc, "-b") == 0) {
						debug("-b detected\n");
						if (bCheck || count + 1 >= argc || !isStringInteger(*(argvc + 1))) return -1;
						bIntegerResult = stringToInteger(*(argvc + 1), &isOverflow);
						if (isOverflow || bIntegerResult < 10 || bIntegerResult > 1000) return -1;
						//printf("-b value: %d\n", bIntegerResult);
						bCheck = 1;
					} else {
						return -1;
					}
					count++;
					argvc += 2;
					continue;
				}
				break;

			default :
				return -1;

			}
			//
		}
		switch (mode) {
		case 1:
			global_options = GENERATE_OPTION;
			audio_samples = tIntegerResult * 8;
			noise_file = nNameResult;
			noise_level = lIntegerResult;
			break;

		case 2:
			global_options = DETECT_OPTION;
			block_size = bIntegerResult;
			break;

		default: return -1;
		}
		debug("audio_samples: %d\n", audio_samples);
		debug("noise_file: %s\n", noise_file);
		debug("noise_level: %d\n-------------------------------------\n", noise_level);
		debug("block_size: %d\n-------------------------------------\n", block_size);
		return 0;
	}

	return -1;
}
