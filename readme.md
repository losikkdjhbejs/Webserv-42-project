# Webserv

A lightweight HTTP/1.1 web server implemented in **C++98** using low-level POSIX networking and system calls.

The goal of this project is to understand how a real web server works internally: accepting TCP connections, parsing HTTP requests, routing requests according to configuration, serving files, handling directories, executing CGI programs, creating HTTP responses, and managing client connections.

---

## Features

The server currently supports:

* HTTP/1.0 and HTTP/1.1
* Non-blocking TCP sockets
* `epoll`
* HTTP request parsing
* HTTP response serialization
* Virtual hosts using the `Host` header
* Location-based routing
* Longest-prefix location matching
* Static file serving
* MIME type detection
* Directory index files
* Directory autoindex
* HTTP redirects
* `GET`
* `POST`
* `DELETE`
* CGI execution
* CGI request bodies through `stdin`
* CGI response headers
* Custom error pages
* HTTP `Content-Length`
* Keep-alive support
* Binary-safe request and response bodies

---

# Architecture

The server is divided into several components, each with a specific responsibility.

```text
                         CLIENT
                            |
                            | TCP
                            v
                    +---------------+
                    |    Server     |
                    |   sockets     |
                    |     epoll     |
                    +-------+-------+
                            |
                            v
                    +---------------+
                    |  Connection   |
                    | request data  |
                    | write buffer  |
                    +-------+-------+
                            |
                            v
                    +---------------+
                    |  HttpRequest  |
                    | HTTP parsing  |
                    +-------+-------+
                            |
                            v
                    +---------------+
                    |    Router     |
                    | server select |
                    | location      |
                    | path mapping  |
                    | CGI detection |
                    +-------+-------+
                            |
                            v
                    +---------------+
                    |RequestHandler |
                    +-------+-------+
                            |
             +--------------+--------------+
             |              |              |
             v              v              v
            GET            POST          DELETE
             |
       +-----+-----+
       |           |
       v           v
    Static      Directory
     File          |
                  +------> index
                  |
                  +------> autoindex
             |
             v
            CGI
             |
             v
       fork + pipes
             |
             v
       CGI executable
             |
             v
       HttpResponse
             |
             v
         serialize()
             |
             v
          write()
             |
             v
           CLIENT
```

---

# Main Components

## `Server`

Responsible for the networking layer.

Its responsibilities include:

* Creating the listening socket
* Binding the socket
* Listening for connections
* Accepting clients
* Making client sockets non-blocking
* Registering file descriptors with `epoll`
* Detecting readable/writable events
* Reading request data
* Sending response data
* Managing client connections

The server is therefore the connection between the operating system's networking API and the HTTP layer.

---

## `Connection`

A `Connection` represents the state of one client connection.

It stores information such as:

```text
client file descriptor
HTTP request
response/write buffer
write offset
```

This is particularly important with non-blocking sockets because one `write()` call is not guaranteed to send the entire response.

The server therefore keeps:

```text
response buffer
+
current write offset
```

and continues writing when the socket becomes writable.

---

# HTTP Request

`HttpRequest` represents a parsed HTTP request.

Conceptually:

```text
HTTP Request
    |
    +-- Method
    |
    +-- Path
    |
    +-- HTTP Version
    |
    +-- Headers
    |
    +-- Body
```

The request provides header access through:

```cpp
bool hasHeader(const std::string& key) const;

const std::string& getHeader(const std::string& key) const;

const std::map<std::string, std::string>& getHeaders() const;

bool keepAlive() const;
```

### `hasHeader()`

Checks whether a header exists.

Example:

```cpp
if (request.hasHeader("Host"))
{
    std::string host = request.getHeader("Host");
}
```

### `getHeader()`

Returns the value of a specific header.

If the requested header does not exist, the implementation throws an exception.

### `getHeaders()`

Returns read-only access to the complete header map.

### `keepAlive()`

Determines whether the connection should remain open according to the HTTP version and connection headers.

This is important because HTTP/1.1 normally uses persistent connections.

---

# HTTP Response

`HttpResponse` represents the response that will be sent to the client.

Its main interface is:

```cpp
HttpResponse();

void setVersion(HttpRequest::Version version);

void setStatus(int code);

void setHeader(
    const std::string& key,
    const std::string& value);

bool hasHeader(const std::string& key) const;

const std::string& getHeader(
    const std::string& key) const;

void setBody(const std::vector<char>& body);

const std::vector<char>& getBody() const;

std::vector<char> serialize() const;
```

