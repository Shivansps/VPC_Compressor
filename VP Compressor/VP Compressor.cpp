#include <iostream>
#include <thread>
#include <chrono>
#include <vector> 
#include <filesystem>
#include "lz41_compression.h"
using namespace std;
namespace fs = filesystem;
#pragma warning(disable:4996)

#define VPC_VERSION "1.0"
#define DEFAULT_IGNORE_LIST ".ogg .wav .fc2 .fs2 .tbm .tbl"
#define DEFAULT_MINIMUM_SIZE 10240
#define DEFAULT_MAX_THREADS 4
#define DEFAULT_BLOCK_SIZE 65536
#define DEFAULT_COMPRESSION_LEVEL 6 /* 1 to 4 LZ4, 5 to 12 LZ4-HC. Recomended: 6 */
#define DEFAULT_FIX_POFS 0 /* Convert pof files to version 2118, 1=Enabled, 0=Disabled */
#define DEFAULT_COMPRESS_ONLY_VP 0
#define DEFAULT_TAG_COMPRESSED_VP 0
#define SYSTEM_IGNORE_LIST ".exe .ini .dll .log .reg .sys .lnk"

void compress_folder(LZ41CONFIG *config, const char* path);
void decompress_folder(LZ41CONFIG *config, const char * path);
void compress_file(LZ41CONFIG* config, const char* pathc);
void decompress_file(LZ41CONFIG* config, const char* pathc);
bool parsecmd(int argc, char* argv[], LZ41CONFIG* config);
void load_config(LZ41CONFIG *mainconfig);
void ui_header(LZ41CONFIG *mainconfig);
void ui_gauges(THREAD_INFO* ti, LZ41CONFIG *config);

int main(int argc, char* argv[])
{
    LZ41CONFIG mainconfig;
    load_config(&mainconfig);

    if (parsecmd(argc,argv,&mainconfig))
        return 0;

    ui_header(&mainconfig);

    cout << "\n =>The configuracion is written to vpcconfig.ini, to change the configuration edit that file\n   and run this program again.\n\n";

    cout << " =>Valid Command Line Arguments: \n"<<" =>VPCompressor /compress_folder path_to_folder\n";
    cout << " =>VPCompressor /decompress_folder path_to_folder\n";
    cout << " =>VPCompressor /compress_file path_to_file\n";
    cout << " =>VPCompressor /decompress_file path_to_file\n\n";

    cout << " =>The working folder is: " << fs::current_path() << "\n\n";
    cout << " =>Press any key to compress all files inside the working folder or close the application.\n\n\n\n";

    system("pause");
    mainconfig.log = fopen("last.log", "wt");
    compress_folder(&mainconfig, fs::current_path().string().c_str());
    fclose(mainconfig.log);
	return 0;
}

bool parsecmd(int argc, char* argv[],LZ41CONFIG *config)
{
    if (argc > 2)
    {
        if (strcmp(argv[1], "/compress_folder") == 0)
        {
            config->log = fopen("last.log", "wt");
            if(fs::exists(argv[2]))
                compress_folder(config, argv[2]);
            fclose(config->log);
        }
        if (strcmp(argv[1], "/decompress_folder") == 0)
        {
            config->log = fopen("last.log", "wt");
            if (fs::exists(argv[2]))
                decompress_folder(config, argv[2]);
            fclose(config->log);
        }
        if (strcmp(argv[1], "/compress_file") == 0)
        {
            config->log = fopen("last.log", "wt");
            if (fs::exists(argv[2]))
                compress_file(config, argv[2]);
            fclose(config->log);
        }
        if (strcmp(argv[1], "/decompress_file") == 0)
        {
            config->log = fopen("last.log", "wt");
            if (fs::exists(argv[2]))
                decompress_file(config, argv[2]);
            fclose(config->log);
        }     
    }
    else
    {
        return false;
    }
    return true;
}

void compress_file(LZ41CONFIG* config, const char* pathc)
{
    config->max_threads = 1;
    THREAD_INFO ti;
    auto file_path = fs::path(pathc);
    auto outpath = file_path.parent_path().append("compressed/");

    create_directory(outpath);

    string path = file_path.string();
    string pathout = outpath.string().append(file_path.filename().string());
    strcpy(ti.current_file, file_path.filename().string().c_str());

    char* file_in = (char*)malloc(path.length() + 1);
    char* file_out = (char*)malloc(pathout.length() + 1);
    strcpy(file_in, path.c_str());
    strcpy(file_out, pathout.c_str());

    std::string str_ext = file_path.extension().string();
    std::transform(str_ext.begin(), str_ext.end(), str_ext.begin(), ::tolower);

    if (str_ext == ".vp")
    {
            thread wthread(compress_vp, file_in, file_out, config, &ti);
            thread ui_thread(ui_gauges, &ti, config);
            wthread.join();
            ui_thread.join();
    }
    else
    {
            thread wthread(compress_single_file, file_in, file_out, config, &ti);
            thread ui_thread(ui_gauges, &ti, config);
            wthread.join();
            ui_thread.join();
    }
}

