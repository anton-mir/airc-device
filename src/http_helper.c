#include <stdio.h>
#include <string.h>
#include "picohttpparser.h"
#include "http_helper.h"
#include "esp8266_wifi.h"

#include "main.h"

static const struct HTTP_ROUTE ALLOWED_ROUTES[] = {
    { HTTP_GET, "settings", ESP_PAGE_PIN_HOST_REQUIRED, HTML_SETTINGS, 35395, ESP_VOID_HANDLER },
    { HTTP_GET, "network", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_VOID_HANDLER },
    { HTTP_POST, "network", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_CONNECT_WIFI },
    { HTTP_GET, "networks", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_GET_WIFI_LIST },
    { HTTP_POST, "wifi-mode", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_WIFI_MODE },
    { HTTP_POST, "conf-mode", ESP_PAGE_HOST_REQUIRED, NULL, 0, ESP_CONF_MODE },
    { HTTP_GET, "device-conf", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_GET_DEVICE_CONF },
    { HTTP_POST, "device-conf", ESP_PAGE_PIN_HOST_REQUIRED, NULL, 0, ESP_SET_DEVICE_CONF }
};

static const struct HTTP_CONTENT_TYPE ALLOWED_CONTENT_TYPES[] = {
    { "text/html", HTTP_HTML },
    { "text/plain", HTTP_TEXT },
    { "text/css", HTTP_CSS },
    { "text/javascript", HTTP_JS }
};

static const struct HTTP_METHOD ALLOWED_METHODS[] = {
    { "GET", HTTP_GET },
    { "POST", HTTP_POST }
};

static int find_header(struct phr_header *headers, size_t headers_count, const char *header_name, const char *header_value);

void http_get_form_field(char **field, size_t *field_size, const char *field_name, const char *data, size_t data_size)
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

void http_build_response(char *buffer, struct HTTP_RESPONSE *response)
{
    response->head_size = sprintf(buffer, "HTTP/1.1 ");

    // Build status header
    switch (response->http_status)
    {
    case HTTP_200:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_200_TEXT "\n"));
        if (ALLOWED_ROUTES[response->route_index].data != NULL)
        {
            response->message = ALLOWED_ROUTES[response->route_index].data;
            response->message_size = ALLOWED_ROUTES[response->route_index].data_size;
        }
        esp_server_handler(ALLOWED_ROUTES[response->route_index].handler);
        break;
    case HTTP_401:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_401_TEXT "\n"));
        response->message = HTTP_401_TEXT;
        response->message_size = sizeof(HTTP_401_TEXT) - 1;
        break;
    case HTTP_404:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_404_TEXT "\n"));
        response->message = HTTP_404_TEXT;
        response->message_size = sizeof(HTTP_404_TEXT) - 1;
        break;
    case HTTP_405:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_405_TEXT "\n"));
        response->message = HTTP_405_TEXT;
        response->message_size = sizeof(HTTP_405_TEXT) - 1;
        break;
    case HTTP_415:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_415_TEXT "\n"));
        response->message = HTTP_415_TEXT;
        response->message_size = sizeof(HTTP_415_TEXT) - 1;
        break;
    
    default:
        response->head_size += sprintf(buffer + response->head_size, (HTTP_500_TEXT "\n"));
        response->message = HTTP_500_TEXT;
        response->message_size = sizeof(HTTP_500_TEXT) - 1;
        break;
    }

    // Build Content-Type header
    switch (response->http_content_type)
    {
    case HTTP_HTML:
        response->head_size += sprintf(buffer + response->head_size, ("Content-Type: " HTTP_HTML_TYPE_TEXT "\n"));
        break;
    case HTTP_CSS:
        response->head_size += sprintf(buffer + response->head_size, ("Content-Type: " HTTP_CSS_TYPE_TEXT "\n"));
        break;
    case HTTP_JS:
        response->head_size += sprintf(buffer + response->head_size, ("Content-Type: " HTTP_JS_TYPE_TEXT "\n"));
        break;
    
    default:
        response->head_size += sprintf(buffer + response->head_size, ("Content-Type: " HTTP_TEXT_TYPE_TEXT "\n"));
        break;
    }

    // Build Content-Length header
    response->head_size += sprintf(buffer + response->head_size, "Content-Length: %u\n", response->message_size);
    
    // End HTTP head
    response->head_size += sprintf(buffer + response->head_size, "\n");
}

