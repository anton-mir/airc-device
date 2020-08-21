#include "picohttpparser.h"
#include "http_helper.h"
#include "esp8266_wifi.h"

#include "main.h"

void http_get_form_field(char **field, size_t *field_size, const char *field_name, char *data, size_t data_size)
{
    char *delimeter, *pos;
    if ((pos = strstr(data, field_name)) != NULL)
    {
        pos += strlen(field_name);
        *field = pos;
        if ((delimeter = strstr(pos, "&")) != NULL)
        {
            *field_size = (intptr_t)delimeter - (intptr_t)pos;
        }
        else
        {
            *field_size = ((intptr_t)data + data_size) - (intptr_t)pos;
        }
    }
}

void http_build_html_response(
    char *buffer,
    const char **message,
    size_t *message_size,
    size_t *head_size,
    const char *route,
    size_t route_size
)
{
    memcpy(buffer, "HTTP/1.1 ", 9);
    *head_size = 9;
    memcpy(buffer + *head_size, "200 OK\r\n", 8);
    *head_size += 8;
    memcpy(buffer + *head_size, "Content-Type: text/html\r\n", 25);
    *head_size += 25;
    memcpy(buffer + *head_size, "Content-Length: ", 16);
    *head_size += 16;

    *message = HTML_SETTINGS_PAGE;
    *message_size = strlen(*message);

    size_t message_size_str_length = NUMBER_LENGTH(*message_size);
    char message_size_str[message_size_str_length];
    sprintf(message_size_str, "%d", *message_size);

    memcpy(buffer + *head_size, message_size_str, message_size_str_length);
    *head_size += message_size_str_length;

    memcpy(buffer + *head_size, "\r\n\r\n", 4);
    *head_size += 4;
}

void http_build_text_response(
    char *buffer,
    const char *message,
    size_t *message_size,
    size_t *head_size,
    const char *route,
    size_t route_size,
    uint16_t code
)
{
    memcpy(buffer, "HTTP/1.1 ", 9);
    *head_size = 9;

    if (code == 200)
    {
        memcpy(buffer + *head_size, "200 OK\r\n", 8);
        *head_size += 8;
    }
    else if (code == 500)
    {
        memcpy(buffer + *head_size, "500 Internal Server Error\r\n", 27);
        *head_size += 27;
    }
    else if (code == 404)
    {
        memcpy(buffer + *head_size, "404 Not Found\r\n", 15);
        *head_size += 15;
    }
    else if (code == 405)
    {
        memcpy(buffer + *head_size, "405 Method Not Allowed\r\n", 24);
        *head_size += 24;
    }

    memcpy(buffer + *head_size, "Content-Type: text/plain\r\n", 26);
    *head_size += 26;
    memcpy(buffer + *head_size, "Content-Length: ", 16);
    *head_size += 16;

    *message_size = strlen(message);

    size_t message_size_str_length = NUMBER_LENGTH(*message_size);
    char message_size_str[message_size_str_length];
    sprintf(message_size_str, "%d", *message_size);

    memcpy(buffer + *head_size, message_size_str, message_size_str_length);
    *head_size += message_size_str_length;

    memcpy(buffer + *head_size, "\r\n\r\n", 4);
    *head_size += 4;
}