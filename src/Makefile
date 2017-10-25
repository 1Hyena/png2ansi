NAME    = png2ansi

CC      = gcc
PROF    = -O -g

C_FLAGS = -Wall $(PROF)
L_FLAGS = -lstdc++ -lpng

OBJ_DIR = obj

SRC_FILES := $(wildcard *.cpp)
O_FILES   := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

all:
	@$(MAKE) dynamically_linked -s

dynamically_linked: $(O_FILES)
	@printf "\033[1;33mMaking \033[37m   ...."
	$(CC) -o ../$(NAME) $(O_FILES) $(L_FLAGS)
	@printf "\033[1;32m Dynamically linked %s DONE!\033[0m\n" $(NAME)

$(OBJ_DIR)/more/%.o: more/%.cpp
		@printf "\033[1m\033[36mCompiling \033[37m....\033[1;34m %-30s\033[1;33m%6s\033[31m lines\033[0m \n" more/$*.cpp "`wc -l more/$*.cpp | cut -f1 -d' '`"
		@$(CC) $< $(C_FLAGS) -c -o $@

$(OBJ_DIR)/%.o: %.cpp
		@printf "\033[1m\033[31mCompiling \033[37m....\033[34m %-30s\033[33m%6s\033[31m lines\033[0m \n" $*.cpp "`wc -l $*.cpp | cut -f1 -d' '`"
		@$(CC) $< $(C_FLAGS) -c -o $@

clean:
	@printf "\033[1;36mCleaning up\033[37m ...."
	@rm -f $(O_FILES) *~ *.bak *.orig *.rej
	@printf "\033[1;37m ALL CLEAN!\033[0m\n"

