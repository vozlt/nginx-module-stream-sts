
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#ifndef _NGX_STREAM_STS_VARIABLES_H_INCLUDED_
#define _NGX_STREAM_STS_VARIABLES_H_INCLUDED_


ngx_int_t ngx_stream_server_traffic_status_node_variable(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);
ngx_int_t ngx_stream_server_traffic_status_add_variables(ngx_conf_t *cf);


#endif /* _NGX_STREAM_STS_VARIABLES_H_INCLUDED_ */

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
