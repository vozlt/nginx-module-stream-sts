# vi:set ft=perl ts=4 sw=4 et fdm=marker:

use Test::Nginx::Socket;
use Fcntl;

add_response_body_check(
    sub {
        my ($block, $body, $req_idx, $repeated_req_idx, $dry_run) = @_;

        my $path = 't/servroot/logs/access.stream.log';
        my @lines = FH->getlines() if (sysopen(FH, $path, O_RDONLY));
        close(FH);
        my $ll = $lines[-1];

        print "$ll";

        if ($block->name =~ /TEST 4/) {
            ($req_idx == 4 && $ll !~ /status:100/) and
            bail_out "limit_traffic_by_set_key[UG] error($ll)";
        } else {
            ($req_idx == 3 && $ll !~ /status:100/) and
            bail_out "limit_traffic_by_set_key[FG] error($ll)";
        }
    }
);

plan tests => repeat_each() * blocks() * 4 + 1;
no_shuffle();
run_tests();

__DATA__

=== TEST 1: server_traffic_status_limit_traffic_by_set_key FG@group@name connect:n 100
--- main_config
    stream {
        server_traffic_status_zone;
        log_format basic '[$time_local] status:$status '
                         'sts_connect_counter:$sts_connect_counter '
                         'sts_in_bytes:$sts_in_bytes '
                         'sts_out_bytes:$sts_out_bytes '
                         'sts_session_time:$sts_session_time';
        access_log  logs/access.stream.log basic;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_filter_by_set_key $protocol protocol;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP connect:2 100;
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
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    502 
]



=== TEST 2: server_traffic_status_limit_traffic_by_set_key FG@group@name in:n 100
--- main_config
    stream {
        server_traffic_status_zone;
        log_format basic '[$time_local] status:$status '
                         'sts_connect_counter:$sts_connect_counter '
                         'sts_in_bytes:$sts_in_bytes '
                         'sts_out_bytes:$sts_out_bytes '
                         'sts_session_time:$sts_session_time';
        access_log  logs/access.stream.log basic;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_filter_by_set_key $protocol protocol;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP in:200 100;
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
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    502 
]



=== TEST 3: server_traffic_status_limit_traffic_by_set_key FG@group@name out:n 100
--- main_config
    stream {
        server_traffic_status_zone;
        log_format basic '[$time_local] status:$status '
                         'sts_connect_counter:$sts_connect_counter '
                         'sts_in_bytes:$sts_in_bytes '
                         'sts_out_bytes:$sts_out_bytes '
                         'sts_session_time:$sts_session_time';
        access_log  logs/access.stream.log basic;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_filter_by_set_key $protocol protocol;
            server_traffic_status_limit_traffic_by_set_key FG@protocol@TCP out:512 100;
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
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    502 
]



=== TEST 4: server_traffic_status_limit_traffic_by_set_key UG@group@name connect:n
--- main_config
    stream {
        server_traffic_status_zone;
        log_format basic '[$time_local] status:$status '
                         'sts_connect_counter:$sts_connect_counter '
                         'sts_in_bytes:$sts_in_bytes '
                         'sts_out_bytes:$sts_out_bytes '
                         'sts_session_time:$sts_session_time';
        access_log  logs/access.stream.log basic;
        upstream backend {
            server localhost:1984;
        }
        server {
            listen 1985;
            server_traffic_status_limit_traffic_by_set_key UG@backend@127.0.0.1:1984 connect:2 100;
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
    'GET /stream/file.txt'
]
--- error_code eval
[
    200,
    200,
    200,
    200,
    502 
]
