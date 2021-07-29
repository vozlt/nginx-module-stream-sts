
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include <ngx_config.h>

#include "ngx_stream_server_traffic_status_module.h"
#include "ngx_stream_server_traffic_status_filter.h"
#include "ngx_stream_server_traffic_status_shm.h"


static ngx_int_t ngx_stream_server_traffic_status_shm_add_node(ngx_stream_session_t *s,
    ngx_str_t *key, unsigned type);
static ngx_int_t ngx_stream_server_traffic_status_shm_add_node_upstream(ngx_stream_session_t *s,
    ngx_stream_server_traffic_status_node_t *stsn, unsigned init);

static ngx_int_t ngx_stream_server_traffic_status_shm_add_filter_node(ngx_stream_session_t *s,
    ngx_array_t *filter_keys);


static ngx_int_t
ngx_stream_server_traffic_status_shm_add_node(ngx_stream_session_t *s,
    ngx_str_t *key, unsigned type)
{
    size_t                                    size;
    unsigned                                  init;
    uint32_t                                  hash;
    ngx_slab_pool_t                          *shpool;
    ngx_rbtree_node_t                        *node;
    ngx_stream_server_traffic_status_ctx_t   *ctx;
    ngx_stream_server_traffic_status_node_t  *stsn;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    if (key->len == 0) {
        return NGX_ERROR;
    }

    shpool = (ngx_slab_pool_t *) stscf->shm_zone->shm.addr;

    ngx_shmtx_lock(&shpool->mutex);

    /* find node */
    hash = ngx_crc32_short(key->data, key->len);

    node = ngx_stream_server_traffic_status_find_node(s, key, type, hash);

    /* set common */
    if (node == NULL) {
        init = NGX_STREAM_SERVER_TRAFFIC_STATUS_NODE_NONE;
        size = offsetof(ngx_rbtree_node_t, color)
               + offsetof(ngx_stream_server_traffic_status_node_t, data)
               + key->len;

        node = ngx_slab_alloc_locked(shpool, size);
        if (node == NULL) {
            ngx_shmtx_unlock(&shpool->mutex);
            return NGX_ERROR;
        }

        stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

        node->key = hash;
        stsn->len = (u_char) key->len;
        ngx_stream_server_traffic_status_node_init(s, stsn);
        stsn->stat_upstream.type = type;
        ngx_memcpy(stsn->data, key->data, key->len);

        ngx_rbtree_insert(ctx->rbtree, node);

    } else {
        init = NGX_STREAM_SERVER_TRAFFIC_STATUS_NODE_FIND;
        stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;
        ngx_stream_server_traffic_status_node_set(s, stsn);
    }

    /* set addition */
    switch(type) {
    case NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO:
        break;

    case NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UA:
    case NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UG:
        (void) ngx_stream_server_traffic_status_shm_add_node_upstream(s, stsn, init);
        break;

    case NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG:
        break;
    }

    stscf->node_caches[type] = node;

    ngx_shmtx_unlock(&shpool->mutex);

    return NGX_OK;
}


