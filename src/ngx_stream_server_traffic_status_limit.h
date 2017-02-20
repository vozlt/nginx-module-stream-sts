
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_STREAM_STS_LIMIT_H_INCLUDED_
#define _NGX_STREAM_STS_LIMIT_H_INCLUDED_


typedef struct {
    ngx_stream_complex_value_t  key;
    ngx_stream_complex_value_t  variable;
    ngx_atomic_t                size;
    ngx_uint_t                  code;
    unsigned                    type;        /* unsigned type:5 */
} ngx_stream_server_traffic_status_limit_t;


ngx_int_t ngx_stream_server_traffic_status_limit_handler(ngx_stream_session_t *s);
ngx_int_t ngx_stream_server_traffic_status_limit_handler_traffic(ngx_stream_session_t *s,
    ngx_array_t *traffics);

ngx_int_t ngx_stream_server_traffic_status_limit_traffic_unique(
    ngx_pool_t *pool, ngx_array_t **keys);
char *ngx_stream_server_traffic_status_limit_traffic(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
char *ngx_stream_server_traffic_status_limit_traffic_by_set_key(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);


#endif /* _NGX_STREAM_STS_LIMIT_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
