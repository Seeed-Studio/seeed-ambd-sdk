#include "FreeRTOS.h"
#include "task.h"
#include "platform_opts.h"
#include "section_config.h"

#include <stdio.h>
#include "fatfs_wrap.h"

#if CONFIG_EXAMPLE_STD_FILE

/* Config for Ameba-Pro */
#if defined(CONFIG_PLATFORM_8195BHP)
#define STACK_SIZE		4096
//#include "sdio_combine.h"
//#include "sdio_host.h"
//#include <disk_if/inc/sdcard.h>
#include "fatfs_sdcard_api.h"
#endif // defined(CONFIG_PLATFORM_8195BHP)
FRESULT list_files(char* list_path);
FRESULT del_dir(const char *path, int del_self);

FATFS 	fs_sd;
FILE *    m_file;

#if defined(CONFIG_PLATFORM_8195BHP)
#define TEST_BUF_SIZE	(512)
fatfs_sd_params_t fatfs_sd;

void example_std_file_thread(void* param){

	int Fatfs_ok = 1;
	int sd_drv_num, flash_drv_num;
	char	sd_drv[4]; 	
    u8 WRBuf[TEST_BUF_SIZE];
    u8 RDBuf[TEST_BUF_SIZE];
	u8 test_info[] = "### Ameba test standard file sd card ###\n\r";        
	FRESULT res; 
	char path[64];
        /* test files */
	char sd_fn[20] = "sd_file";
        char sd_dir[20] = "sd_dir";
        char sd_file2[20] = "file2";
        char sd_dir2[20] = "dir2";
        
	int br,bw;
        
        vTaskDelay(2000);
	printf("=== STD File Example (SD card) ===\n\r");

	res = fatfs_sd_init();
	if(res < 0){
		printf("fatfs_sd_init fail (%d)\n", res);
		Fatfs_ok = 0;
		goto fail;
	}
	fatfs_sd_get_param(&fatfs_sd);

	if(Fatfs_ok){
		strcpy(path, fatfs_sd.drv);
		sprintf(&path[strlen(path)],"%s",sd_fn);

		printf("\n\r=== Delete test file ===\n\r");
		//remove(path);
                del_dir(fatfs_sd.drv, 0);

		printf("\n\r=== SD card FS Read/Write test ===\n\r");
		m_file = fopen(path, "a+");
		
		if(!m_file){
			printf("open file (%s) fail.\n", sd_fn);
			goto exit;
		}

		printf("Test file name: %s\n\n",sd_fn);

		memset(&WRBuf[0], 0x00, TEST_BUF_SIZE);
		memset(&RDBuf[0], 0x00, TEST_BUF_SIZE);
		
		strcpy(&WRBuf[0], &test_info[0]);
		
		do{
			bw = fwrite(WRBuf, 1, strlen(WRBuf), m_file);
			if(bw < 0){
				fseek(m_file, 0, SEEK_SET); 
				printf("Write error.\n");
			}
			printf("Write %d bytes.\n", bw);
		} while (bw < strlen(WRBuf));

		printf("Write content:\n%s\n", WRBuf);
		
		/* move the file pointer to the file head*/
		res = fseek(m_file, 0, SEEK_SET); 

		br = fread(RDBuf, 1, TEST_BUF_SIZE, m_file);
		if(br < 0){
			fseek(m_file, 0, SEEK_SET); 
			printf("Read error.\n");
		}
		printf("Read %d bytes.\n", br);
	
		printf("Read content:\n%s\n", RDBuf);
                res = fclose(m_file);
		if(res){
			printf("close file (%s) fail.\n", sd_fn);
		}
                
                // create directory: 0:/sd_dir
		memset(path, 0, sizeof(path)); 
		snprintf(path, sizeof(path), "%s%s", fatfs_sd.drv, sd_dir);		
		printf("\n\rmkdir: %s \n\r", path);
                res = mkdir(path, 0);
                printf("mkdir: %d\n\r", res);
                
                // create file: 0:/sd_dir/file2
                memset(path, 0, sizeof(path)); 
		snprintf(path, sizeof(path), "%s%s/%s", fatfs_sd.drv, sd_dir, sd_file2);		
		printf("\n\rCreate file: %s \n\r", path);
                m_file = fopen(path, "a+");
                res = fclose(m_file);
		if(res){
			printf("close file (%s) fail.\n", sd_fn);
		}
                
                // create directory: 0:/sd_dir/dir2
		memset(path, 0, sizeof(path)); 
		snprintf(path, sizeof(path), "%s%s/%s", fatfs_sd.drv, sd_dir, sd_dir2);		
		printf("\n\rmkdir: %s \n\r", path);
                res = mkdir(path, 0);
                printf("mkdir: %d\n\r", res);
                
                // read directory: 0:/
                struct dirent *dp;
                memset(path, 0, sizeof(path)); 
		snprintf(path, sizeof(path), "%s%s", fatfs_sd.drv, sd_dir);		
		printf("\n\rRead directory: %s \n\r", fatfs_sd.drv);
                list_files(fatfs_sd.drv);
                
                // scandir: 0:/
                struct dirent **namelist;
                int count = scandir(fatfs_sd.drv, &namelist, NULL, NULL);
                printf("\n\rscandir: %s\n\r", fatfs_sd.drv);
                printf("==> %d files in %s\n\r", count, fatfs_sd.drv);
                for(int i = 0; i < count; i++)
                {
                  printf("[%d] %s\n\r", i, namelist[i]->d_name);
                }
                
                // stat: 0:/sd_file
		printf("\n\rstat: %s\n\r", "0:/sd_file");
                struct stat fstat;
                stat("0:/sd_file", &fstat);
                printf("file size: %d bytes\n\r", fstat.st_size);
                printf("file time: %d (Epoch)\n\r", fstat.st_atime);
                
                // access: 0:/some_file 
                printf("\n\raccess: %s (Does not exist)\n\r", "0:/some_file");
                res = access("0:/some_file", W_OK);
                printf("acess: %d\n\r", res);
                
                printf("\n\raccess: %s\n\r", "0:/sd_file");
                res = access("0:/sd_file", W_OK);
                printf("acess: %d\n\r", res);
                
                //printf("access: %s\n\r", "0:/sd_file");
                //res = access("0:/sd_file", W_OK);
                //printf("%s\n\r", (res == 1)? "W_OK" : "Does not exist");

	}
	
fail:
	fatfs_sd_close();
exit:
	vTaskDelete(NULL);
}

