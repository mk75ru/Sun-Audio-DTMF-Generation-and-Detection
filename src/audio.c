#include <stdio.h>

#include "audio.h"
#include "debug.h"

int audio_read_header(FILE *in, AUDIO_HEADER *hp) {
	// TO BE IMPLEMENTED
	if (in == NULL || hp == NULL) return EOF;
	int count = 0;
	unsigned int c, fieldValue = 0, d_offset;
	while ((c = fgetc(in)) != EOF && count < 24) {
		//46*16^6+115*16^4+110*16^2+100
		fieldValue += c << (8 * (3 - (count % 4)));
		count++;
		if (count != 0 && count % 4 == 0) {
			//debug("%u\n", fieldValue);
			int fieldIndex = count / 4;
			switch (fieldIndex) {
			case 1:
				if (fieldValue == AUDIO_MAGIC) {
					hp->magic_number = fieldValue;
					break;
				}
				return EOF;
			case 2:
				if (fieldValue >= AUDIO_DATA_OFFSET) {
					hp->data_offset = fieldValue;
					d_offset = fieldValue;
					break;
				}
				return EOF;
			case 3:
				if (fieldValue >= 0) {
					hp->data_size = fieldValue;
					break;
				}
				return EOF;
			case 4:
				if (fieldValue == PCM16_ENCODING) {
					hp->encoding = fieldValue;
					break;
				}
				return EOF;
			case 5:
				if (fieldValue == AUDIO_FRAME_RATE) {
					hp->sample_rate = fieldValue;
					break;
				}
				return EOF;
			case 6:
				if (fieldValue == AUDIO_CHANNELS) {
					hp->channels = fieldValue;
					for (int i = 0; i < d_offset - 24; i++) {
						if (fgetc(in) == EOF) return EOF;
					}
					return ferror(in) == 0 ? 0 : EOF;
				}
				return EOF;
			default:
				return EOF;
			}

			fieldValue = 0;
		}
	}
	return EOF;
}

//the number of bytes per frame = 2
int audio_write_header(FILE *out, AUDIO_HEADER *hp) {
	// TO BE IMPLEMENTED
	if (out == NULL || hp == NULL) return EOF;
	uint32_t fieldValue;
	for (int count = 0; count < 6; count++) {
		switch (count) {
		case 0:
			fieldValue = hp->magic_number;
			break;
		case 1:
			fieldValue = hp->data_offset;
			break;
		case 2:
			fieldValue = hp->data_size;
			break;
		case 3:
			fieldValue = hp->encoding;
			break;
		case 4:
			fieldValue = hp->sample_rate;
			break;
		case 5:
			fieldValue = hp->channels;
			break;
		default:
			return EOF;
		}
		int oneByte = 0, byteOffset = 0;
		while (fieldValue >= 0 && byteOffset < 4) {
			int shiftValue = (fieldValue == 0) ? 0 : (8 * (3 - byteOffset));
			oneByte = fieldValue >> shiftValue;
			//debug("%d\n", oneByte);
			fputc(oneByte, out);
			fieldValue -= oneByte << shiftValue;
			byteOffset++;
		}
		if (count == 5 ) {
			return (ferror(out) == 0 && byteOffset == 4) ? 0 : EOF;
		}
	}
	return EOF;
}

int audio_read_sample(FILE *in, int16_t *samplep) {
	// TO BE IMPLEMENTED
	if (in == NULL || samplep == NULL) return EOF;
	int c, count = 0;
	int16_t fieldValue = 0;
	while ((c = fgetc(in)) != EOF && count < 2) {
		//fieldValue += (count == 0 ? (((char)c) << (8 * (1 - (count % 2)))) : c << (8 * (1 - (count % 2))));
		fieldValue += c << (8 * (1 - (count % 2)));
		count++;
		if (count != 0 && count % 2 == 0) {
			*samplep = fieldValue;
			//debug("%d\n", fieldValue);
			return ferror(in) == 0 ? 0 : EOF;
		}
	}
	return EOF;
}

int audio_write_sample(FILE *out, int16_t sample) {
	// TO BE IMPLEMENTED
	if (out == NULL) return EOF;
	int oneByte = 0, byteOffset = 0;
	while (byteOffset < 2) {
		int shiftValue = (sample == 0) ? 0 : (8 * (1 - byteOffset));
		oneByte = sample >> shiftValue;
		//debug("%d\n", oneByte);
		fputc(oneByte, out);
		sample -= oneByte << shiftValue;
		byteOffset++;
	}
	return (ferror(out) == 0) ? 0 : EOF;
}
