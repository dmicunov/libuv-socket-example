#include "common_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uv_udp_t g_handler;  

static void send_resp(uv_udp_t* handle, resp_data* data, const struct sockaddr* addr);

static void on_walk_cleanup(uv_handle_t* handle, void* dummy_data)
{
	uv_close(handle, NULL);
}

static void on_close(uv_handle_t* handle) 
{   
	//http://stackoverflow.com/questions/25615340/closing-libuv-handles-correctly
    uv_stop(uv_default_loop());
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_walk(uv_default_loop(), on_walk_cleanup, NULL);
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	uv_loop_close(uv_default_loop());
}

static void on_signal(uv_signal_t* signal, int signum)
{
    if(uv_is_active((uv_handle_t*)&g_handler)) {
		uv_udp_recv_stop(&g_handler);
	}
	uv_close((uv_handle_t*) &g_handler, on_close);
	uv_signal_stop(signal);
}

static void alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) 
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

static void prepare_resp_data(resp_data* data)
{
    uv_cpu_info_t* cpus;
    int count;
    uv_cpu_info(&cpus, &count);
    uv_loadavg(data->avg);
    strncpy((char*)data->brand, cpus[0].model, strlen(cpus[0].model));
    data->total_memory = uv_get_total_memory();
    data->speed = cpus[0].speed;
    uv_uptime(&data->uptime);
    
    printf("SENT DATA:\n");
    printf("Brand: %s\n", data->brand);
    printf("Speed: %d\n", data->speed);
    printf("Total memory: %lu\n", data->total_memory);
    printf("Uptime: %lf\n", data->uptime);
    printf("Data avg[0]:%lf\n", data->avg[0]);
    printf("Data avg[1]:%lf\n", data->avg[1]);
    printf("Data avg[2]:%lf\n", data->avg[2]);
    
    uv_free_cpu_info(cpus, count);
}

static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf, const struct sockaddr* addr, unsigned flags) 
{
    if (nread > 0) {
        uint32_t req_data = *((uint32_t*) rcvbuf->base);
        if (REQ_FLAG == req_data) {
            resp_data data = {0};
            prepare_resp_data(&data);
            send_resp(handle,&data, addr);
        }
    }
    free(rcvbuf->base);
}

static void on_send(uv_udp_send_t* req, int status) 
{   
    free(req);
    if (status) {
        fprintf(stderr, "uv_udp_send_cb error: %s\n", uv_strerror(status));
    }
}

static void send_resp(uv_udp_t* handle, resp_data* data, const struct sockaddr* addr) 
{
    uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t buf = uv_buf_init((char*) data, sizeof(resp_data));
    int r = uv_udp_send(req, handle, &buf, 1, addr, on_send);
    if (r) {
        fprintf(stderr, "uv_udp_send error: %s\n", uv_strerror(r));
    }
}

static void recv_req(uv_udp_t* handle) 
{
    int r = uv_udp_recv_start(handle, alloc_buf, on_recv);
    if (r) {
        fprintf(stderr, "uv_udp_recv_start error: %s\n", uv_strerror(r));
    }
}

int main(int argc, char* argv[])
{
    int r;
    struct sockaddr_in addr;
    uv_signal_t sigint, sigterm;

    uv_ip4_addr("0.0.0.0", 7000, &addr);

    uv_udp_init(uv_default_loop(), &g_handler);
    
    uv_signal_init(uv_default_loop(), &sigint);
    uv_signal_start(&sigint, on_signal, SIGINT);
    uv_signal_init(uv_default_loop(), &sigterm);
    uv_signal_start(&sigterm, on_signal, SIGTERM);
		
	r = uv_udp_bind(&g_handler, (const struct sockaddr*) &addr, 0);
    
    if (r) {
        fprintf(stderr, "uv_udp_bind error: %s\n", uv_strerror(r));
        return 1;
    }

    recv_req(&g_handler);

    r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	
    return r;
}
