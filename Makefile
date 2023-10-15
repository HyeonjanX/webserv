CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
CXXFLAGS += -MD

LDFLAGS =
INCLUDE = -I.
RM = rm -rf

SRCDIR = ./src/
OBJDIR = ./build/

SOURCE = Cgi.cpp \
	   Client.cpp \
	   Config.cpp \
	   EventHandler.cpp \
	   File.cpp \
	   Host.cpp \
	   JsonData.cpp \
	   JsonParser.cpp \
	   Location.cpp \
	   Request.cpp \
	   Response.cpp \
	   Server.cpp \
	   Util.cpp \
	   Webserver.cpp \
	   main.cpp
SRCS = $(addprefix $(SRCDIR), $(SOURCE))
OBJECT = $(patsubst %.cpp, %.o, $(SOURCE))
OBJS = $(addprefix $(OBJDIR), $(OBJECT))
DEPS = $(OBJS:.o=.d)

ifdef DEBUG
	CXXFLAGS += -DDEBUG
	CXXFLAGS += -fsanitize=address -g3
endif

NAME = webserv

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS)

-include $(DEPS)

.PHONY: all debug clean fclean re

all: $(NAME)

debug: fclean
	$(MAKE) all DEBUG=1

clean:
	$(RM) $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean
	$(MAKE) all
