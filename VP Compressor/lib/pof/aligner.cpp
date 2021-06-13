#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aligner.h"
#pragma warning(disable:4996)

void read_typechar_and_size(char *bytes, char *type_char, int *size_int)
{
    memcpy(type_char,bytes,4);
    *(type_char + 4) = '\0';
    memcpy(size_int,bytes+4,4);
}

void read_typeint_and_size(char *bytes, int *type_int, int *size_int)
{
    memcpy(type_int,bytes,4);
    memcpy(size_int,bytes+4,4);
}

size_t align_pof(char *pof_bytes, size_t total_size, char *aligned_pof, bool verbose)
 {
    int chunk_size;
    size_t copied=0;
    int new_pof_version=2200;
    char chunk_typechar[5];
    int unaligned=0;
    int version;
    char *init_pos, *init_aligned;
    init_pos=pof_bytes;
    init_aligned=aligned_pof;
    memcpy(&version,pof_bytes+4,4);
    memcpy(aligned_pof,pof_bytes,8);
    pof_bytes+=8;
    aligned_pof+=8;
    copied+=8;

    memcpy(aligned_pof-4,&new_pof_version,4);

    while(pof_bytes<init_pos+total_size)
    {
        read_typechar_and_size(pof_bytes, chunk_typechar,&chunk_size);
        //printf("|%s | %d|\n",chunk_typechar,chunk_size);
        unaligned=0;
        if ((chunk_size % 4) != 0)
        {
            unaligned = chunk_size + 4 - (chunk_size % 4);
        }

        if (strcmp(chunk_typechar, "FUEL") == 0 && version >= 2117)  /*************/
        {
            int num_thr,i,new_sizes=0;
            char name[260];
            char *size_offset;
            //Copy chunk basic data
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;
            copied+=8;

            //Copy num thrusters
            memcpy(&num_thr,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            //for each thruster
            for(int z=0;z<num_thr;z++)
            {
                int glows;
                //Copy glow number
                memcpy(&glows,pof_bytes,4);
                memcpy(aligned_pof,pof_bytes,4);
                new_sizes+=4;
                pof_bytes+=4;
                aligned_pof+=4;

                //Copy properties, enforce alignment
                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if(verbose)
                        printf("FUEL DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }
                //Copy glow data
                memcpy(aligned_pof,pof_bytes,28*glows);
                new_sizes+=28*glows;
                pof_bytes+=28*glows;
                aligned_pof+=28*glows;
            }
            //Update chunk size
            memcpy(size_offset,&new_sizes,4);
            copied+=new_sizes;
        }
        else if (strcmp(chunk_typechar, "GLOW") == 0)
        {
            int num_gb,i,new_sizes=0;
            char name[260];
            char *size_offset;
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;
            copied+=8;
            memcpy(&num_gb,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            for(int z=0;z<num_gb;z++)
            {
                int num_glows;
                memcpy(aligned_pof,pof_bytes,28);
                new_sizes+=28;
                pof_bytes+=28;
                aligned_pof+=28;
                memcpy(&num_glows,pof_bytes-4,4);

                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("GLOW DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }

                memcpy(aligned_pof,pof_bytes,28*num_glows);
                new_sizes+=28*num_glows;
                pof_bytes+=28*num_glows;
                aligned_pof+=28*num_glows;
            }
            copied+=new_sizes;
            memcpy(size_offset,&new_sizes,4);
        }
        else if (strcmp(chunk_typechar, "SPCL") == 0)
        {
            int num_points,i,new_sizes=0;
            char name[260];
            char *size_offset;
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            copied+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;

            memcpy(&num_points,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            for(int z=0;z<num_points;z++)
            {
                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("SPCL DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }


                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("SPCL DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }
                memcpy(aligned_pof,pof_bytes,16);
                new_sizes+=16;
                pof_bytes+=16;
                aligned_pof+=16;
            }
            copied+=new_sizes;
            memcpy(size_offset,&new_sizes,4);
        }
        else if (strcmp(chunk_typechar, "TXTR") == 0)
        {
            int num_textures,i,new_sizes=0;
            char name[260];
            char *size_offset;
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;
            copied+=8;
            memcpy(&num_textures,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            for(int z=0;z<num_textures;z++)
            {
                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("TXRT DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }
            }
            copied+=new_sizes;
            memcpy(size_offset,&new_sizes,4);
        }
        else if (strcmp(chunk_typechar, "DOCK") == 0)  /*************/
        {
            int num_docks,i,new_sizes=0;
            char name[260];
            char *size_offset;
            //Copy chunk basic data
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;
            copied+=8;

            //Copy num docks
            memcpy(&num_docks,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            //For each dock
            for(int z=0;z<num_docks;z++)
            {
                int spline;
                //Copy Properties, enforce alignment
                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("DOCK DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }

                //Copy spline paths
                memcpy(&spline,pof_bytes,4);
                memcpy(aligned_pof,pof_bytes,4);
                new_sizes+=4;
                pof_bytes+=4;
                aligned_pof+=4;


                memcpy(aligned_pof,pof_bytes,4*spline);
                new_sizes+=4*spline;
                pof_bytes+=4*spline;
                aligned_pof+=4*spline;

                //Now copy dock points and vectors
                memcpy(&spline,pof_bytes,4);
                memcpy(aligned_pof,pof_bytes,4);
                new_sizes+=4;
                pof_bytes+=4;
                aligned_pof+=4;

                memcpy(aligned_pof,pof_bytes,24*spline);
                new_sizes+=24*spline;
                pof_bytes+=24*spline;
                aligned_pof+=24*spline;
            }
            //Update chunk size
            memcpy(size_offset,&new_sizes,4);
            copied+=new_sizes;
        }
        else if (strcmp(chunk_typechar, "PATH") == 0)  /*************/
        {
            int num_paths,i,new_sizes=0;
            char name[260];
            char *size_offset;
            memcpy(aligned_pof,pof_bytes,12);
            pof_bytes+=8;
            size_offset=aligned_pof+4;
            aligned_pof+=12;
            copied+=8;
            memcpy(&num_paths,pof_bytes,4);
            pof_bytes+=4;
            new_sizes+=4;

            for(int z=0;z<num_paths;z++)
            {
                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("PATH DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }

                memcpy(&i,pof_bytes,4);
                pof_bytes+=4;
                memcpy(name,pof_bytes,i);
                if ((i % 4) != 0)
                {
                    if (verbose)
                        printf("PATH DATA ALIGNED||");
                    int newsize = i + 4 - (i % 4);
                    memcpy(aligned_pof,&newsize,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    memset(aligned_pof+i,'\0',newsize-i);
                    new_sizes+=newsize+4;
                    aligned_pof+=newsize;
                    pof_bytes+=i;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,pof_bytes,i);
                    aligned_pof+=i;
                    new_sizes+=i+4;
                    pof_bytes+=i;
                }

                int verts;
                memcpy(&verts,pof_bytes,4);
                memcpy(aligned_pof,pof_bytes,4);
                new_sizes+=4;
                pof_bytes+=4;
                aligned_pof+=4;

                for(int v=0;v<verts;v++)
                {
                    memcpy(aligned_pof,pof_bytes,20);
                    new_sizes+=20;
                    pof_bytes+=20;
                    aligned_pof+=20;
                    int turrets;
                    memcpy(&turrets,pof_bytes-4,4);
                    if(turrets!=0)
                    {
                        memcpy(aligned_pof,pof_bytes,turrets*4);
                        new_sizes+=turrets*4;
                        pof_bytes+=turrets*4;
                        aligned_pof+=turrets*4;
                    }
                }
            }
            copied+=new_sizes;
            memcpy(size_offset,&new_sizes,4);
        }
        else if (strcmp(chunk_typechar, "SLDC") == 0)  /*************/
        {
            /*Only basic alignment for SLDC*/

            memcpy(aligned_pof,pof_bytes,chunk_size+8);

            if(unaligned)
            {
                if (verbose)
                    printf("SLDC DATA ALIGNED||");
                memcpy(aligned_pof+4,&unaligned,4);
                aligned_pof+=unaligned+8;
                copied+=unaligned+8;
            }
            else
            {
                aligned_pof+=chunk_size+8;
                copied+=chunk_size+8;
            }

            /*Convert SLDC to SLC2, Only do this if pof version is 2117*/
            if(version==2117)
            {
                char *p_tree_size, *p_chunk_size;
                unsigned int tree_size,node_size,node_type_int, new_tree_size=0, count=0;
                char node_type_char, new_type[5]="SLC2";

                //Copy Chunk ID and save tree size and chunk size locations
                memcpy(aligned_pof,new_type,4);
                p_chunk_size=aligned_pof+4;
                p_tree_size=aligned_pof+8;
                copied+=12;
                aligned_pof+=12;
                pof_bytes+=8;

                memcpy(&tree_size,pof_bytes,4);
                pof_bytes+=4;

                //Process the SLDC tree to the end
                while(count<tree_size)
                {
                    //Save Node type and size
                    memcpy(&node_type_char,pof_bytes,1);
                    memcpy(&node_size,pof_bytes+1,4);

                    //Convert Node type to int
                    node_type_int = (int)node_type_char;

                    //Copy node type and adjusted node size, move pointers
                    memcpy(aligned_pof,&node_type_int,4);
                    node_size+=3;
                    memcpy(aligned_pof+4,&node_size,4);
                    node_size-=3;
                    copied+=8;
                    aligned_pof+=8;
                    pof_bytes+=5;


                    //Copy Vectors
                    memcpy(aligned_pof,pof_bytes,24);
                    copied+=24;
                    aligned_pof+=24;
                    pof_bytes+=24;

                    if(node_type_char==0)
                    {
                        //Front and back offsets must be adjusted
                        int front,back,newback=0;
                        char *p;

                        p=pof_bytes-29;
                        memcpy(&back,p+33,4);

                        //printf("DEBUG: old back : %d\n",back);

                        //I need to find the new distance to back.
                        while(p<pof_bytes+back-29)
                        {
                            int ns;
                            memcpy(&ns,p+1,4);
                            //printf("DEBUG: ns : %d \n",ns);
                            p+=ns;
                            newback+=ns+3;

                        }
                        //printf("DEBUG: newback: %d\n",newback);

                        //Copy offsets
                        front=node_size+3;
                        memcpy(aligned_pof,&front,4); //Front is always this node size+3;
                        memcpy(aligned_pof+4,&newback,4);

                        copied+=8;
                        aligned_pof+=8;
                        pof_bytes+=8;
                    }
                    else
                    {
                        //Copy the remaining data on the node
                        memcpy(aligned_pof,pof_bytes,node_size-29);

                        //Move pointers
                        copied+=node_size-29;
                        aligned_pof+=node_size-29;
                        pof_bytes+=node_size-29;
                    }

                    //Count the new tree size and move the counter
                    count+=node_size;
                    new_tree_size+=node_size+3;
                }
                //Copy the new chunk and tree size
                memcpy(p_tree_size,&new_tree_size,4);
                new_tree_size+=4;
                memcpy(p_chunk_size,&new_tree_size,4);
                if (verbose)
                    printf("SLDC DATA CONVERTED TO SLC2||");
            }
            else
            {
                pof_bytes+=chunk_size+8;
            }
        }
        else if (strcmp(chunk_typechar, "OBJ2") == 0 || strcmp(chunk_typechar, "SOBJ") == 0) /*************/
        {
            int i, ini_cp=copied;
            char name[260];
            char *this_chunk_size,*end, *bsp_data;
            bsp_data=(char*)malloc(chunk_size+8);
            memcpy(bsp_data,pof_bytes,chunk_size+8);
            memcpy(aligned_pof,bsp_data,68);
            this_chunk_size=aligned_pof+4;
            end=bsp_data+chunk_size+8;
            aligned_pof+=68;
            bsp_data+=68;
            copied+=68;

            memcpy(&i,bsp_data,4);
            bsp_data+=4;
            memcpy(name,bsp_data,i);
            if ((i % 4) != 0)
            {
                if (verbose)
                    printf("OBJ DATA ALIGNED||");
                int newsize = i + 4 - (i % 4);
                memcpy(aligned_pof,&newsize,4);
                aligned_pof+=4;
                memcpy(aligned_pof,bsp_data,i);
                memset(aligned_pof+i,'\0',newsize-i);
                aligned_pof+=newsize;
                bsp_data+=i;
                copied+=newsize+4;
            }
            else
            {
                memcpy(aligned_pof,&i,4);
                aligned_pof+=4;
                memcpy(aligned_pof,bsp_data,i);
                aligned_pof+=i;
                bsp_data+=i;
                copied+=i+4;
            }


            memcpy(&i,bsp_data,4);
            bsp_data+=4;
            memcpy(name,bsp_data,i);
            if ((i % 4) != 0)
            {
                if (verbose)
                    printf("OBJ DATA ALIGNED||");
                int newsize = i + 4 - (i % 4);
                memcpy(aligned_pof,&newsize,4);
                aligned_pof+=4;
                memcpy(aligned_pof,bsp_data,i);
                memset(aligned_pof+i,'\0',newsize-i);
                aligned_pof+=newsize;
                bsp_data+=i;
                copied+=newsize+4;
            }
            else
            {
                if(i==0)
                {
                   if (verbose)
                       printf("OBJ DATA ALIGNED||");
                   int z=4;
                   memcpy(aligned_pof,&z,4);
                   memset(aligned_pof+4,'\0',4);
                   copied+=8;
                   aligned_pof+=8;
                }
                else
                {
                    memcpy(aligned_pof,&i,4);
                    aligned_pof+=4;
                    memcpy(aligned_pof,bsp_data,i);
                    aligned_pof+=i;
                    bsp_data+=i;
                    copied+=i+4;
                }
            }
            memcpy(aligned_pof,bsp_data,16);
            bsp_data+=16;
            aligned_pof+=16;
            copied+=16;

            int bsp_chunk_type,bsp_chunk_size;
            do{
                read_typeint_and_size(bsp_data,&bsp_chunk_type,&bsp_chunk_size);
                if(bsp_chunk_type==0)
                    bsp_chunk_size=4;

                //printf("|%d | %d|\n",bsp_chunk_type,bsp_chunk_size);

                /*DEFPOINTS is the only bsp data chunk that could be unaligned*/
                if(bsp_chunk_type==1)
                {
                    if ((bsp_chunk_size % 4) != 0)
                    {
                        if (verbose)
                            printf("BSP DEFPOINTS DATA ALIGNED||");
                        int newsize = bsp_chunk_size + 4 - (bsp_chunk_size % 4);
                        memcpy(bsp_data+4,&newsize,4);
                        memcpy(aligned_pof,bsp_data,bsp_chunk_size);
                        int vertex_offset;
                        memcpy(&vertex_offset,bsp_data+16,4);
                        memmove(aligned_pof+vertex_offset+(newsize-bsp_chunk_size),aligned_pof+vertex_offset,bsp_chunk_size-vertex_offset);
                        vertex_offset+=(newsize-bsp_chunk_size);
                        memcpy(aligned_pof+16,&vertex_offset,4);

                        bsp_data+=bsp_chunk_size;
                        aligned_pof+=newsize;
                        copied+=newsize;
                    }
                    else
                    {
                        memcpy(aligned_pof,bsp_data,bsp_chunk_size);
                        bsp_data+=bsp_chunk_size;
                        aligned_pof+=bsp_chunk_size;
                        copied+=bsp_chunk_size;
                    }
                }
                else
                {
                    memcpy(aligned_pof,bsp_data,bsp_chunk_size);
                    bsp_data+=bsp_chunk_size;
                    aligned_pof+=bsp_chunk_size;
                    copied+=bsp_chunk_size;
                }
            }while(bsp_data<end);

            ini_cp=copied-ini_cp-8;

            if ((ini_cp % 4) != 0)
            {
                int newsize = ini_cp + 4 - (ini_cp % 4);
                aligned_pof+=newsize-ini_cp;
                ini_cp=newsize;
            }
            memcpy(this_chunk_size,&ini_cp,4);
            pof_bytes+=chunk_size+8;
        }
        else if (strcmp(chunk_typechar, "PINF") == 0)
        {
            if(unaligned)
            {
                if (verbose)
                    printf("PINF DATA ALIGNED||");
            }
            else
            {
                unaligned=chunk_size;
            }

            memcpy(aligned_pof,pof_bytes,8);
            unaligned+=36;
            memset(aligned_pof+8,'\0',unaligned);
            memcpy(aligned_pof+4,&unaligned,4);

            strcat(aligned_pof+8,"Processed with POF_Aligner  v ");
            strcat(aligned_pof+38,VERSION);
            memset(aligned_pof+41,'\n',1);
            memcpy(aligned_pof+42,pof_bytes+8,chunk_size);

            aligned_pof+=unaligned+8;
            copied+=unaligned+8;
            pof_bytes+=chunk_size+8;
        }
        else if (strcmp(chunk_typechar, "SLC2") == 0)
        {
            /*If present just copy it*/
            memcpy(aligned_pof,pof_bytes,chunk_size+8);
            aligned_pof+=chunk_size+8;
            copied+=chunk_size+8;
            pof_bytes+=chunk_size+8;
        }
        else    /*************/
        {
            /*Default Basic Chunk Alignment*/
            memcpy(aligned_pof,pof_bytes,chunk_size+8);

            if(unaligned)
            {
                if (verbose)
                    printf("%s CHUNK DATA ALIGNED. BASIC CHUNK ALIGNMENT WAS USED||",chunk_typechar);
                memcpy(aligned_pof+4,&unaligned,4);
                aligned_pof+=unaligned+8;
                copied+=unaligned+8;
            }
            else
            {
                aligned_pof+=chunk_size+8;
                copied+=chunk_size+8;
            }
            pof_bytes+=chunk_size+8;
        }
    }
    aligned_pof=init_aligned;
    //printf("FINISHED. NEW SIZE IS: %ld (+%ld)\n\n",copied,copied-total_size);
    return copied;
}
