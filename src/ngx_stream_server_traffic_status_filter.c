
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include <ngx_config.h>

#include "ngx_stream_server_traffic_status_module.h"
#include "ngx_stream_server_traffic_status_filter.h"


int ngx_libc_cdecl
ngx_stream_server_traffic_status_filter_cmp_hashs(const void *one, const void *two)
{
    ngx_stream_server_traffic_status_filter_uniq_t *first =
                           (ngx_stream_server_traffic_status_filter_uniq_t *) one;
    ngx_stream_server_traffic_status_filter_uniq_t *second =
                           (ngx_stream_server_traffic_status_filter_uniq_t *) two;

    return (first->hash - second->hash);
}


int ngx_libc_cdecl
ngx_stream_server_traffic_status_filter_cmp_keys(const void *one, const void *two)
{
    ngx_stream_server_traffic_status_filter_key_t *first =
                            (ngx_stream_server_traffic_status_filter_key_t *) one;
    ngx_stream_server_traffic_status_filter_key_t *second =
                            (ngx_stream_server_traffic_status_filter_key_t *) two;

    return (int) ngx_strcmp(first->key.data, second->key.data);
}


ngx_int_t
ngx_stream_server_traffic_status_filter_unique(ngx_pool_t *pool, ngx_array_t **keys)
{
    uint32_t                                         hash;
    u_char                                          *p;
    ngx_str_t                                        key;
    ngx_uint_t                                       i, n;
    ngx_array_t                                     *uniqs, *filter_keys;
    ngx_stream_server_traffic_status_filter_t       *filter, *filters;
    ngx_stream_server_traffic_status_filter_uniq_t  *filter_uniqs;

    if (*keys == NULL) {
        return NGX_OK;
    }

    uniqs = ngx_array_create(pool, 1,
                             sizeof(ngx_stream_server_traffic_status_filter_uniq_t));
    if (uniqs == NULL) {
        return NGX_ERROR;
    }

    /* init array */
    filter_keys = NULL;
    filter_uniqs = NULL;

    filters = (*keys)->elts;
    n = (*keys)->nelts;

    for (i = 0; i < n; i++) {
        key.len = filters[i].filter_key.value.len
                  + filters[i].filter_name.value.len;
        key.data = ngx_pcalloc(pool, key.len);
        if (key.data == NULL) {
            return NGX_ERROR;
        }

        p = key.data;
        p = ngx_cpymem(p, filters[i].filter_key.value.data,
                       filters[i].filter_key.value.len);
        ngx_memcpy(p, filters[i].filter_name.value.data,
                   filters[i].filter_name.value.len);
        hash = ngx_crc32_short(key.data, key.len);

        filter_uniqs = ngx_array_push(uniqs);
        if (filter_uniqs == NULL) {
            return NGX_ERROR;
        }

        filter_uniqs->hash = hash;
        filter_uniqs->index = i;

        if (p != NULL) {
            ngx_pfree(pool, key.data);
        }
    }

    filter_uniqs = uniqs->elts;
    n = uniqs->nelts;

    ngx_qsort(filter_uniqs, (size_t) n,
              sizeof(ngx_stream_server_traffic_status_filter_uniq_t),
              ngx_stream_server_traffic_status_filter_cmp_hashs);

    hash = 0;
    for (i = 0; i < n; i++) {
        if (filter_uniqs[i].hash == hash) {
            continue;
        }

        hash = filter_uniqs[i].hash;

        if (filter_keys == NULL) {
            filter_keys = ngx_array_create(pool, 1,
                                           sizeof(ngx_stream_server_traffic_status_filter_t));
            if (filter_keys == NULL) {
                return NGX_ERROR;
            }
        }

        filter = ngx_array_push(filter_keys);
        if (filter == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(filter, &filters[filter_uniqs[i].index],
                   sizeof(ngx_stream_server_traffic_status_filter_t));

    }

    if ((*keys)->nelts != filter_keys->nelts) {
        *keys = filter_keys;
    }

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_filter_get_keys(ngx_stream_session_t *s,
    ngx_array_t **filter_keys, ngx_rbtree_node_t *node)
{
    ngx_int_t                                       rc;
    ngx_str_t                                       key;
    ngx_stream_server_traffic_status_ctx_t         *ctx;
    ngx_stream_server_traffic_status_node_t        *stsn;
    ngx_stream_server_traffic_status_filter_key_t  *keys;

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);

    if (node != ctx->rbtree->sentinel) {
        stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

        if (stsn->stat_upstream.type == NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG) {
            key.data = stsn->data;
            key.len = stsn->len;

            rc = ngx_stream_server_traffic_status_node_position_key(&key, 1);
            if (rc != NGX_OK) {
                goto next;
            }

            if (*filter_keys == NULL) {
                *filter_keys = ngx_array_create(s->connection->pool, 1,
                                   sizeof(ngx_stream_server_traffic_status_filter_key_t));

                if (*filter_keys == NULL) {
                    ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                                  "filter_get_keys::ngx_array_create() failed");
                    return NGX_ERROR;
                }
            }

            keys = ngx_array_push(*filter_keys);
            if (keys == NULL) {
                ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                              "filter_get_keys::ngx_array_push() failed");
                return NGX_ERROR;
            }

            keys->key.len = key.len;
            /* 1 byte for terminating '\0' for ngx_strcmp() */
            keys->key.data = ngx_pcalloc(s->connection->pool, key.len + 1);
            if (keys->key.data == NULL) {
                ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                              "filter_get_keys::ngx_pcalloc() failed");
            }

            ngx_memcpy(keys->key.data, key.data, key.len);
        }
