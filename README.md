# VPC VP Compressor tool for FSO<br/>

This tool is designed to compress VP files (and standalone files) using LZ4 compression in a format that FSO can read, this allows to use compressed files in FSO what saves disk space and improves HDD loading times.<br/>

HOW TO BUILD:<br/>
-------------<br/>
Visual studio project is provided, makefile for linux will be added later, the compressor uses all standard libs except for LZ4 what is provided.<br/>
Pre-compiled Windows build is also provided.<br/>
<br/>
<br/>
HOW TO USE:<br/>
-----------<br/>
The easiest way to use it is just to drop the executable into the folder with the files to be compressed, execute it and press any key, this will start compression of the current folder with default settings.<br/>
<br/>
Additionally, command line arguments are allowed, these are:<br/>
/compress_folder path_to_folder<br/>
/compress_file path_to_file<br/>
/decompress_folder path_to_folder<br/>
/decompress_file path_to_file<br/>
<br/>
Along with the executable there is a register_menu_entry.cmd file for Windows, by running this file (where the VPCompressor.exe is) a context menu entry is added to<br/>
Windows right click menu to allow easy compression and decompression of file (max 100 selected files are allowed, 101 is not going to work.
<br/>
<br/>

SETTINGS:<br/>
---------<br/>
The configuration file is auto-generated the first time if it does not exist, this allow to change a few options.<br/>
<br/>
block_size<br/>
This is the size in bytes of LZ4 compressed blocks, the higher this number is, the smaller the output files are due to reduced overhead, but it will also use a little more memory to decompress.<br/>
Default value is 65536<br/><br/>

minimum_size<br/>
This is the minimum size in bytes that a file has to be to be compressed, this is used as a optimisation to avoid compressing files that are not worth it. This could be set 0 without any problem since files that end up bigger than original are ignored.<br/>
Default value is 10240<br/><br/>

ignore_list<br/>
This is the list of file extension to be ignored for compression, there is no issue in compressing every format, but i recommend to avoid audio.<br/>
Default value is ".ogg .wav .fc2 .fs2 .tbm .tbl"<br/><br/>

max_threads<br/>
The max number of compression threads, this is the maximum amount of simultaneous compression tasks.<br/>
Default value is 4<br/><br/>

fix_pofs<br/>
This setting allows for converting .pof files from version 2117 to version 2118 format. This is the pof_aligner code, since this is still experimental it is set to disabled by default.<br/>
Default value 0 (disabled)<br/><br/>

only_compress_vps<br/>
Enabling this option makes VPC to only compress vp files and ignore everything else on the folder.<br/>
Default value 0 (disabled)<br/><br/>

compression_level<br/>
This is the compression setting that is passed to the LZ4 compressor, a higher level means slower compression but smaller file size, it should have no effect on FSO decompression speed.<br/>
This can be set from 1 to 12, that being: 1 to 4 standard LZ4 compression, 5 to 12 LZ4-HC compression.<br/>
Default value is 6<br/><br/>

tag_compressed_vps<br/>
Enabling this option will add "_vpc" to the vp name if the VP contains at least 1 compressed file inside.<br/>
Default value is 0 (disabled)<br/>


