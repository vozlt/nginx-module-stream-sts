Nginx stream server traffic status core module
==========

[![License](http://img.shields.io/badge/license-BSD-brightgreen.svg)](https://github.com/vozlt/nginx-module-stream-sts/blob/master/LICENSE)

Nginx stream server traffic status core module

Table of Contents
=================

* [Version](#version)
* [Dependencies](#dependencies)
* [Screenshots](#screenshots)
* [Installation](#installation)
* [Synopsis](#synopsis)
* [Description](#description)
* [Variables](https://github.com/vozlt/nginx-module-sts#variables)
* [Directives](#directives)
  * [server_traffic_status](https://github.com/vozlt/nginx-module-sts#server_traffic_status)
  * [server_traffic_status_zone](https://github.com/vozlt/nginx-module-sts#server_traffic_status_zone)
  * [server_traffic_status_filter](https://github.com/vozlt/nginx-module-sts#server_traffic_status_filter)
  * [server_traffic_status_filter_by_set_key](https://github.com/vozlt/nginx-module-sts#server_traffic_status_filter_by_set_key)
  * [server_traffic_status_filter_check_duplicate](https://github.com/vozlt/nginx-module-sts#server_traffic_status_filter_check_duplicate)
  * [server_traffic_status_limit](https://github.com/vozlt/nginx-module-sts#server_traffic_status_limit)
  * [server_traffic_status_limit_traffic](https://github.com/vozlt/nginx-module-sts#server_traffic_status_limit_traffic)
  * [server_traffic_status_limit_traffic_by_set_key](https://github.com/vozlt/nginx-module-sts#server_traffic_status_limit_traffic_by_set_key)
  * [server_traffic_status_limit_check_duplicate](https://github.com/vozlt/nginx-module-sts#server_traffic_status_limit_check_duplicate)
  * [server_traffic_status_average_method](https://github.com/vozlt/nginx-module-sts#server_traffic_status_average_method)
  * [server_traffic_status_histogram_buckets](https://github.com/vozlt/nginx-module-sts#server_traffic_status_histogram_buckets)
* [See Also](#see-also)
* [TODO](#todo)
* [Donation](#donation)
* [Author](#author)

## Version
This document describes nginx-module-stream-sts `v0.1.1` released on 04 Feb 2018.

## Dependencies
* [nginx](http://nginx.org)
* [nginx-module-sts](https://github.com/vozlt/nginx-module-sts)

## Compatibility
* Nginx
  * 1.11.5 \<= (last tested: 1.15.0)

Earlier versions does not work.

## Screenshots
![nginx-module-sts screenshot](https://cloud.githubusercontent.com/assets/3648408/23112117/e8c56cda-f770-11e6-9c68-f57cbf4dd542.png "screenshot with deault")

## Installation

1. Clone the git repository.

  ```
  shell> git clone git://github.com/vozlt/nginx-module-sts.git
  ```
  ```
  shell> git clone git://github.com/vozlt/nginx-module-stream-sts.git
  ```

2. Add the module to the build configuration by adding
  ```
  --with-stream
  --add-module=/path/to/nginx-module-sts
  --add-module=/path/to/nginx-module-stream-sts
  ```

3. Build the nginx binary.

4. Install the nginx binary.

## Synopsis

```Nginx
http {
    stream_server_traffic_status_zone;

    ...

    server {

        ...

        location /status {
            stream_server_traffic_status_display;
            stream_server_traffic_status_display_format html;
        }
    }
}

stream {
    server_traffic_status_zone;

    ...

    server {
        ...
    }
}
```

## Description
This is an Nginx module that provides access to stream server traffic status information.
This is a porting version of the [nginx-module-vts](https://github.com/vozlt/nginx-module-vts) to the NGINX "stream" subsystem so as to support the same features in [nginx-module-vts](https://github.com/vozlt/nginx-module-vts).
It contains the current status such as servers, upstreams, user-defined filter.
This module is the core module of two modules([nginx-module-sts](https://github.com/vozlt/nginx-module-sts), [nginx-module-stream-sts](https://github.com/vozlt/nginx-module-stream-sts)).

The functions of each module are as follows:

* [nginx-module-stream-sts](https://github.com/vozlt/nginx-module-stream-sts)
  * Support for implementing stream server stats.
  * Support for implementing stream filter.
  * Support for implementing stream limit.
  * Support for implementing stream embedded variables.
* [nginx-module-sts](https://github.com/vozlt/nginx-module-sts)
  * Support for implementing display of stream server stats.
  * Support for implementing control of stream server stats.

## See Also
* [nginx-module-sts](https://github.com/vozlt/nginx-module-sts)
* [nginx-module-vts](https://github.com/vozlt/nginx-module-vts)

## TODO

## Donation
[![License](http://img.shields.io/badge/PAYPAL-DONATE-yellow.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=PWWSYKQ9VKH38&lc=KR&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHosted)

## Author
YoungJoo.Kim(김영주) [<vozltx@gmail.com>]