static ngx_int_t
ngx_stream_server_traffic_status_shm_add_node_upstream(ngx_stream_session_t *s,
    ngx_stream_server_traffic_status_node_t *stsn, unsigned init)
{
    ngx_msec_int_t                            connect_time, first_byte_time, session_time;
    ngx_stream_server_traffic_status_node_t   ostsn;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    ostsn = *stsn;
    connect_time = ngx_stream_server_traffic_status_upstream_response_time(s, 2);
    first_byte_time = ngx_stream_server_traffic_status_upstream_response_time(s, 1); 
    session_time = ngx_stream_server_traffic_status_upstream_response_time(s, 0); 

    ngx_stream_server_traffic_status_node_time_queue_insert(
        &stsn->stat_upstream.connect_times,
        connect_time);
    ngx_stream_server_traffic_status_node_time_queue_insert(
        &stsn->stat_upstream.first_byte_times,
        first_byte_time);
    ngx_stream_server_traffic_status_node_time_queue_insert(
        &stsn->stat_upstream.session_times,
        session_time);

    ngx_stream_server_traffic_status_node_histogram_observe(
        &stsn->stat_upstream.connect_buckets,
        connect_time);

    ngx_stream_server_traffic_status_node_histogram_observe(
        &stsn->stat_upstream.first_byte_buckets,
        first_byte_time);

    ngx_stream_server_traffic_status_node_histogram_observe(
        &stsn->stat_upstream.session_buckets,
        session_time);

    if (init == NGX_STREAM_SERVER_TRAFFIC_STATUS_NODE_NONE) {
        stsn->stat_upstream.connect_time_counter = (ngx_atomic_uint_t) connect_time;
        stsn->stat_upstream.connect_time = (ngx_msec_t) connect_time;
        stsn->stat_upstream.first_byte_time_counter = (ngx_atomic_uint_t) first_byte_time;
        stsn->stat_upstream.first_byte_time = (ngx_msec_t) first_byte_time;
        stsn->stat_upstream.session_time_counter = (ngx_atomic_uint_t) session_time;
        stsn->stat_upstream.session_time = (ngx_msec_t) session_time;

    } else {
        stsn->stat_upstream.connect_time_counter += (ngx_atomic_uint_t) connect_time;
        stsn->stat_upstream.connect_time = ngx_stream_server_traffic_status_node_time_queue_average(
                                               &stsn->stat_upstream.connect_times,
                                               stscf->average_method, stscf->average_period);

        stsn->stat_upstream.first_byte_time_counter += (ngx_atomic_uint_t) first_byte_time;
        stsn->stat_upstream.first_byte_time = ngx_stream_server_traffic_status_node_time_queue_average(
                                                  &stsn->stat_upstream.first_byte_times,
                                                  stscf->average_method, stscf->average_period);

        stsn->stat_upstream.session_time_counter += (ngx_atomic_uint_t) session_time;
        stsn->stat_upstream.session_time = ngx_stream_server_traffic_status_node_time_queue_average(
                                               &stsn->stat_upstream.session_times,
                                               stscf->average_method, stscf->average_period);

        /* overflow */
        if (ostsn.stat_upstream.connect_time_counter
            > stsn->stat_upstream.connect_time_counter)
        {
            stsn->stat_u_connect_time_counter_oc++;
        }
        if (ostsn.stat_upstream.first_byte_time_counter
            > stsn->stat_upstream.first_byte_time_counter)
        {
            stsn->stat_u_first_byte_time_counter_oc++;
        }
        if (ostsn.stat_upstream.session_time_counter
            > stsn->stat_upstream.session_time_counter)
        {
            stsn->stat_u_session_time_counter_oc++;
        }
    }

    return NGX_OK;
}


static ngx_int_t
ngx_stream_server_traffic_status_shm_add_filter_node(ngx_stream_session_t *s,
    ngx_array_t *filter_keys)
{
    u_char                                     *p;
    unsigned                                    type;
    ngx_int_t                                   rc;
    ngx_str_t                                   key, dst, filter_key, filter_name;
    ngx_uint_t                                  i, n;
    ngx_stream_server_traffic_status_filter_t  *filters;

    if (filter_keys == NULL) {
        return NGX_OK;
    }

    filters = filter_keys->elts;
    n = filter_keys->nelts;

    for (i = 0; i < n; i++) {
        if (filters[i].filter_key.value.len <= 0) {
            continue;
        }

        if (ngx_stream_complex_value(s, &filters[i].filter_key, &filter_key) != NGX_OK) {
            return NGX_ERROR;
        }

        if (ngx_stream_complex_value(s, &filters[i].filter_name, &filter_name) != NGX_OK) {
            return NGX_ERROR;
        }

        if (filter_key.len == 0) {
            continue;
        }

        if (filter_name.len == 0) {
            type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;

            rc = ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &filter_key, type);
            if (rc != NGX_OK) {
                return NGX_ERROR;
            }

        } else {
            type = filter_name.len
                   ? NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG
                   : NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;

            dst.len = filter_name.len + sizeof("@") - 1 + filter_key.len;
            dst.data = ngx_pnalloc(s->connection->pool, dst.len);
            if (dst.data == NULL) {
                return NGX_ERROR;
            }

            p = dst.data;
            p = ngx_cpymem(p, filter_name.data, filter_name.len);
            *p++ = NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR;
            p = ngx_cpymem(p, filter_key.data, filter_key.len);

            rc = ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &dst, type);
            if (rc != NGX_OK) {
                return NGX_ERROR;
            }
        }

        rc = ngx_stream_server_traffic_status_shm_add_node(s, &key, type);
        if (rc != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                          "shm_add_filter_node::shm_add_node(\"%V\") failed", &key);
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_shm_add_server(ngx_stream_session_t *s)
{
    unsigned   type;
    ngx_int_t  rc;
    ngx_str_t  key, dst;

    rc = ngx_stream_server_traffic_status_find_name(s, &dst);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;

    rc = ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &dst, type);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    return ngx_stream_server_traffic_status_shm_add_node(s, &key, type);
}


