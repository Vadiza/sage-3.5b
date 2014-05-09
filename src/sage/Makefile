TOP_DIR = ../..
SRC_DIR = $(TOP_DIR)/src
OBJ_DIR = $(TOP_DIR)/obj/sage
LIB_DIR = $(TOP_DIR)/lib
BIN_DIR = $(TOP_DIR)/bin

include $(TOP_DIR)/config.mk

CFLAGS = ${SAGE_CFLAGS} -O3 $(SDL_CFLAGS) -I$(SRC_DIR)/QUANTA $(MAGICK_CFLAGS) $(GLEW_CFLAGS) $(GLSL_YUV_DEFINE) $(PORTAUDIO_CFLAGS) $(SUN_INCLUDE) -DSAGE_S3D -MMD
LDFLAGS = -O $(SDL_LDFLAGS) $(XLIBS) -lpthread -lm -ldl -L$(LIB_DIR) -lquanta $(GLEW_LDFLAGS) $(SUN_LDFLAGS) $(MAGICK_LDFLAGS)

SOURCES = \
misc.cpp \
sageMessage.cpp \
sageRect.cpp \
streamInfo.cpp \
sageBlock.cpp \
sageBuf.cpp \
sageBlockPool.cpp \
sageFrame.cpp \
sageBlockPartition.cpp \
tinyxml.cpp \
tinyxmlparser.cpp \
tinyxmlerror.cpp \
tinystr.cpp

OBJECTS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.o})
DEPENDS = $(addprefix $(OBJ_DIR)/,${SOURCES:.cpp=.d})

FSM_SOURCES = \
fsManager.cpp \
fsServer.cpp \
fsCore.cpp \
sageVirtualDesktop.cpp \
tileConfig.cpp \
displayInstance.cpp \
fsMain.cpp

FSM_OBJECTS = $(addprefix $(OBJ_DIR)/,${FSM_SOURCES:.cpp=.o})
FSM_DEPENDS = $(addprefix $(OBJ_DIR)/,${FSM_SOURCES:.cpp=.d})

RCV_SOURCES = \
sageDisplayManager.cpp \
fsClient.cpp \
sageSync.cpp \
sageDisplay.cpp \
sageTexture.cpp \
sageShader.cpp \
pixelDownloader.cpp \
streamProtocol.cpp \
sageTcpModule.cpp \
sageUdpModule.cpp \
sageReceiver.cpp \
sageDraw.cpp \
sageDrawObject.cpp \
overlayPointer.cpp \
overlayButton.cpp \
overlayApp.cpp	\
overlayIcon.cpp \
overlayLabel.cpp \
overlayMenu.cpp \
overlaySizer.cpp \
overlayPanel.cpp \
overlayThumbnail.cpp \
overlaySplitter.cpp \
overlayEnduranceThumbnail.cpp \
sageBlockQueue.cpp \
sageSharedData.cpp \
sageEvent.cpp \
sdlSingleContext.cpp \
font.cpp \
image.cpp

ifeq ($(MACHINE), Darwin)
RCV_SOURCES += SDLMain.cpp
endif

RCV_OBJECTS = $(addprefix $(OBJ_DIR)/,${RCV_SOURCES:.cpp=.o})
RCV_DEPENDS = $(addprefix $(OBJ_DIR)/,${RCV_SOURCES:.cpp=.d})

ARCV_SOURCES = \
sageAudioManager.cpp \
fsClient.cpp \
sageSync.cpp \
sageEvent.cpp \
sageReceiver.cpp \
sageAudioReceiver.cpp \
sageBlockQueue.cpp \
streamProtocol.cpp \
sageTcpModule.cpp \
sageUdpModule.cpp \
sageAudioCircBuf.cpp \
sageAudio.cpp \
sageAudioModule.cpp \
sageAudioSync.cpp \
audioFileReader.cpp \
audioFileWriter.cpp \
audioFormatManager.cpp \
audioConverter.cpp \
wavConverter.cpp

