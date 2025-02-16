NAME	:= webserv

CC		:= c++
CFLAGS	:= -g -std=c++20
INC_DIR  := ./include
SRC_DIR  := ./src
OBJ_DIR  := ./obj

SRCS	:= $(SRC_DIR)/main.cpp \
			$(SRC_DIR)/ConfigFile.cpp \
			$(SRC_DIR)/CgiHandler.cpp \
			$(SRC_DIR)/HandleRequest.cpp \
			$(SRC_DIR)/HttpParser.cpp \
			$(SRC_DIR)/Server.cpp \
			$(SRC_DIR)/HttpResponse.cpp
OBJS	:= $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

INC_FLAG := -I include

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INC_FLAG) -o $@ -c $< && printf "Compiling: $(notdir $<)\n"

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

all: $(NAME)

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

.PHONY: all, clean, fclean, re
