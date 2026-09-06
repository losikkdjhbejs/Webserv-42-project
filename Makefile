NAME = p

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS = main.cpp \
		src/config/ConfigLoader.cpp  src/config/Lexer.cpp  src/config/Parser.cpp  src/config/Validator.cpp  \
		src/http/HttpRequest.cpp  src/http/HttpResponse.cpp  src/http/RequestHandler.cpp  src/http/Router.cpp  src/http/RouteResult.cpp \
		src/core/Connection.cpp  src/core/WebServer.cpp\
		src/utils/MimeTypes.cpp



OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all