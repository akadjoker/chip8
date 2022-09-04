MLX_DIR = minilibx


UNAME := $(shell uname)


NAME= teste
SRC = srcs/main.c

OBJ = $(SRC:%.c=%.o)


CC	= gcc -Wall  $(INC)
LFLAGS = -lXext -lX11 -lm -lz
INC = -I ./inc -I $(MLX_DIR) 
LIB = -L $(MLX_DIR) -lmlx 


# Colors #
GREEN		=	\033[0;32m
YELLOW		=	\033[0;33m
RED			=	\033[0;31m
RESET       =   \033[0m

all: $(NAME)


$(NAME): $(OBJ)
	@echo "$(YELLOW)[ Compiling  ]$(RESET)"
	$(CC) -o $(NAME) $(OBJ) $(INC) $(LIB) $(LFLAGS)
	mkdir -p build \build/objs
	mv $(OBJ) build/objs



mlx:
	make -s -C $(MLX_DIR)
	@echo "$(GREEN)  MLX Created  $(RESET)"


clean:
	rm -rf build/objs
	@echo "\n$(YELLOW)[      objects were removed      ]$(RESET)"

fclean: clean
	make -s fclean 
	rm -rf build


re: fclean all