next:
        rc = ngx_stream_server_traffic_status_filter_get_keys(s, filter_keys, node->left);
        if (rc != NGX_OK) {
            return rc;
        }

        rc = ngx_stream_server_traffic_status_filter_get_keys(s, filter_keys, node->right);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_filter_get_nodes(ngx_stream_session_t *s,
    ngx_array_t **filter_nodes, ngx_str_t *name,
    ngx_rbtree_node_t *node)
{
    ngx_int_t                                        rc;
    ngx_str_t                                        key;
    ngx_stream_server_traffic_status_ctx_t          *ctx;
    ngx_stream_server_traffic_status_node_t         *stsn;
    ngx_stream_server_traffic_status_filter_node_t  *nodes;

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);

    if (node != ctx->rbtree->sentinel) {
        stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

        if (stsn->stat_upstream.type == NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_FG) {
            key.data = stsn->data;
            key.len = stsn->len;

            rc = ngx_stream_server_traffic_status_node_position_key(&key, 1);
            if (rc != NGX_OK) {
                goto next;
            }

            if (name->len != key.len) {
                goto next;
            }

            if (ngx_strncmp(name->data, key.data, key.len) != 0) {
                goto next;
            }

            if (*filter_nodes == NULL) {
                *filter_nodes = ngx_array_create(s->connection->pool, 1,
                                    sizeof(ngx_stream_server_traffic_status_filter_node_t));

                if (*filter_nodes == NULL) {
                    ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                                  "filter_get_nodes::ngx_array_create() failed");
                    return NGX_ERROR;
                }
            }

            nodes = ngx_array_push(*filter_nodes);
            if (nodes == NULL) {
                ngx_log_error(NGX_LOG_ERR, s->connection->log, 0,
                              "filter_get_nodes::ngx_array_push() failed");
                return NGX_ERROR;
            }

            nodes->node = stsn;
        }
next:
        rc = ngx_stream_server_traffic_status_filter_get_nodes(s, filter_nodes, name, node->left);
        if (rc != NGX_OK) {
            return rc;
        }

        rc = ngx_stream_server_traffic_status_filter_get_nodes(s, filter_nodes, name, node->right);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    return NGX_OK;
}


char *
ngx_stream_server_traffic_status_filter_by_set_key(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    ngx_stream_server_traffic_status_conf_t *stscf = conf;

    ngx_str_t                                  *value, name;
    ngx_array_t                                *filter_keys;
    ngx_stream_compile_complex_value_t          ccv;
    ngx_stream_server_traffic_status_ctx_t     *ctx;
    ngx_stream_server_traffic_status_filter_t  *filter;

    ctx = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_server_traffic_status_module);
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;
    if (value[1].len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "empty key pattern");
        return NGX_CONF_ERROR;
    }

    filter_keys = (cf->cmd_type == NGX_STREAM_MAIN_CONF) ? ctx->filter_keys : stscf->filter_keys;
    if (filter_keys == NULL) {
        filter_keys = ngx_array_create(cf->pool, 1,
                                       sizeof(ngx_stream_server_traffic_status_filter_t));
        if (filter_keys == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    filter = ngx_array_push(filter_keys);
    if (filter == NULL) {
        return NGX_CONF_ERROR;
    }

    /* first argument process */
    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &filter->filter_key;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    /* second argument process */
    if (cf->args->nelts == 3) {
        name = value[2];

    } else {
        ngx_str_set(&name, "");
    }

    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &name;
    ccv.complex_value = &filter->filter_name;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cf->cmd_type == NGX_STREAM_MAIN_CONF) {
        ctx->filter_keys = filter_keys;

    } else {
        stscf->filter_keys = filter_keys;
    }

    return NGX_CONF_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
