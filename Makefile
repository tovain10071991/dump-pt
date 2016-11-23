#export LD_LIBRARY_PATH=/home/user/Documents/libelf-0.8.13/install/lib/

ELF_INCLUDE := /home/user/Documents/libelf-0.8.13/install/include/
ELF_LIBRARY := /home/user/Documents/libelf-0.8.13/install/lib/

INCLUDE := -I $(ELF_INCLUDE)
LIB := -L $(ELF_LIBRARY)
CXXFLAGS := -c -g -Wall $(INCLUDE) -D_GNU_SOURCE -std=c++11
LDFLAGS := -lelf -lipt

EXECUTE := dump-pt
SOURCE := $(wildcard *.cpp)
OBJECT := $(SOURCE:.cpp=.o)
OUTPUT := $(wildcard *out*)

all:  $(EXECUTE)

$(EXECUTE): $(OBJECT)
	$(CXX) $(LIB) -o $(EXECUTE) $(OBJECT) $(LDFLAGS)

$(OBJECT): $(SOURCE)

clean:
	rm $(EXECUTE) $(OBJECT) $(OUTPUT)

clean-out:
	rm $(OUTPUT)