ARCV_OBJECTS = $(addprefix $(OBJ_DIR)/,${ARCV_SOURCES:.cpp=.o})
ARCV_DEPENDS = $(addprefix $(OBJ_DIR)/,${ARCV_SOURCES:.cpp=.d})

BRIDGE_SOURCES = \
sageBridge.cpp \
sageAudioBridge.cpp \
messageInterface.cpp \
sageSync.cpp \
streamProtocol.cpp \
sageTcpModule.cpp \
sageUdpModule.cpp \
sageSharedData.cpp \
appInstance.cpp \
sageStreamer.cpp \
bridgeStreamer.cpp \
sageBlockQueue.cpp \
sageEvent.cpp \
sageConfig.cpp \
sageReceiver.cpp \
sageDoubleBuf.cpp

BRIDGE_OBJECTS = $(addprefix $(OBJ_DIR)/,${BRIDGE_SOURCES:.cpp=.o})
BRIDGE_DEPENDS = $(addprefix $(OBJ_DIR)/,${BRIDGE_SOURCES:.cpp=.d})

SAIL_SOURCES = \
sail.cpp \
envInterface.cpp \
fsClient.cpp \
sageSync.cpp \
streamProtocol.cpp \
sageTcpModule.cpp \
sageUdpModule.cpp \
sageDoubleBuf.cpp \
sageConfig.cpp \
sageStreamer.cpp \
sageBlockStreamer.cpp \
appUI.cpp \
appWidgets.cpp \
libsage.cpp \
suil.cpp

ifdef AUDIO
SAIL_SOURCES += \
sageAudioCircBuf.cpp \
sageAudio.cpp \
sageAudioModule.cpp \
sageAudioSync.cpp \
audioFileReader.cpp \
audioFileWriter.cpp \
audioFormatManager.cpp \
sageAppAudio.cpp \
audioConverter.cpp \
wavConverter.cpp \
sageAudioStreamer.cpp
endif

SAIL_OBJECTS = $(addprefix $(OBJ_DIR)/,${SAIL_SOURCES:.cpp=.o})
SAIL_DEPENDS = $(addprefix $(OBJ_DIR)/,${SAIL_SOURCES:.cpp=.d})

UI_CONSOLE_SOURCES = \
uiConsole.cpp \
fsClient.cpp \
suil.cpp \
sageMessage.cpp \
misc.cpp

UI_CONSOLE_OBJECTS = $(addprefix $(OBJ_DIR)/,${UI_CONSOLE_SOURCES:.cpp=.o})
UI_CONSOLE_DEPENDS = $(addprefix $(OBJ_DIR)/,${UI_CONSOLE_SOURCES:.cpp=.d})

BRIDGE_CONSOLE_SOURCES = \
bridgeConsole.cpp \
messageInterface.cpp \
sageMessage.cpp \
misc.cpp

BRIDGE_CONSOLE_OBJECTS = $(addprefix $(OBJ_DIR)/,${BRIDGE_CONSOLE_SOURCES:.cpp=.o})
BRIDGE_CONSOLE_DEPENDS = $(addprefix $(OBJ_DIR)/,${BRIDGE_CONSOLE_SOURCES:.cpp=.d})

FS_CONSOLE_SOURCES = \
fsConsole.cpp \
fsClient.cpp \
suil.cpp \
sageMessage.cpp \
misc.cpp

FS_CONSOLE_OBJECTS = $(addprefix $(OBJ_DIR)/,${FS_CONSOLE_SOURCES:.cpp=.o})
FS_CONSOLE_DEPENDS = $(addprefix $(OBJ_DIR)/,${FS_CONSOLE_SOURCES:.cpp=.d})


TARGETS = \
$(LIB_DIR)/$(SAIL_LIB) \
$(BIN_DIR)/fsManager \
$(BIN_DIR)/sageDisplayManager \
$(BIN_DIR)/uiConsole \
$(BIN_DIR)/fsConsole \
$(BIN_DIR)/sageBridge \
$(BIN_DIR)/bridgeConsole

