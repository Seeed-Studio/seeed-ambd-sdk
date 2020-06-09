#pragma once

#include <basic_types.h>
#include <stdbool.h>

void IntercomInit(void);
void IntercomDirRx(bool on);
void IntercomExistTxData(bool on);

int IntercomRx(u8* buf, u16 len);
int IntercomTx(const u8* buf, u16 len);
