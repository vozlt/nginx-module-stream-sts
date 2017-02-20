# vi:set ft=perl ts=4 sw=4 et fdm=marker:

use Test::Nginx::Socket;

plan tests => repeat_each() * blocks() * 6;
no_shuffle();
run_tests();

__DATA__

=== TEST 1: server_traffic_status_limit_check_duplicate on
--- main_config
    stream {
        server_traffic_status_zone;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_limit_check_duplicate on;
            server_traffic_status_filter_by_set_key $protocol protocol;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP connect:8;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP connect:4;
            proxy_pass backend;
        }

    }
--- http_config
    stream_server_traffic_status_zone;
--- config
    location /status {
        stream_server_traffic_status_display;
        stream_server_traffic_status_display_format json;
        access_log off;
    }
    location /stream {
        proxy_pass http://localhost:1985/return;
    }
--- user_files eval
[
    ['return/file.txt' => '{"return":"OK"}']
]
--- request eval
[
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    200,
    200,
    200
]



=== TEST 2: server_traffic_status_limit_check_duplicate off
--- main_config
    stream {
        server_traffic_status_zone;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_limit_check_duplicate off;
            server_traffic_status_filter_by_set_key $protocol protocol;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP connect:8;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP connect:4;
            proxy_pass backend;
        }

    }
--- http_config
    stream_server_traffic_status_zone;
--- config
    location /status {
        stream_server_traffic_status_display;
        stream_server_traffic_status_display_format json;
        access_log off;
    }
    location /stream {
        proxy_pass http://localhost:1985/return;
    }
--- user_files eval
[
    ['return/file.txt' => '{"return":"OK"}']
]
--- request eval
[
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt',
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    200,
    200,
    502
]
