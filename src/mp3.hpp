#ifndef _MP3_HPP_INCLUDED_
#define _MP3_HPP_INCLUDED_

#include <string>
#include <vector>

#define MIN_CONSEC_GOOD_FRAMES	4
#define FRAME_HEADER_SIZE	4
#define MIN_FRAME_SIZE		21
#define NUM_SAMPLES		4

namespace mp3 {

struct header_t {

	header_t() {
		sync = 0;
		version = 0;
		layer = 0;
		crc = 0;
		bitrate = 0;
		freq = 0;
		padding = 0;
		extension = 0;
		mode = 0; 
		mode_extension = 0;
		copyright = 0;
		original = 0;
		emphasis = 0;
	}

	bool operator == (const header_t &rhs) {
		if (this == &rhs) {
			return true;
		}

		if (version == rhs.version &&
			layer == rhs.layer &&
			crc == rhs.crc &&
			freq == rhs.freq &&
			mode == rhs.mode &&
			copyright == rhs.copyright &&
			original == rhs.original &&
			emphasis == rhs.emphasis) {
			return true;
		}

		return false;
	}

	bool operator != (const header_t &rhs) {
		return !(*this == rhs);
	}

	unsigned long sync;
	unsigned int version;
	unsigned int layer;
	unsigned int crc;
	unsigned int bitrate;
	unsigned int freq;
	unsigned int padding;
	unsigned int extension;
	unsigned int mode;
	unsigned int mode_extension;
	unsigned int copyright;
	unsigned int original;
	unsigned int emphasis;

};

struct id3_t {
	char title[31];
	char artist[31];
	char album[31];
	char year[5];
	char comment[31];
	unsigned char track[1];
	char genre[1];
};      

class mp3_t {

public:
	mp3_t(const std::string &filename);
	~mp3_t();

public:
	void dump(const std::string &filename, bool index_only);

private:
	void get_id3(FILE *file);

	int get_first_header(FILE *file, off_t start);
	int get_next_header(FILE *file);

	static int get_header(FILE *file, header_t &header);

	static int frame_length(const header_t &header);
	static int header_layer(const header_t &header);
	static int header_bitrate(const header_t &header);
	static int header_frequency(const header_t &header);

private:
	bool valid;

	std::vector<unsigned int> seconds;

	off_t data_offset;
	off_t data_size;

	header_t header;
	bool header_valid;

	id3_t id3;
	bool id3_valid;

	size_t badframes;

	char *data;

};

} // namespace mp3

#endif // _MP3_HPP_INCLUDED_