void decompress_file(LZ41CONFIG* config, const char* pathc)
{
    config->verbose = true;
    auto file_path = fs::path(pathc);
    auto outpath = file_path.parent_path().append("decompressed/");

    fs::create_directory(outpath);

    string path = file_path.string();
    string pathout = outpath.string().append(file_path.filename().string());

    std::string str_ext = file_path.extension().string();
    std::transform(str_ext.begin(), str_ext.end(), str_ext.begin(), ::tolower);

    if (str_ext == ".vp")
    {
        decompress_vp(path.c_str(), pathout.c_str(), config);
    }
    else
    {
        decompress_single_file(path.c_str(), pathout.c_str(), config);
    }
}


void compress_folder(LZ41CONFIG *config, const char* pathc)
{
    vector<thread> threads;
    int thread_number = 1;
    THREAD_INFO* ti = (THREAD_INFO*)malloc(config->max_threads * sizeof(THREAD_INFO));
    auto outpath = fs::path(pathc).append("compressed/");

    create_directory(outpath);

    for (const auto& entry : fs::directory_iterator(fs::path(pathc), fs::directory_options::skip_permission_denied))
    {
        try {
            if (fs::is_regular_file(entry))
            {
                string path = entry.path().string();
                string pathout = outpath.string().append(entry.path().filename().string());

                std::string str_ext = entry.path().extension().string();
                std::transform(str_ext.begin(), str_ext.end(), str_ext.begin(), ::tolower);

                if (str_ext == ".vp")
                {
                    if (thread_number <= config->max_threads)
                    {
                        ti[thread_number - 1].thread_num = thread_number;
                        strcpy(ti[thread_number - 1].current_file, entry.path().filename().string().c_str());
                        char* file_in = (char*)malloc(path.length() + 1);
                        char* file_out = (char*)malloc(pathout.length() + 1);
                        strcpy(file_in, path.c_str());
                        strcpy(file_out, pathout.c_str());
                        threads.emplace_back(compress_vp, file_in, file_out, config, &ti[thread_number-1]);
                        thread_number++;
                    }
                }
                else if (!config->only_vps && !strstr(SYSTEM_IGNORE_LIST, str_ext.c_str()))
                {
                    if (thread_number <= config->max_threads)
                    {
                        ti[thread_number-1].thread_num = thread_number;
                        strcpy(ti[thread_number - 1].current_file, entry.path().filename().string().c_str());
                        char* file_in = (char*)malloc(path.length() + 1);
                        char* file_out = (char*)malloc(pathout.length() + 1);
                        strcpy(file_in, path.c_str());
                        strcpy(file_out, pathout.c_str());
                        threads.emplace_back(compress_single_file, file_in, file_out, config, &ti[thread_number-1]);
                        thread_number++;
                    }
                }

                if (thread_number > config->max_threads)
                {
                    thread ui_thread(ui_gauges, ti, config);                 
                    for (auto& th : threads)
                        th.join();
                    ui_thread.join();
                    threads.clear();
                    thread_number = 1;
                    for (int i = 0; i < config->max_threads; i++)
                        ti[i].max_files = 0;
                }
            }
        }
        catch(const fs::filesystem_error& e)
        {

        }
    }

    thread ui_thread(ui_gauges, ti, config);
    
    for (auto& th : threads)
        th.join();
    ui_thread.join();

    free(ti);
}

void decompress_folder(LZ41CONFIG* config, const char* path)
{
    config->verbose = true;
    auto outpath = fs::path(path).append("decompressed/");

    fs::create_directory(outpath);

    for (const auto& entry : fs::directory_iterator(fs::path(path), fs::directory_options::skip_permission_denied))
    {
        try {
            if (fs::is_regular_file(entry))
            {
                string path = entry.path().string();
                string pathout = outpath.string().append(entry.path().filename().string());

                std::string str_ext = entry.path().extension().string();
                std::transform(str_ext.begin(), str_ext.end(), str_ext.begin(), ::tolower);

                if (str_ext == ".vp")
                {
                    decompress_vp(path.c_str(), pathout.c_str(), config);
                }
                else
                {
                    if(!strstr(SYSTEM_IGNORE_LIST, str_ext.c_str()))
                        decompress_single_file(path.c_str(), pathout.c_str(), config);
                }
            }
        }
        catch (const fs::filesystem_error& e)
        {

        }
    }
}

void ui_header(LZ41CONFIG* mainconfig)
{
    cout << " ################################################################################################" << "\n";
    cout << " #                                    VP Compressor v" << VPC_VERSION << "                                        #" << "\n";
    cout << " ################################################################################################" << "\n";
    cout << " # VPC CONFIGURATION                                                                            #" << "\n";
    cout << " # Block Size: " << mainconfig->block_size << "            ## Only Compress VPs: " << mainconfig->only_vps << "               ## Minimum Size: " << mainconfig->minimum_size << "\n";
    cout << " # Max Threads: " << mainconfig->max_threads << "       ## Compression Level: " << mainconfig->compression_level << "         ## Convert pofs to version 2118: " << mainconfig->fix_pof << "\n";
    cout << " # Ignore List: " << mainconfig->ignore_list;
    if (!strchr(mainconfig->ignore_list, '\n'))
        cout << "\n";
    cout << " ################################################################################################" << "\n";
}