void http_check_method(struct HTTP_RESPONSE *http_response, const char *method, size_t method_size)
{
    size_t methods_count = sizeof(ALLOWED_METHODS) / sizeof(ALLOWED_METHODS[0]);
    for (size_t method_i = 0; method_i < methods_count; method_i++)
    {
        if (memcmp(method, ALLOWED_METHODS[method_i].name, method_size) == 0)
        {
            http_response->http_method = ALLOWED_METHODS[method_i].method;
            return;
        }
    }

    http_response->http_method = HTTP_NOT_ALLOWED;
}

void http_check_route(struct phr_header *headers, size_t headers_count, struct HTTP_RESPONSE *http_response, const char *route, size_t route_size, int mode)
{
    size_t routes_count = sizeof(ALLOWED_ROUTES) / sizeof(ALLOWED_ROUTES[0]);
    for (size_t route_i = 0; route_i < routes_count; route_i++)
    {
        size_t route_length = strlen(ALLOWED_ROUTES[route_i].name);
        if (route_length < (route_size - 1)) route_length = route_size - 1;
        if (memcmp(route + 1, ALLOWED_ROUTES[route_i].name, route_length) == 0)
        {
            http_response->route_index = route_i;
            if (http_response->http_method == ALLOWED_ROUTES[route_i].method)
            {
                switch (ALLOWED_ROUTES[route_i].access)
                {
                case ESP_PAGE_PIN_REQUIRED:
                    if (mode == 1) http_response->availible = 1;
                    else http_response->availible = 0;
                    break;
                case ESP_PAGE_HOST_REQUIRED:
                    if (find_header(headers, headers_count, "Host", ESP_SERVER_HOST) >= 0) http_response->availible = 1;
                    else http_response->availible = 0;
                    break;
                case ESP_PAGE_PIN_HOST_REQUIRED:
                    if (mode == 1 && find_header(headers, headers_count, "Host", ESP_SERVER_HOST) >= 0) http_response->availible = 1;
                    else http_response->availible = 0;
                    break;
                case ESP_PAGE_OPEN:
                    http_response->availible = 1;
                    break;
                
                default:
                    http_response->availible = 0;
                    break;
                }
                return;
            }
            else continue;
        }
    }
    http_response->route_index = -1;
}

void http_check_content_type(struct HTTP_RESPONSE *http_response, struct phr_header *headers, size_t headers_count)
{
    size_t types_count = sizeof(ALLOWED_CONTENT_TYPES) / sizeof(ALLOWED_CONTENT_TYPES[0]);
    for (size_t type_i = 0; type_i < types_count; type_i++)
    {
        if (find_header(headers, headers_count, "Accept", ALLOWED_CONTENT_TYPES[type_i].name) >= 0)
        {
            http_response->http_content_type = ALLOWED_CONTENT_TYPES[type_i].content_type;
            return;
        }
    }

    http_response->http_content_type = HTTP_NOT_ALLOWED;
}

static int find_header(struct phr_header *headers, size_t headers_count, const char *header_name, const char *header_value)
{
    for (size_t header_i = 0; header_i < headers_count; header_i++)
    {
        if (memcmp(headers[header_i].name, header_name, headers[header_i].name_len) == 0)
        {
            if (strstr(headers[header_i].value, header_value) != NULL) return header_i;
            else return -1;
        }
    }
    return -1;
}

void http_request_clear(struct HTTP_REQUEST *http_request)
{
    http_request->body = NULL;
    http_request->body_size = 0;
    http_request->method = NULL;
    http_request->method_size = 0;
    http_request->route = NULL;
    http_request->route_size = 0;
    http_request->version = 0;
    http_request->headers_count = 0;
}

void http_response_clear(struct HTTP_RESPONSE *http_response)
{
    http_response->head_size = 0;
    http_response->message = NULL;
    http_response->message_size = 0;
    http_response->http_status = HTTP_NOT_ALLOWED;
    http_response->http_content_type = HTTP_NOT_ALLOWED;
    http_response->http_method = HTTP_NOT_ALLOWED;
    http_response->route_index = -1;
    http_response->version = 0;
    http_response->availible = 0;
    http_response->ready = 0;
}