FRESULT del_dir(const char *list_path, int del_self)  
{  
    FRESULT res;    
    DIR *m_dir;
    FILINFO m_fileinfo;     
    char *filename;
    char path[1024];
    char file[1024];  
    struct dirent *entry;
#if _USE_LFN
		char fname_lfn[_MAX_LFN + 1];
		m_fileinfo.lfname = fname_lfn;
		m_fileinfo.lfsize = sizeof(fname_lfn);
#endif        
        m_dir = opendir(list_path);
        sprintf(path, "%s", list_path); 

	if(m_dir) {
		for (;;) {
			// read directory and store it in file info object
                        entry = readdir(m_dir);
                 
                        if(!entry)
                          break;
                        filename = entry->d_name;
          
			if (*filename == '.' || *filename == '..') 
			{
				continue;
			}

			sprintf((char*)file, "%s/%s", path, filename);  

			if (entry->d_type == DT_DIR) {  
                          printf("del dir: %s\n\r", file);
                          del_dir(file, 1);  
                        }  
                        else { 
                          printf("del file: %s\n\r", file);
                          remove(file);  
                        }  	
		}
	}
	
    // close directory
	res = closedir(m_dir);

	// delete self? 
    if(res == FR_OK) {
		if(del_self == 1)
			res = remove(path);  
    }
    
	return res;  
}  

char *print_file_info(struct dirent *entry, char* path)
{
	char info[512];

	snprintf(info, sizeof(info), 
		"%c  %9u  %30s  %30s", 
		(entry->d_type == DT_DIR) ? 'D' : 'F',
		entry->d_reclen,
		entry->d_name,
		path);
	printf("%s\n\r", info);
	return info;
}

FRESULT list_files(char* list_path)
{
	FRESULT res;    
    DIR *m_dir;
    FILINFO m_fileinfo;     
    char *filename;
    char path[1024];
    char file[512];  
    struct dirent *entry;

    m_dir = opendir(list_path);
        sprintf(path, "%s", list_path); 
	if(m_dir) {
		for (;;) {
			// read directory
                        entry = readdir(m_dir);
                 
                        if(!entry) {
                          break;
                        }
                        filename = entry->d_name;
			if (*filename == '.' || *filename == '..') 
			{
				continue;
			}

			if (entry->d_type == DT_DIR) {  
                          sprintf((char*)file, "%s/%s", path, filename); 
                          print_file_info(entry, path);
                          res = list_files(file);
                          if (res != FR_OK) 
                          {
				break;
                          }
                        }  
                        else { 
                          print_file_info(entry, path);
                        }  	
		}
	}
	
        // close directory
	res = closedir(m_dir);
    
	return res;  
}

#endif
void example_std_file(void)
{
	if(xTaskCreate(example_std_file_thread, ((const char*)"example_std_file_thread"), STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_std_file_thread) failed", __FUNCTION__);
}
#endif
