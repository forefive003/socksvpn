#pragma once

#define MAX_REMOTE_SRV_CNT  10

#define SESSION_UPDATE_TIME  3
#define REMOTE_CHK_TIME 5

int proxy_init();
void proxy_free();
int proxy_reset();