---

# Response Structure

An HTTP response has four main parts:

```text
Status Line
Headers
Empty Line
Body
```

Example:

```text
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 18

<html>Hello</html>
```

`HttpResponse::serialize()` converts the internal response representation into the actual bytes that are sent through the socket.

---

# Supported Status Codes

The current implementation defines reason phrases for:

| Code | Meaning               |
| ---: | --------------------- |
|  200 | OK                    |
|  201 | Created               |
|  204 | No Content            |
|  301 | Moved Permanently     |
|  302 | Found                 |
|  400 | Bad Request           |
|  403 | Forbidden             |
|  404 | Not Found             |
|  405 | Method Not Allowed    |
|  413 | Payload Too Large     |
|  500 | Internal Server Error |

Unknown status codes use:

```text
Unknown Status
```

---

# Content-Length

When serializing a response, the server automatically adds:

```text
Content-Length
```

if the application did not already specify it.

The value is calculated from:

```cpp
body.size()
```

This tells the client how many bytes belong to the response body.

---

# Binary-Safe Bodies

HTTP bodies are stored as:

```cpp
std::vector<char>
```

instead of relying exclusively on C-style strings.

This is important because HTTP bodies are not necessarily text.

For example, the server can return:

```text
HTML
CSS
JavaScript
PNG
JPEG
GIF
PDF
```

without treating binary data as null-terminated strings.

---

# Router

The `Router` converts an HTTP request into a `RouteResult`.

```cpp
RouteResult route(const HttpRequest& request);
```

The router is responsible for determining:

```text
Which server?
Which location?
Which filesystem path?
Is this CGI?
Is autoindex enabled?
Which CGI executable?
```

The router does **not** actually serve the resource.

It only determines what should happen.

This separation keeps routing logic independent from request-handling logic.

---

# Virtual Host Selection

The router uses the HTTP:

```text
Host
```

header to select the appropriate configured server.

For example:

```text
Host: example.com:8080
```

The router removes the port:

```text
example.com
```

and compares the hostname against configured:

```text
server_name
```

values.

If no matching server is found, the first configured server is used as the fallback.

---

# Location Matching

The router supports location matching based on the request path.

For example:

```text
location /
location /api
location /api/users
```

For:

```text
/api/users/42
```

the most specific matching location is:

```text
/api/users
```

The implementation therefore chooses the **longest matching location path**.

---

## Prefix Boundary

The implementation also prevents incorrect prefix matches.

For example:

```text
location /cgi-bin
```

matches:

```text
/cgi-bin/test.py
```

but does **not** match:

```text
/cgi-binary
```

This is important because simple string-prefix matching would incorrectly consider `/cgi-binary` a match.

---

# RouteResult

After routing, the router returns a `RouteResult`.

```cpp
class RouteResult
{
public:
    const Server* server;
    const Location* location;

    std::string fileSystemPath;

    bool isCGI;
    bool autoindex;

    std::string cgiExecutable;

    RouteResult();
};
```

The important fields are:

| Field            | Purpose                              |
| ---------------- | ------------------------------------ |
| `server`         | Selected virtual server              |
| `location`       | Selected location                    |
| `fileSystemPath` | URI converted to filesystem path     |
| `isCGI`          | Whether CGI should be executed       |
| `autoindex`      | Whether directory listing is enabled |
| `cgiExecutable`  | CGI interpreter/executable           |

---

# Filesystem Mapping

The router converts a URI into a filesystem path using the configured `root`.

For example:

```text
root ./www
```

and:

```text
GET /index.html
```

becomes:

```text
./www/index.html
```

A location-specific `root` takes precedence over the server-level `root`.

---

# RequestHandler

`RequestHandler` contains the actual HTTP application logic.

The main dispatcher is:

```cpp
HttpResponse handle(
    const HttpRequest& request,
    const RouteResult& routeResult);
```

The dispatch process is:

```text
GET     -> handleGet()
POST    -> handlePost()
DELETE  -> handleDelete()
other   -> 405
```

This keeps each HTTP method's implementation separate.

---

# GET

The GET request flow is approximately:

