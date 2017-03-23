
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include <ngx_config.h>

#include "ngx_stream_server_traffic_status_module.h"
#include "ngx_stream_server_traffic_status_variables.h"


static ngx_stream_variable_t  ngx_stream_server_traffic_status_vars[] = {

    { ngx_string("sts_connect_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_connect_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_in_bytes"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_in_bytes),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_out_bytes"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_out_bytes),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_1xx_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_1xx_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_2xx_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_2xx_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_3xx_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_3xx_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_4xx_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_4xx_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_5xx_counter"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_5xx_counter),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_string("sts_session_time"), NULL,
      ngx_stream_server_traffic_status_node_variable,
      offsetof(ngx_stream_server_traffic_status_node_t, stat_session_time),
      NGX_STREAM_VAR_NOCACHEABLE, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


ngx_int_t
ngx_stream_server_traffic_status_node_variable(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data)
{
    u_char                                   *p;
    unsigned                                  type;
    ngx_int_t                                 rc;
    ngx_str_t                                 key, dst;
    ngx_slab_pool_t                          *shpool;
    ngx_rbtree_node_t                        *node;
    ngx_stream_server_traffic_status_node_t  *stsn;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    rc = ngx_stream_server_traffic_status_find_name(s, &dst);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;

    rc = ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &dst, type);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    if (key.len == 0) {
        return NGX_ERROR;
    }

    shpool = (ngx_slab_pool_t *) stscf->shm_zone->shm.addr;

    ngx_shmtx_lock(&shpool->mutex);

    node = ngx_stream_server_traffic_status_find_node(s, &key, type, 0);

    if (node == NULL) {
        goto not_found;
    }

    p = ngx_pnalloc(s->connection->pool, NGX_ATOMIC_T_LEN);
    if (p == NULL) {
        goto not_found;
    }

    stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

    v->len = ngx_sprintf(p, "%uA", *((ngx_atomic_t *) ((char *) stsn + data))) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    goto done;

not_found:

    v->not_found = 1;

done:

    stscf->node_caches[type] = node;

    ngx_shmtx_unlock(&shpool->mutex);

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_add_variables(ngx_conf_t *cf)
{
    ngx_stream_variable_t  *var, *v;
    
    for (v = ngx_stream_server_traffic_status_vars; v->name.len; v++) {
        var = ngx_stream_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