ngx_int_t
ngx_stream_server_traffic_status_shm_add_filter(ngx_stream_session_t *s)
{
    ngx_int_t                                 rc;
    ngx_stream_server_traffic_status_ctx_t   *ctx;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    if (!stscf->filter) {
        return NGX_OK;
    }

    if (ctx->filter_keys != NULL) {
        rc = ngx_stream_server_traffic_status_shm_add_filter_node(s, ctx->filter_keys);
        if (rc != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                          "shm_add_filter::shm_add_filter_node(\"stream\") failed");
        }
    }

    if (stscf->filter_keys != NULL) {
        rc = ngx_stream_server_traffic_status_shm_add_filter_node(s, stscf->filter_keys);
        if (rc != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                          "shm_add_filter::shm_add_filter_node(\"server\") failed");
        }
    }

    return NGX_OK;
}

ngx_int_t
ngx_stream_server_traffic_status_shm_add_upstream(ngx_stream_session_t *s)
{
    u_char                           *p;
    unsigned                          type;
    ngx_int_t                         rc;
    ngx_str_t                        *host, key, dst;
    ngx_uint_t                        i;
    ngx_stream_upstream_t            *u;
    ngx_stream_upstream_srv_conf_t   *uscf, **uscfp;
    ngx_stream_upstream_main_conf_t  *umcf;
    ngx_stream_upstream_state_t      *state;

    if (s->upstream_states == NULL || s->upstream_states->nelts == 0
        || s->upstream->state == NULL)
    {
        return NGX_OK;
    }

    u = s->upstream;

    if (u->resolved == NULL) {
        uscf = u->upstream;
    } else {
        host = &u->resolved->host;

        umcf = ngx_stream_get_module_main_conf(s, ngx_stream_upstream_module);

        uscfp = umcf->upstreams.elts;

        for (i = 0; i < umcf->upstreams.nelts; i++) {

            uscf = uscfp[i];

            if (uscf->host.len == host->len
                && ((uscf->port == 0 && u->resolved->no_port)
                     || uscf->port == u->resolved->port)
                && ngx_strncasecmp(uscf->host.data, host->data, host->len) == 0)
            {
                goto found;
            }
        }

        /* routine for proxy_pass|fastcgi_pass|... $variables */
        uscf = ngx_pcalloc(s->connection->pool, sizeof(ngx_stream_upstream_srv_conf_t));
        if (uscf == NULL) {
            return NGX_ERROR;
        }

        uscf->host = u->resolved->host;
        uscf->port = u->resolved->port;
    }

found:

    state = s->upstream_states->elts;
    if (state[0].peer == NULL) {
        ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                      "shm_add_upstream::peer failed");
        return NGX_ERROR;
    }

    dst.len = (uscf->port ? 0 : uscf->host.len + sizeof("@") - 1) + state[0].peer->len;
    dst.data = ngx_pnalloc(s->connection->pool, dst.len);
    if (dst.data == NULL) {
        return NGX_ERROR;
    }

    p = dst.data;
    if (uscf->port) {
        p = ngx_cpymem(p, state[0].peer->data, state[0].peer->len);
        type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UA;

    } else {
        p = ngx_cpymem(p, uscf->host.data, uscf->host.len);
        *p++ = NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR;
        p = ngx_cpymem(p, state[0].peer->data, state[0].peer->len);
        type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_UG;
    }

    rc = ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &dst, type);
    if (rc != NGX_OK) {
        return NGX_ERROR;
    }

    rc = ngx_stream_server_traffic_status_shm_add_node(s, &key, type);
    if (rc != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                      "shm_add_upstream::shm_add_node(\"%V\") failed", &key);
    }

    return NGX_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