```text
GET
 |
 +--> Redirect?
 |
 +--> CGI?
 |
 +--> Resource exists?
 |
 +--> Regular file?
 |       |
 |       +--> serveFile()
 |
 +--> Directory?
         |
         +--> index file?
         |
         +--> autoindex?
         |
         +--> 403
```

---

# Static File Serving

Regular files are handled by:

```cpp
serveFile()
```

The file is opened in binary mode and loaded into:

```cpp
std::vector<char>
```

The server then determines the MIME type and sets:

```text
Content-Type
```

before returning the response.

---

# MIME Types

`MimeTypes` maps file extensions to MIME types.

Currently supported examples include:

| Extension | MIME type                |
| --------- | ------------------------ |
| `.html`   | `text/html`              |
| `.htm`    | `text/html`              |
| `.css`    | `text/css`               |
| `.js`     | `application/javascript` |
| `.json`   | `application/json`       |
| `.png`    | `image/png`              |
| `.jpg`    | `image/jpeg`             |
| `.jpeg`   | `image/jpeg`             |
| `.gif`    | `image/gif`              |
| `.svg`    | `image/svg+xml`          |
| `.ico`    | `image/x-icon`           |
| `.txt`    | `text/plain`             |
| `.pdf`    | `application/pdf`        |
| `.py`     | `text/x-python`          |

Unknown extensions return:

```text
application/octet-stream
```

---

# Directory Handling

If a GET request points to a directory, the server first searches for configured index files.

For example:

```nginx
location / {
    root ./www;
    index index.html;
}
```

For:

```text
GET /
```

the server checks:

```text
./www/index.html
```

If it exists, it is served.

---

# Autoindex

If no index file exists and:

```text
autoindex on
```

is configured, the server generates an HTML directory listing.

Example:

```nginx
location /files {
    root ./www;
    autoindex on;
}
```

The generated page contains links to files and directories.

If no index exists and autoindex is disabled:

```text
403 Forbidden
```

is returned.

---

# Redirects

The server supports the configured `return` directive.

For example:

```nginx
location /old {
    return /new;
}
```

produces:

```text
HTTP/1.1 302 Found
Location: /new
```

The response also contains a small redirect body.

---

# POST

POST requests are handled by:

```cpp
handlePost()
```

There are two main cases.

## POST to CGI

If the selected route is CGI:

```text
POST
 |
 v
CGI
```

The request body is passed to the CGI process through:

```text
stdin
```

## POST to a file

For a normal route, the request body is written to the target file.

The implementation:

1. Checks whether the resource already exists.
2. Rejects existing non-regular resources.
3. Opens the file.
4. Writes the request body.
5. Returns `201 Created`.

---

# DELETE

DELETE is implemented by:

```cpp
handleDelete()
```

The processing is:

```text
DELETE
 |
 +--> Does resource exist?
 |
 +--> Is it a regular file?
 |
 +--> unlink()
 |
 +--> 204 No Content
```

Directories are not deleted.

A successful deletion returns:

```text
204 No Content
```

---

# CGI

CGI allows the web server to execute external programs.

The current implementation uses:

```text
pipe()
fork()
dup2()
execv()
waitpid()
```

The architecture is:

```text
                 Web Server
                     |
              +------+------+
              |             |
         input pipe     output pipe
              |             ^
              v             |
          CGI stdin      CGI stdout
              |             |
              +----- CGI ---+
                     |
                  response
```

---

# CGI Execution

The server creates two pipes:

```text
inputPipe
outputPipe
```

The child process redirects:

```text
stdin  -> inputPipe
stdout -> outputPipe
```

using:

```cpp
dup2()
```

The child then executes the configured CGI executable using:

```cpp
execv()
```

For example:

```text
/usr/bin/python3
```

can execute:

```text
./www/cgi-bin/test.py
```

---

# CGI POST

For POST requests, the parent writes:

```text
HTTP request body
```

to the CGI input pipe.

The CGI program receives it through:

```text
stdin
```

This allows CGI programs to process submitted data.

---

# CGI Response Parsing

CGI output can contain HTTP-style headers.

Example:

```text
Content-Type: text/plain

Hello from CGI!
```

The server searches for:

```text
\r\n\r\n
```

and also supports:

```text
\n\n
```

The part before the separator is treated as CGI headers.

The part after it becomes the response body.

For example:

```text
Content-Type: text/html
X-Test: hello

<html>Hello</html>
```

becomes approximately:

