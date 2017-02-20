
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_STREAM_STS_FILTER_H_INCLUDED_
#define _NGX_STREAM_STS_FILTER_H_INCLUDED_


typedef struct {
    ngx_stream_complex_value_t                filter_key;
    ngx_stream_complex_value_t                filter_name;
} ngx_stream_server_traffic_status_filter_t;


typedef struct {
    ngx_str_t                                 key;
} ngx_stream_server_traffic_status_filter_key_t;


typedef struct {
    uint32_t                                  hash;
    ngx_uint_t                                index;
} ngx_stream_server_traffic_status_filter_uniq_t;


typedef struct {
    ngx_stream_server_traffic_status_node_t  *node;
} ngx_stream_server_traffic_status_filter_node_t;


int ngx_libc_cdecl ngx_stream_server_traffic_status_filter_cmp_hashs(
    const void *one, const void *two);
int ngx_libc_cdecl ngx_stream_server_traffic_status_filter_cmp_keys(
    const void *one, const void *two);
ngx_int_t ngx_stream_server_traffic_status_filter_unique(
    ngx_pool_t *pool, ngx_array_t **keys);
ngx_int_t ngx_stream_server_traffic_status_filter_get_keys(
    ngx_stream_session_t *s, ngx_array_t **filter_keys,
    ngx_rbtree_node_t *node);
ngx_int_t ngx_stream_server_traffic_status_filter_get_nodes(
    ngx_stream_session_t *s, ngx_array_t **filter_nodes,
    ngx_str_t *name, ngx_rbtree_node_t *node);


char *ngx_stream_server_traffic_status_filter_by_set_key(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);


#endif /* _NGX_STREAM_STS_FILTER_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
