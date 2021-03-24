#ifndef VP_H_INCLUDED
#define VP_H_INCLUDED

/*
    VP Read Process:
    -Open file and call read_vp_header()
    -Allocate enoght memory to store the entire VP index structure*numfiles in the vp
    -Call load_vp_index() to load the entire VP index.
    -Iterate trought the index array, once you find the file you are looking for call load_vp_file()

    VP Writting Process:
    -Create a new index array and allocate enoght memory for the VP index structure*numfiles you want to write to the VP
    -Create two ints to store the current number of written files and the current index offset, these must remain unchanged until the entire process is completed.
    -Call write_vp_file(), you need to pass all the data about the new file as well as the pointers to the index array,
     number of files and index_offset.
*/


typedef struct vp_index_entry {
    unsigned int offset;
    unsigned int filesize;
    char name[32];
    unsigned int timestamp;
}vp_index_entry;

typedef struct file_vp {
    char *data;
    unsigned int type; //0 folder // 1 file
}file_vp;

/*
    Loads the VP header info to variables
    Header char must be of size 5, Returns 0 if file is not a VP
*/
int read_vp_header(FILE *vp, char *header,unsigned int *version, unsigned int *index_offset, unsigned int *num_files);
/*
    Loads the entire VP file index to memory, vp_index must be already allocated * numfiles
*/
void load_vp_index(FILE *vp, vp_index_entry *vp_index, unsigned int index_offset,  unsigned int num_files);
/*
    Loads a file from the VP to memory, no pre-alloc is needed.
    Returns the char* pointer to the memory location, this must be later be free using free()
*/
char* load_vp_file(FILE *vp, vp_index_entry *vp_index_entry);
/*
    Writes a new VP header to file, mostly used internally by write_vp_file.
*/
void write_vp_header(FILE* vp, unsigned int index_offset, unsigned int num_files);
/*
    Writes a new file index entry to the vp file, mostly used internally by write_vp_file.
*/
void write_vp_index_entry(FILE* vp, vp_index_entry* index_entry);
/*
    Writes a new file to the VP.
    *index is the pointer to a previously allocated VP Index structure array, this structure must be allocated first and be of the
    correct size to store this file information alongside all others file indexes in the vp.
    *num_files and *index_offset are increased after every file that is added, they must remain in memory until the VP file writting is complete.
*/
void write_vp_file(FILE *vp, char *file, char *name, unsigned int filesize, unsigned int timestamp, vp_index_entry *index, unsigned int *num_files, unsigned int *index_offset);

/*
    Used to get a new timestamp for vp files if needed.
*/
unsigned int getUnixTime();

#endif // VP_H_INCLUDED