```text
HttpResponse
    |
    +-- Status: 200
    |
    +-- Content-Type: text/html
    |
    +-- X-Test: hello
    |
    +-- Body: <html>Hello</html>
```

---

# CGI Exit Status

After the CGI process finishes, the parent calls:

```cpp
waitpid()
```

If the CGI exits with a non-zero exit code:

```text
500 Internal Server Error
```

is returned.

If the process is terminated by a signal, the server also returns:

```text
500 Internal Server Error
```

This prevents failed CGI programs from being interpreted as successful requests.

---

# Error Handling

Errors are centralized through:

```cpp
handleError()
```

The server can use configured custom error pages.

Example:

```nginx
error_page 404 /errors/404.html;
```

When a 404 occurs, the server attempts to find:

```text
./www/errors/404.html
```

and serves it while preserving the original:

```text
404 Not Found
```

status.

If no custom error page is available, a basic fallback response is generated.

---

# Configuration Features

The current implementation uses configuration concepts such as:

```text
server
server_name
root
location
index
autoindex
allow_methods
cgi_ext
cgi_path
return
error_page
```

Example:

```nginx
server {
    root ./www;

    server_name localhost;

    location / {
        root ./www;
        index index.html;
        autoindex off;
        allow_methods GET POST DELETE;
    }

    location /cgi-bin {
        root ./www;
        cgi_ext .py .sh;
        cgi_path /usr/bin/python3 /bin/bash;
        index handler.py;
        allow_methods GET POST DELETE;
    }

    error_page 404 /errors/404.html;
}
```

The exact syntax depends on the project's configuration parser.

---

# Complete Request Lifecycle

A request goes through the following pipeline:

```text
1. Client connects
        |
2. Server accepts connection
        |
3. Client socket becomes readable
        |
4. Server reads bytes
        |
5. Connection stores request data
        |
6. HttpRequest parses the request
        |
7. Request becomes complete
        |
8. Router::route()
        |
9. Select virtual server
        |
10. Select location
        |
11. Build filesystem path
        |
12. Detect CGI / autoindex
        |
13. RequestHandler::handle()
        |
        +---- GET
        |
        +---- POST
        |
        +---- DELETE
        |
14. HttpResponse created
        |
15. HttpResponse::serialize()
        |
16. Response stored in Connection
        |
17. Socket becomes writable
        |
18. Server writes response
        |
19. Keep connection alive or close
```

---

# Important Design Decisions

## Separation of responsibilities

The project intentionally separates:

```text
Networking
HTTP parsing
Routing
Request handling
Response construction
MIME detection
Configuration
```

For example:

```text
Router
```

decides where a request goes.

```text
RequestHandler
```

decides what to do with it.

```text
HttpResponse
```

constructs the HTTP response.

This makes the architecture easier to understand and maintain.

---

## Binary-safe data

Using:

```cpp
std::vector<char>
```

for HTTP bodies allows the server to handle binary files correctly.

---

## Centralized response serialization

Handlers do not manually construct:

```text
HTTP/1.1 200 OK
Content-Type: ...
Content-Length: ...
```

Instead, they configure an `HttpResponse`, and:

```cpp
serialize()
```

creates the final wire-format response.

---

## Centralized error handling

Instead of implementing custom error behavior inside every handler, errors are passed through:

```cpp
handleError()
```

This provides one place to implement:

```text
status code
custom error page
fallback body
```

---

# Testing Checklist

A good Webserv test suite should cover the following.

## GET

```text
GET /
GET /index.html
GET /file.txt
GET /image.png
GET /missing.html
```

Check:

```text
200
404
```

---

## Directories

Test:

```text
directory + index
directory without index + autoindex on
directory without index + autoindex off
```

Expected:

```text
200
403
```

depending on configuration.

---

## MIME Types

Test:

```text
.html
.css
.js
.json
.png
.jpg
.gif
.svg
.pdf
.txt
unknown extension
```

---

## POST

Test:

```text
POST new resource
POST existing resource
POST empty body
POST binary body
POST to directory
POST to CGI
```

---

## DELETE

Test:

```text
DELETE existing file
DELETE missing file
DELETE directory
DELETE inaccessible file
```

---

## Routing

Test:

```text
multiple server_name values
unknown Host
location /
location /api
location /api/users
location /cgi-bin
/cgi-bin vs /cgi-binary
```

The last test is particularly important because it verifies correct location boundary matching.

---

## CGI

Test:

