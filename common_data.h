#ifndef _COMMON_DATA_H_
#define _COMMON_DATA_H_

#include <stdint.h>
#include <uv.h>

static const uint32_t REQ_FLAG = 1234;

struct _resp_data {
        char brand[64];
        uint64_t total_memory;
        int speed;
        double avg[3];
        double uptime;
} __attribute__((packed));

typedef struct _resp_data resp_data;

#endif /* _COMMON_DATA_H_ */
