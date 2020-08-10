#include "picohttpparser.h"
#include "http_helper.h"
#include "esp8266_wifi.h"

#include "main.h"

const char * const HTTP_ROUTES[] = {
    "/",
    "/settings"
};
const size_t HTTP_ROUTES_COUNT = 2;

const char * const HTTP_METHODS[] = {
    "GET",
    "POST",
    "PUT",
    "DELETE"
};
const size_t HTTP_METHODS_COUNT = 4;

int http_validate_route(const char *route, size_t route_size)
{
    for (size_t i = 0; i < HTTP_ROUTES_COUNT; i++)
    {
        size_t size = strlen(HTTP_ROUTES[i]);
        if (size >= route_size)
            if (memcmp(route, HTTP_ROUTES[i], size) == 0)
                return 1;
        else
            if (memcmp(route, HTTP_ROUTES[i], route_size) == 0)
                return 1;
    }
    
    return 0;
}

int http_validate_method(const char *method, size_t method_size)
{
    for (size_t i = 0; i < HTTP_METHODS_COUNT; i++)
    {
        size_t size = strlen(HTTP_METHODS[i]);
        if (size >= method_size)
            if (memcmp(method, HTTP_METHODS[i], size) == 0)
                return 1;
        else
            if (memcmp(method, HTTP_METHODS[i], method_size) == 0)
                return 1;
    }

    return 0;
}

void http_build_error_response(
    char *buffer,
    const char **message,
    size_t *message_size,
    size_t *head_size,
    uint16_t status
)
{
    memcpy(buffer, "HTTP/1.1 ", 9);
    *head_size = 9;

    if (status == 404)
    {
        *message = HTML_404_PAGE;
        memcpy(buffer + *head_size, "404 Not Found\n", 14);
        *head_size += 14;
    }
    else if (status == 405)
    {
        *message = HTML_405_PAGE;
        memcpy(buffer + *head_size, "405 Method Not Allowed\n", 23);
        *head_size += 23;
    }
    else if (status == 505)
    {
        *message = HTML_505_PAGE;
        memcpy(buffer + *head_size, "505 HTTP Version Not Supported\n", 31);
        *head_size += 31;
    }
    else
    {
        *message = HTML_500_PAGE;
        memcpy(buffer + *head_size, "500 Internal Server Error\n", 26);
        *head_size += 26;
    }
    *message_size = strlen(*message);

    memcpy(buffer + *head_size, "Content-Type: text/html\n", 24);
    *head_size += 24;
    memcpy(buffer + *head_size, "Content-Length: ", 16);
    *head_size += 16;

    size_t message_size_str_length = NUMBER_LENGTH(*message_size);
    char message_size_str[message_size_str_length];
    sprintf(message_size_str, "%d", *message_size);

    memcpy(buffer + *head_size, message_size_str, message_size_str_length);
    *head_size += message_size_str_length;

    memcpy(buffer + *head_size, "\n\n", 2);
    *head_size += 2;
}