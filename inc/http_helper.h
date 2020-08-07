#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include "stm32f4xx_hal.h"
#include "http_defs.h"

#define HTTP_SERVER_PORT 11333
#define HTTP_MAX_HEADERS 20

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

typedef struct HTTP_REQUEST
{
    HTTP_REQUEST_DATA_TYPE data_type;
    HTTP_METHOD method;
    const char *path;
    size_t path_length;
    const char *body;
    size_t body_length;
    uint16_t status;
} HTTP_REQUEST;

typedef struct HTTP_RESPONSE
{
    uint8_t id;
    uint16_t code;
    HTTP_CONTENT_TYPE content_type;
    size_t content_size;
    HTML_PAGE body_template;
    const char *body;
    const char *content_type_header;
    const char *content_length_header;
    const char *http;
} HTTP_RESPONSE;

uint16_t http_parse_request(HTTP_REQUEST *request, uint8_t *tcp_data, size_t tcp_length);

#endif /* HTTP_HELPER_H */