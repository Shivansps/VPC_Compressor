#ifndef ALIGNER_H_INCLUDED
#define ALIGNER_H_INCLUDED
#define VERSION "0.3"

/*
Only convert to 2117 to 2118 in order not to break internal structures of older versions
-pof_bytes is the pointer to the buffer containing the cached pof file.
-total_size is the original file size of the pof.
-aligned_pof is the pointer where the aligned pof will be written to, memory has to be already
allocated and it has to be bigger than total_size, i recomend to just use total_size*2.
-verbose true/false prints to console the blocks that are unaligned and fixed.
-Returns the new pof_size.
*/
size_t align_pof(char *pof_bytes, size_t total_size, char *aligned_pof, bool verbose);

#endif // ALIGNER_H_INCLUDED
