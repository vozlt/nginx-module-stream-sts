
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include <ngx_config.h>

#include "ngx_stream_server_traffic_status_module.h"
#include "ngx_stream_server_traffic_status_filter.h"
#include "ngx_stream_server_traffic_status_limit.h"


ngx_int_t
ngx_stream_server_traffic_status_limit_handler(ngx_stream_session_t *s)
{
    ngx_int_t                                 rc;
    ngx_stream_server_traffic_status_ctx_t   *ctx;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, s->connection->log, 0,
                   "stream sts limit handler");

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    if (!stscf->limit) {
        return NGX_DECLINED;
    }

    /* limit traffic of server */
    rc = ngx_stream_server_traffic_status_limit_handler_traffic(s, ctx->limit_traffics);
    if (rc != NGX_DECLINED) {
        return rc;
    }

    rc = ngx_stream_server_traffic_status_limit_handler_traffic(s, stscf->limit_traffics);
    if (rc != NGX_DECLINED) {
        return rc;
    }

    /* limit traffic of filter */
    rc = ngx_stream_server_traffic_status_limit_handler_traffic(s, ctx->limit_filter_traffics);
    if (rc != NGX_DECLINED) {
        return rc;
    }

    rc = ngx_stream_server_traffic_status_limit_handler_traffic(s, stscf->limit_filter_traffics);
    if (rc != NGX_DECLINED) {
        return rc;
    }

    return NGX_DECLINED;
}


ngx_int_t
ngx_stream_server_traffic_status_limit_handler_traffic(ngx_stream_session_t *s,
    ngx_array_t *traffics)
{
    unsigned                                   type;
    ngx_str_t                                  variable, key, dst;
    ngx_int_t                                  rc;
    ngx_uint_t                                 i, n;
    ngx_atomic_t                               traffic_used;
    ngx_slab_pool_t                           *shpool;
    ngx_rbtree_node_t                         *node;
    ngx_stream_server_traffic_status_node_t   *stsn;
    ngx_stream_server_traffic_status_limit_t  *limits;
    ngx_stream_server_traffic_status_conf_t   *stscf;

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    rc = NGX_DECLINED;

    if (traffics == NULL) {
        return rc;
    }

    shpool = (ngx_slab_pool_t *) stscf->shm_zone->shm.addr;

    ngx_shmtx_lock(&shpool->mutex);

    limits = traffics->elts;
    n = traffics->nelts;

    for (i = 0; i < n; i++) {
        if (limits[i].variable.value.len <= 0) {
            continue;
        }

        /* init */
        traffic_used = 0;
        variable.len = 0;
        key.len = 0;
        dst.len = 0;
        type = limits[i].type;

        if (ngx_stream_complex_value(s, &limits[i].variable, &variable) != NGX_OK) {
            goto done;
        }

        if (variable.len == 0) {
            continue;
        }

        /* traffic of filter */
        if (limits[i].key.value.len > 0) {
            if (ngx_stream_complex_value(s, &limits[i].key, &key) != NGX_OK) {
                goto done;
            }

            if (key.len == 0) {
                continue;
            }

            node = ngx_stream_server_traffic_status_find_node(s, &key, type, 0);

            if (node == NULL) {
                continue;
            }

            stscf->node_caches[type] = node;

            stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

            traffic_used = (ngx_atomic_t) ngx_stream_server_traffic_status_node_member(stsn, &variable);

        /* traffic of server */
        } else {
            ngx_stream_server_traffic_status_find_name(s, &dst);

            if (ngx_stream_server_traffic_status_node_generate_key(s->connection->pool, &key, &dst, type)
                != NGX_OK || key.len == 0)
            {
                goto done;
            }

            node = ngx_stream_server_traffic_status_find_node(s, &key, type, 0);

            if (node == NULL) {
                continue;
            }

            stscf->node_caches[type] = node;

            stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

            traffic_used = (ngx_atomic_t) ngx_stream_server_traffic_status_node_member(stsn, &variable);
        }

        if (traffic_used > limits[i].size) {
            rc = limits[i].code;
            goto done;
        }
    }

done:

    ngx_shmtx_unlock(&shpool->mutex);

    return rc;
}


