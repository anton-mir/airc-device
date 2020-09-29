#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include "stm32f4xx_hal.h"
#include "picohttpparser.h"
#include "esp8266_wifi.h"
#include "http_defs.h"

#define HTTP_SERVER_PORT    80
#define HTTP_MAX_HEADERS    20
#define HTTP_NOT_ALLOWED    0

typedef enum HTTP_STATUSES
{
    HTTP_200 = 200,
    HTTP_401 = 401,
    HTTP_404 = 404,
    HTTP_405 = 405,
    HTTP_415 = 415,
    HTTP_500 = 500
} HTTP_STATUS;

typedef enum HTTP_METHODS
{
    HTTP_GET = 1,
    HTTP_POST
} HTTP_METHOD;

typedef enum HTTP_CONTENT_TYPES
{
    HTTP_HTML = 1,
    HTTP_TEXT,
    HTTP_CSS,
    HTTP_JS,
    HTTP_WEBP
} HTTP_CONTENT_TYPE;

struct HTTP_METHOD
{
    const char *name;
    HTTP_METHOD method;
};

struct HTTP_CONTENT_TYPE
{
    const char *name;
    HTTP_CONTENT_TYPE content_type;
};

struct HTTP_ROUTE
{
    HTTP_METHOD method;
    const char *name;
    ESP8266_SERVER_PAGE_ACCESS access;
    char *data;
    size_t data_size;
    ESP8266_SERVER_HANDLER handler;
};

struct HTTP_REQUEST
{
    size_t headers_count, method_size, route_size, body_size;
    const char *method, *route, *body;
    int version;
};

struct HTTP_RESPONSE
{
    size_t message_size, head_size;
    char *message;
    HTTP_METHOD http_method;
    HTTP_STATUS http_status;
    HTTP_CONTENT_TYPE http_content_type;
    int version, route_index, availible, ready;
};

struct HTTP_FORM_VALUE
{
    char *value;
    size_t size;
};


void http_build_routes(void);

void http_get_form_field(char **field, size_t *field_size, const char *field_name, const char *data, size_t data_size);
void http_build_response(char *buffer, struct HTTP_RESPONSE *response);
void http_check_method(struct HTTP_RESPONSE *response, const char *method, size_t method_size);
void http_check_route(struct phr_header *headers, size_t headers_count, struct HTTP_RESPONSE *response, const char *route, size_t route_size, int mode);
void http_check_content_type(struct HTTP_RESPONSE *response, struct phr_header *headers, size_t headers_count);
void http_request_clear(struct HTTP_REQUEST *request);
void http_response_clear(struct HTTP_RESPONSE *response);

#endif /* HTTP_HELPER_H */