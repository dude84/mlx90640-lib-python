# MLX90640 C++ Library Makefile
# LINUX I2C mode only (uses standard Linux I2C drivers)

# Build mode: release by default, debug if DEBUG=1
ifdef DEBUG
	CXXFLAGS+=-DDEBUG -g
else
	CXXFLAGS+=-O3 -march=native -DNDEBUG
endif

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
	PREFIX = /usr/local
endif

all: libMLX90640_API.a libMLX90640_API.so

libMLX90640_API.so: functions/MLX90640_API.o functions/MLX90640_LINUX_I2C_Driver.o
	$(CXX) -fPIC -shared $^ -o $@

libMLX90640_API.a: functions/MLX90640_API.o functions/MLX90640_LINUX_I2C_Driver.o
	ar rcs $@ $^
	ranlib $@

functions/MLX90640_API.o functions/MLX90640_LINUX_I2C_Driver.o : CXXFLAGS+=-fPIC -I headers -shared

clean:
	rm -f functions/*.o
	rm -f *.o
	rm -f *.so
	rm -f *.a

install: libMLX90640_API.a libMLX90640_API.so
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libMLX90640_API.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 libMLX90640_API.so $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/MLX90640/
	install -m 644 headers/*.h $(DESTDIR)$(PREFIX)/include/MLX90640/
	ldconfig
