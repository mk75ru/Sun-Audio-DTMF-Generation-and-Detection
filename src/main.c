#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
	if (validargs(argc, argv))
		USAGE(*argv, EXIT_FAILURE);
	if (global_options & 1)
		USAGE(*argv, EXIT_SUCCESS);
	// TO BE IMPLEMENTED
	if (global_options & 2) {
		if (!dtmf_generate(stdin, stdout, audio_samples)) return EXIT_SUCCESS;
	} else if (global_options & 4) {
		if (!dtmf_detect(stdin, stdout)) return EXIT_SUCCESS;
	}
	fprintf(stderr, "\033[1;31m" "ERROR: %s:%s:%d: %s\n" , \
	        __FILE__,  __FUNCTION__, __LINE__, "Operation failed");

	/*
	AUDIO_HEADER header;
	int16_t samplep;
	debug("%d\n", audio_read_header(stdin, &header));
	//debug("%d\n", audio_write_header(stdout, &header));
	debug("%d\n", audio_read_sample(stdin, &samplep));
	debug("%d\n", audio_write_sample(stdout, samplep));
	debug("magic_num: %u\n", header.magic_number);
	debug("data_offset: %u\n", header.data_offset );
	debug("data_size: %u\n", header.data_size );
	debug("encoding: %u\n", header.encoding);
	debug("sample_rate: %u\n", header.sample_rate);
	debug("channels: %u\n", header.channels);
	debug("global_options: %d\n-------------------------------------\n", global_options);
	*/
	return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
