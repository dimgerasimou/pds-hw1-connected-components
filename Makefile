# --- Compiler settings ---
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lmatio

# Αν το matio είναι εγκατεστημένο στο /usr/local
# ξεκλείδωσε την επόμενη γραμμή:
# CFLAGS += -I/usr/local/include
# LDFLAGS += -L/usr/local/lib

# --- Files ---
SRC = main.c read_mat.c
OBJ = $(SRC:.c=.o)
TARGET = print_mat

# --- Rules ---
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
