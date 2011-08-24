#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mp3.hpp"

namespace mp3 {

int layer_tab[4]= {0, 3, 2, 1};
	
int frequencies[3][4] = {
	{ 22050, 24000, 16000, 50000 },
	{ 44100, 48000, 32000, 50000 },
	{ 11025, 12000, 8000,  50000 }
};
		
int bitrate[2][3][14] = {
	{
		{ 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256 },
		{ 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160 },
		{ 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160 }
	},	      

	{	       
		{ 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
		{ 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384 },
		{ 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320 }
	}
};		      
			
int frame_size_index[] = { 24000, 72000, 72000 };

void
mp3_t::dump(const std::string &filename, bool index_only) {
	std::ofstream fout;

	fout.open(filename.c_str(), std::ios::out | std::ios::binary);

	fout.put('1');

	size_t vector_size = seconds.size();
	fout.write((char*)&vector_size, sizeof (size_t));

	for (std::size_t i = 0; i < seconds.size(); ++i) {
		unsigned int corrected_offset = seconds[i] & 0x7F7F7F7F;

		char correction = 0x0;
		correction |= ((0x80000000 & seconds[i]) ? 1 << 3 : 0);
		correction |= ((0x00800000 & seconds[i]) ? 1 << 2 : 0);
		correction |= ((0x00008000 & seconds[i]) ? 1 << 1 : 0);
		correction |= ((0x00000080 & seconds[i]) ? 1: 0);

		fout.write((char*)&corrected_offset, sizeof (corrected_offset));
		fout.put(correction);
	}

	if (!index_only) {
		fout.write(data, data_size);
	}
}

void
mp3_t::get_id3(FILE *file) {
	id3_valid = false;

	char buf[4];
	if (data_size >= 128) {
		data_size -= 128;

		fseek(file, -128, SEEK_END);

		size_t readed = fread(buf, 1, 3, file);
		buf[sizeof (buf) - 1] = '\0';

		id3.genre[0] = 225;

		if (!strncmp(buf, "TAG", sizeof ("TAG"))) {
			id3_valid = true;

			fseek(file, -125, SEEK_END);

			readed = fread(id3.title, sizeof (char), sizeof (id3.title) - 1, file);
			id3.title[sizeof (id3.title) - 1] = '\0';

			readed = fread(id3.artist, sizeof (char), sizeof (id3.artist) - 1, file);
			id3.artist[sizeof (id3.artist) - 1] = '\0';

			readed = fread(id3.album, sizeof (char), sizeof (id3.album) - 1, file);
			id3.album[sizeof (id3.album) - 1] = '\0';

			readed = fread(id3.year, sizeof (char), sizeof (id3.year) - 1, file);
			id3.year[sizeof (id3.year) - 1] = '\0';

			readed = fread(id3.comment, sizeof (char), sizeof (id3.comment) - 1, file);
			id3.comment[sizeof (id3.comment) - 1] = '\0';

			if (id3.comment[28] == '\0') {
				id3.track[0] = id3.comment[29];
			}

			readed = fread(id3.genre, sizeof (char), sizeof (id3.genre) - 1, file);
		}
	}
}

int	     
mp3_t::get_first_header(FILE *file, off_t start) {
	int k, l = 0, c;
	header_t h, h2;
	long valid_start = 0;
			
	fseek(file, start, SEEK_SET);
	while (true) {  
		while ((c = fgetc(file)) != 255 && (c != EOF)) {
		}       
		if (c == 255) {
			ungetc(c, file);
			valid_start = ftell(file);
			if ((l = get_header(file, h))) { 
				fseek(file, l - FRAME_HEADER_SIZE, SEEK_CUR);
				for (k = 1; (k < MIN_CONSEC_GOOD_FRAMES) && (data_size - ftell(file) >= FRAME_HEADER_SIZE); k++) {
					if (!(l = get_header(file, h2))) {
						break;
					}
					if (h != h2) {
						break;
					}
					fseek(file, l - FRAME_HEADER_SIZE, SEEK_CUR);
				}
				if(k == MIN_CONSEC_GOOD_FRAMES) {
					fseek(file, valid_start, SEEK_SET);
					header = h2;
					header_valid = 1;
					return 1;
				}
			}
		} else {
			return 0;
		}
	}
	return 0;
}

int
mp3_t::get_next_header(FILE *file) {
	int l = 0, c, skip_bytes = 0;
	header_t h;

	while (true) {
		while ((c = fgetc(file)) != 255 && (ftell(file) < data_size)) {
			skip_bytes++;
		}
		if (c == 255) {
			ungetc(c, file);
			if ((l = get_header(file, h))) {
				if (skip_bytes) {
					badframes++;
				}
				fseek(file, l - FRAME_HEADER_SIZE, SEEK_CUR);
				return 15 - h.bitrate;
			} else {
				skip_bytes += FRAME_HEADER_SIZE;
			}
		} else {
			if (skip_bytes) {
				badframes++;
			}
			return 0;
		}
	}
}

int
mp3_t::get_header(FILE *file, header_t &header) {
	unsigned char buffer[FRAME_HEADER_SIZE];
	int fl;

	if (fread(&buffer, FRAME_HEADER_SIZE, 1, file) < 1) {
		header.sync = 0;
		return 0;
	}

	header.sync = (((int)buffer[0] << 4) | ((int)(buffer[1] & 0xE0) >> 4));

	if (buffer[1] & 0x10) {
		header.version = (buffer[1] >> 3) & 1;
	} else {
		header.version = 2;
	}

	header.layer = (buffer[1] >> 1) & 3;

	if ((header.sync != 0xFFE) || (header.layer != 1)) {
		header.sync = 0;
		return 0;
	}

	header.crc = buffer[1] & 1;
	header.bitrate = (buffer[2] >> 4) & 0x0F;
	header.freq = (buffer[2] >> 2) & 0x3;
	header.padding = (buffer[2] >>1) & 0x1;
	header.extension = (buffer[2]) & 0x1;
	header.mode = (buffer[3] >> 6) & 0x3;
	header.mode_extension = (buffer[3] >> 4) & 0x3;
	header.copyright = (buffer[3] >> 3) & 0x1;
	header.original = (buffer[3] >> 2) & 0x1;
	header.emphasis = (buffer[3]) & 0x3;

	return ((fl = frame_length(header)) >= MIN_FRAME_SIZE ? fl : 0);
}

int
mp3_t::frame_length(const header_t &header) {
	return header.sync == 0xFFE ?
		    (frame_size_index[3 - header.layer]*((header.version & 1) + 1) *
		    header_bitrate(header) / header_frequency(header)) +
		    header.padding : 1;
}

int
mp3_t::header_layer(const header_t &header) {
	return layer_tab[header.layer];
}

int
mp3_t::header_bitrate(const header_t &header) {
	if (!header.bitrate)
		return 0;
	return bitrate[header.version & 1][3 - header.layer][header.bitrate - 1];
}

int
mp3_t::header_frequency(const header_t &header) {
	return frequencies[header.version][header.freq];
}

mp3_t::mp3_t(const std::string &filename) {
	struct stat filestat;
	if (-1 == stat(filename.c_str(), &filestat)) {
		return;
	}

	data_size = filestat.st_size;

	FILE  *fp; 

	fp = fopen(filename.c_str(), "rb+");

	if (NULL == fp) {
		return;
	}

	get_id3(fp);

	header_t header_tmp;

	int frame_type[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	int frames = 0;

	badframes = 0;

	off_t data_start = 0;
	if (get_first_header(fp, 0)) {
		data_start = ftell(fp);
		header_tmp = header;

		int prev_second = 0;
		float total_seconds = 0;
		off_t second_position = data_start;

		int bitrate;
		while ((bitrate = get_next_header(fp))) {
			++frame_type[sizeof (frame_type) - bitrate];
			header_tmp.bitrate = bitrate;

			float local_seconds = (float)(frame_length(header_tmp)) / (float)(header_bitrate(header_tmp) * 125);
			total_seconds += local_seconds;

			off_t p = ftell(fp);

			if (total_seconds > prev_second) {
				seconds.push_back(p);
				prev_second += 1;
			}
			second_position += frame_length(header_tmp) ;
			frames++;
		}
	}

	data = (char *)malloc (sizeof (char) * data_size);
	fseek(fp, data_start, SEEK_SET);

	size_t readed = fread(data, sizeof (char), data_size, fp);
	valid = (readed > 0);

	fclose(fp);
}

mp3_t::~mp3_t() {
	free(data);
}

} // namespace mp3

