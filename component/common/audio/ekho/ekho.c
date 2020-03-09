#include "platform_stdlib.h"
#include "pinyin_voice.h"
#include "file_reader.h"
#include "ekho_common.h"
#include "dict.h"
#include "voice_index.h"

flash_file mVoiceFile;
GSM610_PRIVATE	*pgsm610;

void load_mandarin(){
	ff_open(&mVoiceFile, pinyin_voice, sizeof(pinyin_voice));
}

int tts_init(){
	load_mandarin();
	if ((pgsm610 = (GSM610_PRIVATE	*)calloc (1, sizeof (GSM610_PRIVATE))) == NULL)
	return -1;

	memset (pgsm610, 0, sizeof (GSM610_PRIVATE));
	
	pgsm610->samplesperblock = WAVLIKE_GSM610_SAMPLES;
	pgsm610->blocksize = WAVLIKE_GSM610_BLOCKSIZE;
	pgsm610->blockcount = 0;
}

int tts_lookup(unsigned short code, char *buf){
	
	int offset;
	int data_len;
	int read_len;
	int pcm_len;
	char gsm[4000];
	int true_flag = 1;

	offset = ((PhoneticSymbol *)mSymbolArray)[((DictItem *)mDictItemArray)[code].symbolCode].offset+GSM_WAV_HEADER_LEN;
	data_len = ((PhoneticSymbol *)mSymbolArray)[((DictItem *)mDictItemArray)[code].symbolCode].bytes-GSM_WAV_HEADER_LEN;

	ff_lseek(&mVoiceFile, offset);
	ff_read(&mVoiceFile, gsm, data_len, &read_len);


	if ((pgsm610->gsm_data = gsm_create()) == NULL)
		return -1 ;
	
	gsm_option (pgsm610->gsm_data, GSM_OPT_WAV49, &true_flag);

	if ((data_len % pgsm610->blocksize == 0) || (data_len % pgsm610->blocksize == 1))
		pgsm610->blocks = data_len / pgsm610->blocksize;
	else
	{	
		pgsm610->blocks = data_len / pgsm610->blocksize + 1;
	};
	pcm_len = pgsm610->samplesperblock * pgsm610->blocks * 2;
	
	for (int i=0; i<pgsm610->blocks; i++){
		memcpy(pgsm610->block, gsm+i*WAVLIKE_GSM610_BLOCKSIZE, WAVLIKE_GSM610_BLOCKSIZE);
		if (gsm_decode(pgsm610->gsm_data, pgsm610->block, pgsm610->samples) < 0){
			printf ("Error from WAV gsm_decode() on frame : %d\n", pgsm610->blockcount);
			return -1;
		};
		
		if (gsm_decode(pgsm610->gsm_data, pgsm610->block + (WAVLIKE_GSM610_BLOCKSIZE + 1)/2, pgsm610->samples + WAVLIKE_GSM610_SAMPLES/2) < 0){
			printf ("Error from WAV gsm_decode() on frame : %d.5\n", pgsm610->blockcount) ;
			return -1;
		};
		memcpy(buf + i*WAVLIKE_GSM610_SAMPLES*2, (unsigned char *)pgsm610 -> samples, WAVLIKE_GSM610_SAMPLES*2);
	}
	free(pgsm610->gsm_data);
	return pcm_len;
}

void tts_deinit(){
	free(pgsm610);
	ff_close(&mVoiceFile);
}