ifdef AUDIO
TARGETS += $(BIN_DIR)/sageAudioManager
endif

all: $(TARGETS)

ifdef AUDIO
$(LIB_DIR)/$(SAIL_LIB): $(OBJECTS) $(SAIL_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(SHARED_LDFLAGS) $(OBJECTS) $(SAIL_OBJECTS) $(PORTAUDIO_LDFLAGS) -lpthread -lm -ldl $(SUN_LDFLAGS) -L$(LIB_DIR) -lquanta -o $(LIB_DIR)/$(SAIL_LIB)
else
$(LIB_DIR)/$(SAIL_LIB): $(OBJECTS) $(SAIL_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(SHARED_LDFLAGS) $(OBJECTS) $(SAIL_OBJECTS) -lpthread -lm -ldl $(SUN_LDFLAGS) -L$(LIB_DIR) -lquanta -o $(LIB_DIR)/$(SAIL_LIB)
endif

$(BIN_DIR)/fsManager: $(OBJECTS) $(FSM_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(OBJECTS) $(FSM_OBJECTS) $(LDFLAGS) -o $(BIN_DIR)/fsManager

$(BIN_DIR)/sageDisplayManager: $(OBJECTS) $(RCV_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(OBJECTS) $(RCV_OBJECTS) $(FONT_LDFLAGS) $(LDFLAGS) -o $(BIN_DIR)/sageDisplayManager

$(BIN_DIR)/sageAudioManager: $(OBJECTS) $(ARCV_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(OBJECTS) $(ARCV_OBJECTS) $(LDFLAGS) $(PORTAUDIO_LDFLAGS) -o $(BIN_DIR)/sageAudioManager

$(BIN_DIR)/sageBridge: $(OBJECTS) $(BRIDGE_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(OBJECTS) $(BRIDGE_OBJECTS) $(LDFLAGS) -o $(BIN_DIR)/sageBridge

$(BIN_DIR)/uiConsole: $(UI_CONSOLE_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(UI_CONSOLE_OBJECTS) $(LDFLAGS) -o $(BIN_DIR)/uiConsole

$(BIN_DIR)/bridgeConsole: $(BRIDGE_CONSOLE_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(BRIDGE_CONSOLE_OBJECTS) $(LDFLAGS) -o $(BIN_DIR)/bridgeConsole

$(BIN_DIR)/fsConsole: $(FS_CONSOLE_OBJECTS)
	$(CC) $(SAGE_LDFLAGS) $(FS_CONSOLE_OBJECTS) $(LDFLAGS) $(READLINE_LDFLAGS) -o $(BIN_DIR)/fsConsole

$(OBJ_DIR)/SDLMain.o: SDLMain.m
	$(CC) $(SAGE_LDFLAGS) -c SDLMain.m -o $(OBJ_DIR)/SDLMain.o

$(OBJ_DIR)/fsConsole.o: fsConsole.cpp
	$(CC) $(SAGE_LDFLAGS) $(CFLAGS) $(READLINE_CFLAGS) -c fsConsole.cpp -o $(OBJ_DIR)/fsConsole.o

$(OBJ_DIR)/%.o : %.c
	mkdir -p $(OBJ_DIR)
	$(cc) -c -o $@ $<

$(OBJ_DIR)/%.o : %.cpp
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ_DIR)/*.d $(OBJ_DIR)/*.o
	rm -f $(TARGETS)

distclean: clean

-include $(DEPENDS) $(FSM_DEPENDS) $(RCV_DEPENDS) $(ARCV_DEPENDS) $(BRIDGE_DEPENDS) $(SAIL_DEPENDS) $(UI_CONSOLE_DEPENDS) $(BRIDGE_CONSOLE_DEPENDS) $(FS_CONSOLE_DEPENDS)
