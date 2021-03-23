#ifndef ALIGNER_H_INCLUDED
#define ALIGNER_H_INCLUDED
#define VERSION "0.3"

/*Only convert to 2117 to 2118 in order not to break internal structures of older versions*/
size_t align_pof(char *pof_bytes, size_t total_size, char *aligned_pof, bool verbose);

#endif // ALIGNER_H_INCLUDED
