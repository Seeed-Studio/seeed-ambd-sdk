//使用前需要f_open()
//current_index永远指向下一个要读的byte的index
#include "file_reader.h"
#include "platform_stdlib.h"
void ff_open(flash_file* file, const char* array_name, int file_size){
	file -> array_data = array_name;
	file -> current_index = 0;
	file -> size = file_size;
}
//这里其实需要验证读数超出的情况
void ff_read(flash_file* file, char* buffer, int byte_to_read, int* byte_read){
	if(file -> current_index >= file ->size){
		printf("file overread\n");
		printf("file size is %d\n", file ->size);
		printf("current_index is %d\n", file -> current_index);
	}
	for(int i = 0; i < byte_to_read; i++){
		buffer[i] = (file -> array_data)[file -> current_index];
		file -> current_index++;
		//printf("buffer direct read %d\n", buffer[i]);
		//printf("buffer direct read %d\n", buffer[i+1]);
		//printf("buffer direct read %d\n", buffer[i+2]);
		//printf("buffer direct read %d\n", buffer[i+3]);
	}
}

unsigned char ff_eof(flash_file* file){
	if(file -> current_index == 0){
		if(file -> size > 0){
			return 0;
		}			
	} else {
		if(file -> current_index == file -> size){
			return 1;
		} else{
			return 0;
		}
	}
}

void ff_lseek(flash_file* file, int offset){
	file -> current_index = offset;
}

void ff_close(flash_file* file){

}
