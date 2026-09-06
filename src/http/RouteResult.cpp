#include "RouteResult.hpp"


RouteResult::RouteResult()
    : server(NULL),
      location(NULL),
      fileSystemPath(""),
      isCGI(false),
      autoindex(false),
      cgiExecutable("")
{
}