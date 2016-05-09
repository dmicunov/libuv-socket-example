#include "common_data.h"
#include <stdio.h>
#include <stdlib.h>

static struct sockaddr_in server_addr;

void alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) 
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void on_close(uv_handle_t* handle) 
{
    uv_unref(handle);
    free(handle);
    uv_loop_close(uv_default_loop());
}

static void on_send_request(uv_udp_send_t* req, int status) 
{
    free(req);
    if (status) {
        fprintf(stderr, "uv_udp_send_cb error: %s\n", uv_strerror(status));
    }
}

static void send_request(uv_udp_t* handle) 
{
    uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t));
    
    uint32_t req_data = REQ_FLAG;
    uv_buf_t buf = uv_buf_init((char*) &req_data, sizeof(uint32_t));

    int r = uv_udp_send(req, handle, &buf, 1, (const struct sockaddr*) &server_addr, on_send_request);
    if (r) {
        fprintf(stderr, "uv_udp_send error: %s\n", uv_strerror(r));
    } 
}


static void on_recv_resp(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) 
{
    if (nread && (sizeof(resp_data)==nread)) {
        resp_data* data = (resp_data*) buf->base;
        printf("RECEIVED DATA:\n");
        printf("Brand: %s\n", data->brand);
        printf("Total memory: %lu\n", data->total_memory);
        printf("Uptime: %lf\n", data->uptime);
        printf("Speed: %d\n", data->speed);
        printf("Data avg[0]:%lf\n", data->avg[0]);
        printf("Data avg[1]:%lf\n", data->avg[1]);
        printf("Data avg[2]:%lf\n", data->avg[2]);
        uv_close((uv_handle_t*) handle, on_close);
    } else {
        send_request(handle); 
    }
    free(buf->base);
}

static void recv_resp(uv_udp_t* handle) 
{
    int r = uv_udp_recv_start(handle, alloc_buf, on_recv_resp);
    if (r) {
        fprintf(stderr, "uv_udp_recv_start error: %s\n", uv_strerror(r));
    }
}

int main(int argc, char** argv) 
{
    int r;
    
    uv_loop_t* loop = uv_default_loop();
    
    //HARDCODED IP ADDRESS OF SERVER 
    uv_ip4_addr("10.30.10.76", 7000, &server_addr);

    // closed below, freed in on_close()
    uv_udp_t* handle = (uv_udp_t*) malloc(sizeof(uv_udp_t));
    uv_udp_init(loop, handle);
    
    send_request(handle);   
    recv_resp(handle);
    
    r = uv_run(loop, UV_RUN_DEFAULT);
    
    
    return r;
}