```text
successful CGI
CGI GET
CGI POST
CGI response headers
CGI without headers
CGI non-zero exit
missing CGI file
non-executable CGI
large CGI output
```

---

## HTTP

Test:

```text
HTTP/1.0
HTTP/1.1
Host
Connection: keep-alive
Connection: close
multiple requests on one connection
malformed requests
large requests
```

---

# Security Considerations

This is an educational web server, not a production web server.

Important areas that should be hardened include:

### Path traversal

Requests such as:

```text
/../../etc/passwd
```

must never allow access outside the configured document root.

The URI should be normalized and validated before filesystem access.

### Request size limits

The server should enforce limits for:

```text
request line
headers
body
```

to prevent excessive memory usage.

### CGI security

CGI execution should only be allowed for explicitly configured extensions.

### Timeouts

A production server needs timeouts for:

```text
client headers
request body
keep-alive connections
CGI execution
```

### Large files

The current implementation loads complete files into memory:

```cpp
std::vector<char>
```

This is simple and useful for the project, but a production server should consider streaming large files.

### CGI pipe handling

The current CGI implementation writes the request body and then reads CGI output sequentially.

Very large request bodies or CGI responses can expose pipe-buffer blocking issues. A more advanced implementation should coordinate pipe I/O using non-blocking/event-driven techniques.

---

# What This Project Teaches

Webserv provides practical experience with:

* C++98
* Object-oriented design
* TCP/IP
* Socket programming
* Non-blocking I/O
* `epoll`
* HTTP
* HTTP request parsing
* HTTP response serialization
* Virtual hosting
* URI routing
* Filesystem APIs
* File permissions
* MIME types
* Processes
* `fork()`
* `execv()`
* `dup2()`
* `pipe()`
* `waitpid()`
* CGI
* POSIX APIs
* Configuration parsing
* Persistent connections
* Binary data
* Error handling

---

# Project Scope

The objective is **not** to reproduce every feature of Nginx or Apache.

The important concept is understanding the complete lifecycle of an HTTP request:

```text
TCP bytes
    ↓
HTTP request
    ↓
HTTP parsing
    ↓
Routing
    ↓
Filesystem / CGI decision
    ↓
Request handling
    ↓
HTTP response
    ↓
Serialization
    ↓
TCP bytes
    ↓
Client
```

Understanding this pipeline provides a strong foundation for working later with higher-level technologies such as Node.js/Express, Java web frameworks, reverse proxies, and cloud infrastructure.

---

# Current HTTP Methods

| Method | Supported | Implementation                            |
| ------ | --------- | ----------------------------------------- |
| GET    | Yes       | Static files, directories, CGI, redirects |
| POST   | Yes       | File creation/overwrite and CGI           |
| DELETE | Yes       | Regular file deletion                     |
| Other  | No        | `405 Method Not Allowed`                  |

---

# Project Structure

A simplified view of the project:

```text
.
├── config/
│   ├── Config.hpp
│   ├── Location.hpp
│   ├── Server.hpp
│   └── ...
│
├── src/
│   ├── core/
│   │   ├── Server.*
│   │   ├── Connection.*
│   │   └── ...
│   │
│   ├── http/
│   │   ├── HttpRequest.*
│   │   ├── HttpResponse.*
│   │   └── ...
│   │
│   ├── router/
│   │   ├── Router.*
│   │   ├── RouteResult.*
│   │   └── ...
│   │
│   ├── handlers/
│   │   ├── RequestHandler.*
│   │   └── ...
│   │
│   └── utils/
│       ├── MimeTypes.*
│       └── ...
│
├── www/
│   ├── index.html
│   ├── ...
│   └── cgi-bin/
│
├── config/
│   └── example.conf
│
├── Makefile
└── README.md
```

The exact directory names may differ depending on the final project organization.

---

# Conclusion

Webserv is a low-level implementation of an HTTP server designed to demonstrate what happens between a browser sending an HTTP request and receiving an HTTP response.

The most important architectural flow is:

```text
Server
   ↓
Connection
   ↓
HttpRequest
   ↓
Router
   ↓
RouteResult
   ↓
RequestHandler
   ↓
HttpResponse
   ↓
serialize()
   ↓
Connection
   ↓
Client
```

The project combines networking, HTTP, operating-system primitives, filesystem operations, process management, configuration parsing, and object-oriented C++ design into one complete system.
