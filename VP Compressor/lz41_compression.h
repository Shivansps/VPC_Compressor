/* ShivanSpS - I modified the LZ4 random access example with dictionary as a base for this implementation. 
-Mission files and tables should stay in a readeable format, you can compress then, but i think it is better this way.
-The minimum size is a optimization, there is no need to compress very small files.
-The file header is a version, this is used to tell FSO how to decompress that file, always use 4 chars to mantain alignment, it is stored at the start of the file. "LZ41" for this implementation.
-The block size is used to tell how much information is compressed into a block, each block adds overhead, so a larger the block bytes result in smaller file size.
-COMPRESSED FILE DATA STUCTURE: HEADER|BLOCKS|OFFSETS|NUM_OFFSETS|ORIGINAL_FILESIZE|BLOCK_SIZE
*/

struct THREAD_INFO {
	int thread_num = 1;
	char vp_file[200];
	char current_file[200];
	int finished_files = 0;
	int max_files = 1;
};

struct LZ41CONFIG{
	int block_size = 65536;
	int minimum_size = 0;
	int max_threads = 1;
	char ignore_list[200];
	bool fix_pof = false;
	bool verbose = false;
	bool only_vps = false;
	bool tag_c_vps = true;
	int compression_level = 6;
	FILE* log = nullptr;
};

#define LZ41_FILE_HEADER "LZ41"

/*RETURN ERROR CODES*/
#define LZ41_DECOMPRESSION_ERROR -1
#define LZ41_MAX_BLOCKS_OVERFLOW -2
#define LZ41_HEADER_MISMATCH -3
#define LZ41_OFFSETS_MISMATCH -4
#define LZ41_COMPRESSION_ERROR -5

void compress_vp(char* fn_in, char* fn_out, const LZ41CONFIG *config, THREAD_INFO *ti);
void decompress_vp(const char* fn_in, const char* fn_out, const LZ41CONFIG *config);
void compress_single_file(char* fn_in, char* fn_out, const  LZ41CONFIG *config, THREAD_INFO* ti);
void decompress_single_file(const char* fn_in, const char* fn_out, const LZ41CONFIG *config);