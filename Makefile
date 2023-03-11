CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDLIBS = -lws2_32

BUILD_DIR = build

all: server.exe client.exe

server.exe: $(BUILD_DIR)/server.o | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

client.exe: $(BUILD_DIR)/client.o | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBS)

$(BUILD_DIR)/server.o: src/server.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/client.o: src/client.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

clean:
	if exist $(BUILD_DIR) rmdir /Q /S $(BUILD_DIR)
	if exist server.exe del server.exe
	if exist client.exe del client.exe