void ui_gauges(THREAD_INFO* ti, LZ41CONFIG *config)
{
    try {
        while (true)
        {
            int count_files = 0;
            this_thread::sleep_for(chrono::milliseconds(1000));
            system("cls");
            ui_header(config);
            cout << "\n\n";
            for (int i = 0; i < config->max_threads; i++)
            {
                if (ti[i].max_files > 0)
                {
                    count_files += ti[i].max_files - ti[i].finished_files;
                    double percent = (static_cast<double>(ti[i].finished_files) / ti[i].max_files) * 100.0;
                    cout << " |=======================================================================================================|\n";
                    cout << " | [THREAD #" << ti[i].thread_num << "] COMPRESSING: " << ti[i].current_file << "      " << (int)percent << "%  File: " << ti[i].finished_files << "/" << ti[i].max_files << "\n | ";
                    for (int z = 0; z <= percent; z++)
                        cout << "#";
                    cout << " \n | VP FILE: " << ti[i].vp_file << "\n";
                    cout << " |=======================================================================================================|\n";
                }
            }
            if (count_files == 0)
                return;
        }
    }catch(exception& e)
    {
    }
}

void load_config(LZ41CONFIG* mainconfig)
{
    strcpy(mainconfig->ignore_list, DEFAULT_IGNORE_LIST);
    mainconfig->minimum_size = DEFAULT_MINIMUM_SIZE;
    mainconfig->max_threads = DEFAULT_MAX_THREADS;
    mainconfig->block_size = DEFAULT_BLOCK_SIZE;
    mainconfig->fix_pof = (bool)DEFAULT_FIX_POFS;
    mainconfig->compression_level = DEFAULT_COMPRESSION_LEVEL;
    mainconfig->only_vps = (bool)DEFAULT_COMPRESS_ONLY_VP;
    mainconfig->tag_c_vps = (bool)DEFAULT_TAG_COMPRESSED_VP;
    FILE* conf_file = fopen("vpcconfig.ini", "rt");
    if (conf_file == nullptr)
    {
        conf_file = fopen("vpcconfig.ini", "wt");
        fprintf(conf_file, "block_size=%d\n", mainconfig->block_size);
        fprintf(conf_file, "minimum_size=%d\n", mainconfig->minimum_size);
        fprintf(conf_file, "ignore_list=%s\n", mainconfig->ignore_list);
        fprintf(conf_file, "max_threads=%d\n", mainconfig->max_threads);
        fprintf(conf_file, "fix_pofs=%d\n", (int)mainconfig->fix_pof);
        fprintf(conf_file, "only_compress_vps=%d\n", (int)mainconfig->only_vps);
        fprintf(conf_file, "compression_level=%d\n", mainconfig->compression_level);
        fprintf(conf_file, "tag_compressed_vps=%d\n", (int)mainconfig->tag_c_vps);
        fclose(conf_file);
        return;
    }
    char buffer[250];
    while (fgets(buffer, 250, conf_file)) {
        if (strstr(buffer, "block_size") != NULL) {
            char* ptr = strchr(buffer, '=');
            sscanf(ptr + 1, "%d", &mainconfig->block_size);
        }
        if (strstr(buffer, "minimum_size") != NULL) {
            char* ptr = strchr(buffer, '=');
            sscanf(ptr + 1, "%d", &mainconfig->minimum_size);
        }
        if (strstr(buffer, "ignore_list") != NULL) {
            char* ptr = strchr(buffer, '=');
            strcpy(mainconfig->ignore_list, ptr + 1);
        }
        if (strstr(buffer, "max_threads") != NULL) {
            char* ptr = strchr(buffer, '=');
            sscanf(ptr + 1, "%d", &mainconfig->max_threads);
        }
        if (strstr(buffer, "fix_pofs") != NULL) {
            char* ptr = strchr(buffer, '=');
            int temp;
            sscanf(ptr + 1, "%d", &temp);
            mainconfig->fix_pof = (bool)temp;
        }
        if (strstr(buffer, "only_compress_vps") != NULL) {
            char* ptr = strchr(buffer, '=');
            int temp;
            sscanf(ptr + 1, "%d", &temp);
            mainconfig->only_vps = (bool)temp;
        }
        if (strstr(buffer, "compression_level") != NULL) {
            char* ptr = strchr(buffer, '=');
            sscanf(ptr + 1, "%d", &mainconfig->compression_level);
        }
        if (strstr(buffer, "tag_compressed_vps") != NULL) {
            char* ptr = strchr(buffer, '=');
            int temp;
            sscanf(ptr + 1, "%d", &temp);
            mainconfig->tag_c_vps = (bool)temp;
        }
    }
    if (mainconfig->compression_level > 12)
        mainconfig->compression_level = 12;
    if (mainconfig->compression_level < 1)
        mainconfig->compression_level = 1;
    if (mainconfig->max_threads < 1)
        mainconfig->max_threads = 1;
    fclose(conf_file);
}