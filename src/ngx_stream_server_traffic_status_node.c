
/*
 * Copyright (C) YoungJoo Kim (vozlt)
 */


#include <ngx_config.h>

#include "ngx_stream_server_traffic_status_module.h"
#include "ngx_stream_server_traffic_status_node.h"


ngx_int_t
ngx_stream_server_traffic_status_node_generate_key(ngx_pool_t *pool,
    ngx_str_t *buf, ngx_str_t *dst, unsigned type)
{
    size_t   len;
    u_char  *p;

    len = ngx_strlen(ngx_stream_server_traffic_status_group_to_string(type));

    buf->len = len + sizeof("@") - 1 + dst->len;
    buf->data = ngx_pcalloc(pool, buf->len);
    if (buf->data == NULL) {
        *buf = *dst;
        return NGX_ERROR;
    }

    p = buf->data;

    p = ngx_cpymem(p, ngx_stream_server_traffic_status_group_to_string(type), len);
    *p++ = NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR;
    p = ngx_cpymem(p, dst->data, dst->len);

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_node_position_key(ngx_str_t *buf, size_t pos)
{
    size_t   n, c, len;
    u_char  *p, *s;

    n = buf->len + 1;
    c = len = 0;
    p = s = buf->data;

    while (--n) {
        if (*p == NGX_STREAM_SERVER_TRAFFIC_STATUS_KEY_SEPARATOR) {
            if (pos == c) {
                break;
            }
            s = (p + 1);
            c++;
        }
        p++;
        len = (p - s);
    }

    if (pos > c || len == 0) {
        return NGX_ERROR;
    }

    buf->data = s;
    buf->len = len;

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_find_name(ngx_stream_session_t *s,
    ngx_str_t *buf)
{
    u_char      addr[NGX_SOCKADDR_STRLEN];
    ngx_str_t   str, protocol;
    ngx_uint_t  port;

    str.len = NGX_SOCKADDR_STRLEN;
    str.data = addr;

    if (ngx_connection_local_sockaddr(s->connection, &str, 0) != NGX_OK) {
        return NGX_ERROR;
    }

    str.data = ngx_pnalloc(s->connection->pool, str.len);
    if (str.data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(str.data, addr, str.len);

    port = ngx_inet_get_port(s->connection->local_sockaddr);

    protocol.len = 3;
    protocol.data = (u_char *) (s->connection->type == SOCK_DGRAM ? "UDP" : "TCP");

    buf->len = str.len + sizeof("[]:65535") + sizeof("TCP");
    buf->data = ngx_pnalloc(s->connection->pool, buf->len);
    if (buf->data == NULL) {
        return NGX_ERROR;
    }

    /* protocol:port:addr */
    buf->len = ngx_sprintf(buf->data, "%V:%ui:%V", &protocol, port, &str) - buf->data;

    return NGX_OK;
}


ngx_rbtree_node_t *
ngx_stream_server_traffic_status_find_node(ngx_stream_session_t *s,
    ngx_str_t *key, unsigned type, uint32_t key_hash)
{
    uint32_t                                  hash;
    ngx_rbtree_node_t                        *node;
    ngx_stream_server_traffic_status_ctx_t   *ctx;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    ctx = ngx_stream_get_module_main_conf(s, ngx_stream_server_traffic_status_module);
    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    hash = key_hash;

    if (hash == 0) {
        hash = ngx_crc32_short(key->data, key->len);
    }

    if (stscf->node_caches[type] != NULL) {
        if (stscf->node_caches[type]->key == hash) {
            node = stscf->node_caches[type];
            goto found;
        }
    }

    node = ngx_stream_server_traffic_status_node_lookup(ctx->rbtree, key, hash);

found:

    return node;
}


ngx_rbtree_node_t *
ngx_stream_server_traffic_status_node_lookup(ngx_rbtree_t *rbtree, ngx_str_t *key,
    uint32_t hash)
{
    ngx_int_t                                 rc;
    ngx_rbtree_node_t                        *node, *sentinel;
    ngx_stream_server_traffic_status_node_t  *stsn;

    node = rbtree->root;
    sentinel = rbtree->sentinel;

    while (node != sentinel) {

        if (hash < node->key) {
            node = node->left;
            continue;
        }

        if (hash > node->key) {
            node = node->right;
            continue;
        }

        /* hash == node->key */

        stsn = (ngx_stream_server_traffic_status_node_t *) &node->color;

        rc = ngx_memn2cmp(key->data, stsn->data, key->len, (size_t) stsn->len);
        if (rc == 0) {
            return node;
        }

        node = (rc < 0) ? node->left : node->right;
    }

    return NULL;
}


void
ngx_stream_server_traffic_status_node_zero(ngx_stream_server_traffic_status_node_t *stsn)
{
    stsn->stat_connect_counter = 0;
    stsn->stat_in_bytes = 0;
    stsn->stat_out_bytes = 0;
    stsn->stat_1xx_counter = 0;
    stsn->stat_2xx_counter = 0;
    stsn->stat_3xx_counter = 0;
    stsn->stat_4xx_counter = 0;
    stsn->stat_5xx_counter = 0;

    stsn->stat_session_time_counter = 0;
    stsn->stat_session_time = 0;

    stsn->stat_connect_counter_oc = 0;
    stsn->stat_in_bytes_oc = 0;
    stsn->stat_out_bytes_oc = 0;
    stsn->stat_1xx_counter_oc = 0;
    stsn->stat_2xx_counter_oc = 0;
    stsn->stat_3xx_counter_oc = 0;
    stsn->stat_4xx_counter_oc = 0;
    stsn->stat_5xx_counter_oc = 0;
    stsn->stat_session_time_counter_oc = 0;
    stsn->stat_u_connect_time_counter_oc = 0;
    stsn->stat_u_first_byte_time_counter_oc = 0;
    stsn->stat_u_session_time_counter_oc = 0;
}


void
ngx_stream_server_traffic_status_node_init(ngx_stream_session_t *s,
    ngx_stream_server_traffic_status_node_t *stsn)
{
    ngx_uint_t status = s->status;

    /* init serverZone */
    ngx_stream_server_traffic_status_node_zero(stsn);
    ngx_stream_server_traffic_status_node_time_queue_init(&stsn->stat_session_times);
    ngx_stream_server_traffic_status_node_histogram_bucket_init(s,
        &stsn->stat_session_buckets);
    stsn->port = ngx_inet_get_port(s->connection->local_sockaddr);
    stsn->protocol = s->connection->type;

    /* init upstreamZone */
    stsn->stat_upstream.type = NGX_STREAM_SERVER_TRAFFIC_STATUS_UPSTREAM_NO;
    stsn->stat_upstream.connect_time_counter = 0;
    stsn->stat_upstream.connect_time = 0;
    stsn->stat_upstream.first_byte_time_counter = 0;
    stsn->stat_upstream.first_byte_time = 0;
    stsn->stat_upstream.session_time_counter = 0;
    stsn->stat_upstream.session_time = 0;
    ngx_stream_server_traffic_status_node_time_queue_init(
        &stsn->stat_upstream.connect_times);
    ngx_stream_server_traffic_status_node_time_queue_init(
        &stsn->stat_upstream.first_byte_times);
    ngx_stream_server_traffic_status_node_time_queue_init(
        &stsn->stat_upstream.session_times);
    ngx_stream_server_traffic_status_node_histogram_bucket_init(s,
        &stsn->stat_upstream.connect_buckets);
    ngx_stream_server_traffic_status_node_histogram_bucket_init(s,
        &stsn->stat_upstream.first_byte_buckets);
    ngx_stream_server_traffic_status_node_histogram_bucket_init(s,
        &stsn->stat_upstream.session_buckets);

    /* set serverZone */
    stsn->stat_connect_counter = 1;
    stsn->stat_in_bytes = (ngx_atomic_uint_t) s->received;
    stsn->stat_out_bytes = (ngx_atomic_uint_t) s->connection->sent;

    ngx_stream_server_traffic_status_add_rc(status, stsn);

    stsn->stat_session_time = (ngx_msec_t) ngx_stream_server_traffic_status_session_time(s);
    stsn->stat_session_time_counter = (ngx_atomic_uint_t) stsn->stat_session_time; 

    ngx_stream_server_traffic_status_node_time_queue_insert(&stsn->stat_session_times,
        stsn->stat_session_time);
}


void
ngx_stream_server_traffic_status_node_set(ngx_stream_session_t *s,
    ngx_stream_server_traffic_status_node_t *stsn)
{
    ngx_uint_t                                status;
    ngx_msec_int_t                            ms;
    ngx_stream_server_traffic_status_node_t   ostsn;
    ngx_stream_server_traffic_status_conf_t  *stscf;

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    status = s->status;
    ostsn = *stsn;

    stsn->stat_connect_counter++;
    stsn->stat_in_bytes += (ngx_atomic_uint_t) s->received;
    stsn->stat_out_bytes += (ngx_atomic_uint_t) s->connection->sent;

    ngx_stream_server_traffic_status_add_rc(status, stsn);

    ms = ngx_stream_server_traffic_status_session_time(s);

    stsn->stat_session_time_counter += (ngx_atomic_uint_t) ms;

    ngx_stream_server_traffic_status_node_time_queue_insert(&stsn->stat_session_times,
                                                            ms);

    ngx_stream_server_traffic_status_node_histogram_observe(&stsn->stat_session_buckets,
                                                            ms);

    stsn->stat_session_time = ngx_stream_server_traffic_status_node_time_queue_average(
                                  &stsn->stat_session_times, stscf->average_method,
                                  stscf->average_period);

    ngx_stream_server_traffic_status_add_oc((&ostsn), stsn);
}


void
ngx_stream_server_traffic_status_node_time_queue_zero(
    ngx_stream_server_traffic_status_node_time_queue_t *q)
{
    ngx_memzero(q, sizeof(ngx_stream_server_traffic_status_node_time_queue_t));
}


void
ngx_stream_server_traffic_status_node_time_queue_init(
    ngx_stream_server_traffic_status_node_time_queue_t *q)
{
    ngx_stream_server_traffic_status_node_time_queue_zero(q);
    q->rear = NGX_STREAM_SERVER_TRAFFIC_STATUS_DEFAULT_QUEUE_LEN - 1;
    q->len = NGX_STREAM_SERVER_TRAFFIC_STATUS_DEFAULT_QUEUE_LEN;
}


ngx_int_t
ngx_stream_server_traffic_status_node_time_queue_push(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_msec_int_t x)
{
    if ((q->rear + 1) % q->len == q->front) {
        return NGX_ERROR;
    }

    q->times[q->rear].time = ngx_stream_server_traffic_status_current_msec();
    q->times[q->rear].msec = x;
    q->rear = (q->rear + 1) % q->len;

    return NGX_OK;
}


ngx_int_t
ngx_stream_server_traffic_status_node_time_queue_pop(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_stream_server_traffic_status_node_time_t *x)
{
    if (q->front == q->rear) {
        return NGX_ERROR;
    }

    *x = q->times[q->front];
    q->front = (q->front + 1) % q->len;

    return NGX_OK;
}


void
ngx_stream_server_traffic_status_node_time_queue_insert(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_msec_int_t x)
{
    ngx_int_t                                     rc;
    ngx_stream_server_traffic_status_node_time_t  rx;
    rc = ngx_stream_server_traffic_status_node_time_queue_pop(q, &rx)
         | ngx_stream_server_traffic_status_node_time_queue_push(q, x);

    if (rc != NGX_OK) {
        ngx_stream_server_traffic_status_node_time_queue_init(q);
    }
}


ngx_msec_t
ngx_stream_server_traffic_status_node_time_queue_average(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_int_t method, ngx_msec_t period)
{
    ngx_msec_t  avg;

    if (method == NGX_STREAM_SERVER_TRAFFIC_STATUS_AVERAGE_METHOD_AMM) {
        avg = ngx_stream_server_traffic_status_node_time_queue_amm(q, period);
    } else {
        avg = ngx_stream_server_traffic_status_node_time_queue_wma(q, period);
    }

    return avg;
}


ngx_msec_t
ngx_stream_server_traffic_status_node_time_queue_amm(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_msec_t period)
{
    ngx_int_t   c, i, j, k;
    ngx_msec_t  x, current_msec;

    current_msec = ngx_stream_server_traffic_status_current_msec();

    c = 0;
    x = period ? (current_msec - period) : 0;

    for (i = q->front, j = 1, k = 0; i != q->rear; i = (i + 1) % q->len, j++) {
        if (x < q->times[i].time) {
            k += (ngx_int_t) q->times[i].msec;
            c++;
        }
    }

    return (c == 0) ? (ngx_msec_t) 0 : (ngx_msec_t) (k / c);
}


ngx_msec_t
ngx_stream_server_traffic_status_node_time_queue_wma(
    ngx_stream_server_traffic_status_node_time_queue_t *q,
    ngx_msec_t period)
{
    ngx_int_t   c, i, j, k;
    ngx_msec_t  x, current_msec;

    current_msec = ngx_stream_server_traffic_status_current_msec();

    c = 0;
    x = period ? (current_msec - period) : 0;

    for (i = q->front, j = 1, k = 0; i != q->rear; i = (i + 1) % q->len, j++) {
        if (x < q->times[i].time) {
            k += (ngx_int_t) q->times[i].msec * ++c;
        }
    }

    return (c == 0) ? (ngx_msec_t) 0 : (ngx_msec_t)
           (k / (ngx_int_t) ngx_stream_server_traffic_status_triangle(c));
}


void
ngx_stream_server_traffic_status_node_histogram_bucket_init(
    ngx_stream_session_t *s,
    ngx_stream_server_traffic_status_node_histogram_bucket_t *b)
{
    ngx_uint_t                                          i, n;
    ngx_stream_server_traffic_status_conf_t            *stscf;
    ngx_stream_server_traffic_status_node_histogram_t  *buckets;

    stscf = ngx_stream_get_module_srv_conf(s, ngx_stream_server_traffic_status_module);

    if (stscf->histogram_buckets == NULL) {
        b->len = 0;
        return;
    }

    buckets = stscf->histogram_buckets->elts;
    n = stscf->histogram_buckets->nelts;
    b->len = n;

    for (i = 0; i < n; i++) {
        b->buckets[i].msec = buckets[i].msec;
        b->buckets[i].counter = 0;
    }
}


void
ngx_stream_server_traffic_status_node_histogram_observe(
    ngx_stream_server_traffic_status_node_histogram_bucket_t *b,
    ngx_msec_int_t x)
{
    ngx_uint_t  i, n;

    n = b->len;

    for (i = 0; i < n; i++) {
        if (x <= b->buckets[i].msec) {
            b->buckets[i].counter++;
        }
    }
}


ngx_int_t
ngx_stream_server_traffic_status_node_member_cmp(ngx_str_t *member, const char *name)
{
    if (member->len == ngx_strlen(name) && ngx_strncmp(name, member->data, member->len) == 0) {
        return 0;
    }

    return 1;
}


ngx_atomic_uint_t
ngx_stream_server_traffic_status_node_member(ngx_stream_server_traffic_status_node_t *stsn,
    ngx_str_t *member)
{
    if (ngx_stream_server_traffic_status_node_member_cmp(member, "connect") == 0)
    {
        return stsn->stat_connect_counter;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "in") == 0)
    {
        return stsn->stat_in_bytes;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "out") == 0)
    {
        return stsn->stat_out_bytes;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "1xx") == 0)
    {
        return stsn->stat_1xx_counter;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "2xx") == 0)
    {
        return stsn->stat_2xx_counter;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "3xx") == 0)
    {
        return stsn->stat_3xx_counter;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "4xx") == 0)
    {
        return stsn->stat_4xx_counter;
    }
    else if (ngx_stream_server_traffic_status_node_member_cmp(member, "5xx") == 0)
    {
        return stsn->stat_5xx_counter;
    }

    return 0;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