char *
ngx_stream_server_traffic_status_limit_traffic(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    ngx_stream_server_traffic_status_conf_t *stscf = conf;

    u_char                                    *p;
    off_t                                      size;
    ngx_str_t                                 *value, s;
    ngx_array_t                               *limit_traffics;
    ngx_stream_compile_complex_value_t         ccv;
    ngx_stream_server_traffic_status_ctx_t    *ctx;
    ngx_stream_server_traffic_status_limit_t  *traffic;

    ctx = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_server_traffic_status_module);
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;
    if (value[1].len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic() empty value pattern");
        return NGX_CONF_ERROR;
    }

    if (value[1].len > 5 && ngx_strstrn(value[1].data, "$sts_", 5 - 1)) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic() $sts_* is not allowed here");
        return NGX_CONF_ERROR;
    }

    p = (u_char *) ngx_strchr(value[1].data, ':');
    if (p == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic() empty size pattern");
        return NGX_CONF_ERROR;
    }

    s.data = p + 1;
    s.len = value[1].data + value[1].len - s.data;

    size = ngx_parse_offset(&s);
    if (size == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "limit_traffic() invalid limit size \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    limit_traffics = (cf->cmd_type == NGX_STREAM_MAIN_CONF)
                     ? ctx->limit_traffics
                     : stscf->limit_traffics;
    if (limit_traffics == NULL) {
        limit_traffics = ngx_array_create(cf->pool, 1,
                                          sizeof(ngx_stream_server_traffic_status_limit_t));
        if (limit_traffics == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    traffic = ngx_array_push(limit_traffics);
    if (traffic == NULL) {
        return NGX_CONF_ERROR;
    }

    value[1].len = p - value[1].data;

    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &traffic->variable;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    traffic->size = (ngx_atomic_t) size;

    traffic->code = (cf->args->nelts == 3)
                    ? (ngx_uint_t) ngx_atoi(value[2].data, value[2].len)
                    : NGX_STREAM_SERVICE_UNAVAILABLE;

    traffic->type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;

    traffic->key.value.len = 0;

    if (cf->cmd_type == NGX_STREAM_MAIN_CONF) {
        ctx->limit_traffics = limit_traffics;

    } else {
        stscf->limit_traffics = limit_traffics;
    }

    return NGX_CONF_OK;
}


char *
ngx_stream_server_traffic_status_limit_traffic_by_set_key(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
{
    ngx_stream_server_traffic_status_conf_t *stscf = conf;

    u_char                                    *p;
    off_t                                      size;
    ngx_str_t                                 *value, s, alpha;
    ngx_array_t                               *limit_traffics;
    ngx_stream_compile_complex_value_t         ccv;
    ngx_stream_server_traffic_status_ctx_t    *ctx;
    ngx_stream_server_traffic_status_limit_t  *traffic;

    ctx = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_server_traffic_status_module);
    if (ctx == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;
    if (value[1].len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic_by_set_key() empty key pattern");
        return NGX_CONF_ERROR;
    }

    if (value[2].len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic_by_set_key() empty value pattern");
        return NGX_CONF_ERROR;
    }

    if (value[2].len > 5 && ngx_strstrn(value[2].data, "$sts_", 5 - 1)) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "limit_traffic_by_set_key() $sts_* is not allowed here");
        return NGX_CONF_ERROR;
    }

    p = (u_char *) ngx_strchr(value[2].data, ':');
    if (p == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "limit_traffic_by_set_key() empty size pattern");
        return NGX_CONF_ERROR;
    }

    s.data = p + 1;
    s.len = value[2].data + value[2].len - s.data;

    size = ngx_parse_offset(&s);
    if (size == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "limit_traffic_by_set_key() invalid limit size \"%V\"", &value[2]);
        return NGX_CONF_ERROR;
    }

    limit_traffics = (cf->cmd_type == NGX_STREAM_MAIN_CONF)
                     ? ctx->limit_filter_traffics
                     : stscf->limit_filter_traffics;
    if (limit_traffics == NULL) {
        limit_traffics = ngx_array_create(cf->pool, 1,
                                          sizeof(ngx_stream_server_traffic_status_limit_t));
        if (limit_traffics == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    traffic = ngx_array_push(limit_traffics);
    if (traffic == NULL) {
        return NGX_CONF_ERROR;
    }

    /* set key to be limited */
    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    (void) ngx_stream_server_traffic_status_replace_chrc(&value[1], '@',
               NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR);
    ngx_str_set(&alpha, "[:alpha:]");
    if (ngx_stream_server_traffic_status_replace_strc(&value[1], &alpha, '@') != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "limit_traffic_by_set_key()::replace_strc() failed");
    }

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &traffic->key;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    /* set member to be limited */
    value[2].len = p - value[2].data;

    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &traffic->variable;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    traffic->size = (ngx_atomic_t) size;

    traffic->code = (cf->args->nelts == 4)
                    ? (ngx_uint_t) ngx_atoi(value[3].data, value[3].len)
                    : NGX_STREAM_SERVICE_UNAVAILABLE;

    traffic->type = ngx_stream_server_traffic_status_string_to_group(value[1].data);

    if (cf->cmd_type == NGX_STREAM_MAIN_CONF) {
        ctx->limit_filter_traffics = limit_traffics;

    } else {
        stscf->limit_filter_traffics = limit_traffics;
    }

    return NGX_CONF_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_limit_traffic_unique(ngx_pool_t *pool, ngx_array_t **keys)
{
    uint32_t                                         hash;
    u_char                                          *p;
    ngx_str_t                                        key;
    ngx_uint_t                                       i, n;
    ngx_array_t                                     *uniqs, *traffic_keys;
    ngx_stream_server_traffic_status_limit_t        *traffic, *traffics;
    ngx_stream_server_traffic_status_filter_uniq_t  *traffic_uniqs;

    if (*keys == NULL) {
        return NGX_OK;
    }

    uniqs = ngx_array_create(pool, 1,
                             sizeof(ngx_stream_server_traffic_status_filter_uniq_t));
    if (uniqs == NULL) {
        return NGX_ERROR;
    }

    /* init array */
    traffic_keys = NULL;
    traffic_uniqs = NULL;

    traffics = (*keys)->elts;
    n = (*keys)->nelts;

    for (i = 0; i < n; i++) {
        key.len = traffics[i].key.value.len
                  + traffics[i].variable.value.len;
        key.data = ngx_pcalloc(pool, key.len);
        if (key.data == NULL) {
            return NGX_ERROR;
        }

        p = key.data;
        p = ngx_cpymem(p, traffics[i].key.value.data,
                       traffics[i].key.value.len);
        ngx_memcpy(p, traffics[i].variable.value.data,
                   traffics[i].variable.value.len);
        hash = ngx_crc32_short(key.data, key.len);

        traffic_uniqs = ngx_array_push(uniqs);
        if (traffic_uniqs == NULL) {
            return NGX_ERROR;
        }

        traffic_uniqs->hash = hash;
        traffic_uniqs->index = i;

        if (p != NULL) {
            ngx_pfree(pool, key.data);
        }
    }

    traffic_uniqs = uniqs->elts;
    n = uniqs->nelts;

    ngx_qsort(traffic_uniqs, (size_t) n,
              sizeof(ngx_stream_server_traffic_status_filter_uniq_t),
              ngx_stream_server_traffic_status_filter_cmp_hashs);

    hash = 0;
    for (i = 0; i < n; i++) {
        if (traffic_uniqs[i].hash == hash) {
            continue;
        }

        hash = traffic_uniqs[i].hash;

        if (traffic_keys == NULL) {
            traffic_keys = ngx_array_create(pool, 1,
                                            sizeof(ngx_stream_server_traffic_status_limit_t));
            if (traffic_keys == NULL) {
                return NGX_ERROR;
            }
        }

        traffic = ngx_array_push(traffic_keys);
        if (traffic == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(traffic, &traffics[traffic_uniqs[i].index],
                   sizeof(ngx_stream_server_traffic_status_limit_t));

    }

    if ((*keys)->nelts != traffic_keys->nelts) {
        *keys = traffic_keys;
    }

    return NGX_OK;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
