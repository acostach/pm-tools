BIN = pm-daemon
FILES = pm.cpp

build:
	$(CXX) $(FILES) -o $(BIN) $(CXXFLAGS) $(LDFLAGS) `pkg-config --cflags --libs glib-2.0`

.PHONY: clean

clean:
	rm $(BIN) || true
