#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include "stm32f4xx_hal.h"
#include "http_defs.h"

#define HTTP_SERVER_PORT    80
#define HTTP_MAX_HEADERS    20

struct HTTP_REQUEST
{
    size_t headers_count, method_size, route_size, body_size;
    const char *method, *route, *body;
    int version;
};

struct HTTP_RESPONSE
{
    size_t message_size, head_size;
    const char *message;
    uint16_t status;
    int version;
};

void http_get_form_field(char **field, size_t *field_size, const char *field_name, char *data, size_t data_size);
void http_build_html_response(
    char *buffer,
    const char **message,
    size_t *message_size,
    size_t *head_size,
    const char *route,
    size_t route_size
);
void http_build_text_response(
    char *buffer,
    const char *message,
    size_t *message_size,
    size_t *head_size,
    const char *route,
    size_t route_size,
    uint16_t code
);

#endif /* HTTP_HELPER_H */