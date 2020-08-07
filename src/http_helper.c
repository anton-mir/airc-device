#include "picohttpparser.h"
#include "http_helper.h"

#include "main.h"

uint16_t http_parse_request(HTTP_REQUEST *request, uint8_t *tcp_data, size_t tcp_length)
{
    struct phr_header headers[HTTP_MAX_HEADERS];
    size_t headers_count = HTTP_MAX_HEADERS;
    char *method;
    size_t method_length;
    int version;
    int res = phr_parse_request((char *)tcp_data, tcp_length, 
                                &method, &method_length, 
                                &request->path, &request->path_length, &version, 
                                &headers, &headers_count);

    if (res < 0) return 500;

    return 200;
}

/*HTTP_RESPONSE build_http_response(HTTP_REQUEST request, uint16_t code, uint8_t id)
{
    HTTP_RESPONSE response = { 0 };
    response.id = id;
    response.code = code;

    if (code == 200 && request.data_type == TEXT_DATA)
    {
        if (memcmp(request.path, "/", request.path_length) == 0)
        {
            response.body_template = HTML_HOME;
            response.content_type = HTML;
        }
        else if (memcmp(request.path, "/settings", request.path_length) == 0)
        {
            response.body_template = HTML_SETTINGS;
            response.content_type = HTML;
        } 
        else 
        {
            response.body_template = HTML_404;
            response.content_type = HTML;
            response.code = 404;
        }
    }
    else if (code == 401)
    {
        response.body_template = HTML_401;
        response.content_type = HTML;
    }
    else if (code == 404)
    {
        response.body_template = HTML_404;
        response.content_type = HTML;
    }
    else if (code == 500)
    {
        response.body_template = HTML_500;
        response.content_type = HTML;
    }
    
    return response;
}

void format_http_response(HTTP_RESPONSE *response)
{
    if (response->body_template == HTML_HOME)
        response->body = "HOME";
    else if (response->body_template == HTML_SETTINGS)
        response->body = "SETTINGS";
    else if (response->body_template == HTML_401)
        response->body = "401";
    else if (response->body_template == HTML_404)
        response->body = "404";
    else if (response->body_template == HTML_500)
        response->body = "500";

    response->content_size = strlen(response->body);

    response->content_length_header = "Content-Length: ";

    if (response->content_type == TEXT)
        response->content_type_header = "Content-Type: text/plain";
    else if (response->content_type == HTML)
        response->content_type_header = "Content-Type: text/html";
    else if (response->content_type == JSON)
        response->content_type_header = "Content-Type: application/json";
    else if (response->content_type == JS)
        response->content_type_header = "Content-Type: application/javascript";
    else if (response->content_type == CSS)
        response->content_type_header = "Content-Type: text/css";

    if (response->code == 200)
        response->http = "HTTP/1.1 200 OK";
    if (response->code == 401)
        response->http = "HTTP/1.1 401 Unauthorized";
    if (response->code == 404)
        response->http = "HTTP/1.1 404 Not Found";
    if (response->code == 500)
        response->http = "HTTP/1.1 500 Internal Server Error";
}*/