#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include "stm32f4xx_hal.h"
#include "http_defs.h"

#define HTTP_SERVER_PORT 11333
#define HTTP_MAX_HEADERS 20
#define HTTP_MAX_SIZE    0xFFF

typedef enum HTTP_METHODS
{
    GET,
    POST,
    PUT,
    DELETE
} HTTP_METHOD;

typedef enum HTTP_CONTENT_TYPES
{
    TEXT,
    HTML,
    JSON,
    CSS,
    JS
} HTTP_CONTENT_TYPE;

typedef enum HTTP_REQUEST_DATA_TYPES
{
    JSON_DATA,
    FORM_DATA,
    TEXT_DATA
} HTTP_REQUEST_DATA_TYPE;

struct HTTP_REQUEST
{
    size_t headers_count, method_size, route_size;
    const char *method, *route;
    int version;
};

struct HTTP_RESPONSE
{
    size_t message_size, head_size;
    const char *message;
    uint16_t status;
    int version;
};

int http_validate_route(const char *route, size_t route_size);
int http_validate_method(const char *method, size_t method_size);
void http_build_error_response(
    char *buffer,
    const char **message,
    size_t *message_size,
    size_t *head_size,
    uint16_t status
);
void http_build_html_response(
    char *buffer,
    const char **message,
    size_t *message_size,
    size_t *head_size,
    const char *route
);

#endif /* HTTP_HELPER_H */