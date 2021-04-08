CC = g++
INCLUDES = -I/usr/include/eigen3/

ifeq (${mode}, release)
	FLAGS = -O3 -march=native
else
	mode = debug
	FLAGS = -O0 -g
endif

GL_FLAGS = -lglfw -lGL -ldl
LIB_FLAGS =
OBJS = glad.o gpu_immediate.o
PROJECT_NAME = gp_project

ifeq (${mode}, debug)
	PROJECT = ${PROJECT_NAME}_debug
else
	PROJECT = ${PROJECT_NAME}
endif

${PROJECT}: ${OBJS} main.o clean_emacs_files
	@echo "Building on "${mode}" mode"
	@echo ".........................."
	${CC} ${INCLUDES} ${FLAGS} ${OBJS} main.o -o $@ ${GL_FLAGS} ${LIB_FLAGS}
	-make clean

glad.o:
	${CC} -c glad.c -o $@ ${GL_FLAGS}
main.o:
	${CC} ${INCLUDES} ${FLAGS} -c main.cpp -o $@ ${GL_FLAGS} ${LIB_FLAGS}
gpu_immediate.o:
	${CC} ${INCLUDES} ${FLAGS} -c gpu_immediate.cpp -o $@ ${GL_FLAGS} ${LIB_FLAGS}

.PHONEY: clean clean_emacs_files clean_all
clean:
	-rm -rf ${OBJS} main.o
clean_emacs_files:
	-rm -rf *~
clean_all: clean clean_emacs_files
	-rm -rf ${PROJECT_NAME} ${PROJECT_NAME}_debug
