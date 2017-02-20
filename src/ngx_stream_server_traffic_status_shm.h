
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_STREAM_STS_SHM_H_INCLUDED_
#define _NGX_STREAM_STS_SHM_H_INCLUDED_


ngx_int_t ngx_stream_server_traffic_status_shm_add_server(ngx_stream_session_t *s);
ngx_int_t ngx_stream_server_traffic_status_shm_add_filter(ngx_stream_session_t *s);
ngx_int_t ngx_stream_server_traffic_status_shm_add_upstream(ngx_stream_session_t *s);


#endif /* _NGX_STREAM_STS_SHM_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
