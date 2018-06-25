
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_STREAM_STS_MODULE_H_INCLUDED_
#define _NGX_STREAM_STS_MODULE_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_server_traffic_status_string.h"
#include "ngx_stream_server_traffic_status_node.h"

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO          0
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UA          1
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UG          2
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG          3

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAMS            (u_char *) "NO\0UA\0UG\0FG\0"

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_NODE_NONE            0
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_NODE_FIND            1

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR        (u_char) 0x1f

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_AVERAGE_METHOD_AMM   0
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_AVERAGE_METHOD_WMA   1

#define NGX_STREAM_SERVER_TRAFFIC_STATUS_DEFAULT_SHM_NAME     "stream_server_traffic_status"
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_DEFAULT_SHM_SIZE     0xfffff
#define NGX_STREAM_SERVER_TRAFFIC_STATUS_DEFAULT_AVG_PERIOD   60

#define ngx_stream_server_traffic_status_add_rc(s, n) {                   \
    if(s < 200) {n->stat_1xx_counter++;}                                  \
    else if(s < 300) {n->stat_2xx_counter++;}                             \
    else if(s < 400) {n->stat_3xx_counter++;}                             \
    else if(s < 500) {n->stat_4xx_counter++;}                             \
    else {n->stat_5xx_counter++;}                                         \
}

#define ngx_stream_server_traffic_status_add_oc(o, c) {                   \
    if (o->stat_connect_counter > c->stat_connect_counter) {              \
        c->stat_connect_counter_oc++;                                     \
    }                                                                     \
    if (o->stat_in_bytes > c->stat_in_bytes) {                            \
        c->stat_in_bytes_oc++;                                            \
    }                                                                     \
    if (o->stat_out_bytes > c->stat_out_bytes) {                          \
        c->stat_out_bytes_oc++;                                           \
    }                                                                     \
    if (o->stat_1xx_counter > c->stat_1xx_counter) {                      \
        c->stat_1xx_counter_oc++;                                         \
    }                                                                     \
    if (o->stat_2xx_counter > c->stat_2xx_counter) {                      \
        c->stat_2xx_counter_oc++;                                         \
    }                                                                     \
    if (o->stat_3xx_counter > c->stat_3xx_counter) {                      \
        c->stat_2xx_counter_oc++;                                         \
    }                                                                     \
    if (o->stat_4xx_counter > c->stat_4xx_counter) {                      \
        c->stat_4xx_counter_oc++;                                         \
    }                                                                     \
    if (o->stat_5xx_counter > c->stat_5xx_counter) {                      \
        c->stat_5xx_counter_oc++;                                         \
    }                                                                     \
    if (o->stat_session_time_counter > c->stat_session_time_counter) {    \
        c->stat_session_time_counter_oc++;                                \
    }                                                                     \
}

#define ngx_stream_server_traffic_status_group_to_string(n) (u_char *) (  \
    (n > 3)                                                               \
    ? NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAMS                          \
    : NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAMS + 3 * n                  \
)

#define ngx_stream_server_traffic_status_string_to_group(s) (unsigned) (  \
{                                                                         \
    unsigned n = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;            \
    if (*s == 'N' && *(s + 1) == 'O') {                                   \
        n = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;                 \
    } else if (*s == 'U' && *(s + 1) == 'A') {                            \
        n = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UA;                 \
    } else if (*s == 'U' && *(s + 1) == 'G') {                            \
        n = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UG;                 \
    } else if (*s == 'F' && *(s + 1) == 'G') {                            \
        n = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG;                 \
    }                                                                     \
    n;                                                                    \
}                                                                         \
)

#define ngx_stream_server_traffic_status_triangle(n) (unsigned) (         \
    n * (n + 1) / 2                                                       \
)


typedef struct {
    ngx_rbtree_t                              *rbtree;

    /* array of ngx_stream_server_traffic_status_filter_t */
    ngx_array_t                               *filter_keys;

    /* array of ngx_stream_server_traffic_status_limit_t */
    ngx_array_t                               *limit_traffics;

    /* array of ngx_stream_server_traffic_status_limit_t */
    ngx_array_t                               *limit_filter_traffics;

    ngx_flag_t                                 enable;
    ngx_flag_t                                 filter_check_duplicate;
    ngx_flag_t                                 limit_check_duplicate;

    ngx_stream_upstream_main_conf_t           *upstream;
    ngx_str_t                                  shm_name;
    ssize_t                                    shm_size;
} ngx_stream_server_traffic_status_ctx_t;


typedef struct {
    ngx_shm_zone_t                            *shm_zone;
    ngx_str_t                                  shm_name;
    ngx_flag_t                                 enable;
    ngx_flag_t                                 filter;
    ngx_flag_t                                 filter_check_duplicate;

    /* array of ngx_stream_server_traffic_status_filter_t */
    ngx_array_t                               *filter_keys;

    ngx_flag_t                                 limit;
    ngx_flag_t                                 limit_check_duplicate;

    /* array of ngx_stream_server_traffic_status_limit_t */
    ngx_array_t                               *limit_traffics;

    /* array of ngx_stream_server_traffic_status_limit_t */
    ngx_array_t                               *limit_filter_traffics;

    ngx_stream_server_traffic_status_node_t    stats;
    ngx_msec_t                                 start_msec;

    ngx_flag_t                                 average_method;
    ngx_msec_t                                 average_period;

    /* array of ngx_stream_server_traffic_status_node_histogram_t */
    ngx_array_t                               *histogram_buckets;


    ngx_rbtree_node_t                        **node_caches;
} ngx_stream_server_traffic_status_conf_t;


ngx_msec_t ngx_stream_server_traffic_status_current_msec(void);
ngx_msec_int_t ngx_stream_server_traffic_status_session_time(ngx_stream_session_t *s);
ngx_msec_int_t ngx_stream_server_traffic_status_upstream_response_time(ngx_stream_session_t *s,
    uintptr_t data);

extern ngx_module_t ngx_stream_server_traffic_status_module;


#endif /* _NGX_STREAM_STS_MODULE_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
