#include <stdarg.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inc/types.h"
#include "inc/logger.h"

#define DEBUG_BUFFER_SIZE 32768
#define MAX_LINE_LENGTH 256

bool __DEBUG = false;

static char* _debugBuffer = NULL;
static bool _initialized = 0;
static int _bufferPos = 0;

static bool log_init();
static void log_clear();
static void log_cleanup();

void Log_Info(char* format, ...) {
	va_list args;
	char line_buffer[MAX_LINE_LENGTH];
	int line_length;

	if (!_initialized && !log_init()) return;

	// Format the message
	va_start(args, format);
	line_length = vsprintf(line_buffer, format, args);
	va_end(args);

	if (line_length > 0 && line_buffer[line_length - 1] != '\n') {
		if (line_length < MAX_LINE_LENGTH - 2) {
			line_buffer[line_length] = '\n';
			line_buffer[line_length + 1] = '\0';
			line_length++;
		}
	}

	// buffer is full, skip this message
	if (_bufferPos + line_length >= DEBUG_BUFFER_SIZE - 1) return;

	strcpy(_debugBuffer + _bufferPos, line_buffer);
	_bufferPos += line_length;
}

void Log_Dump() {
	FILE* logfile; addr* write_buffer;
	if (!_initialized || !_debugBuffer) return;

	if (_bufferPos == 0) {
		printf("Debug log is empty.\n");
		return;
	}

	logfile = fopen("debug.log", "w+");
	if (logfile) {
		fprintf(logfile, "=== DEBUG LOG START ===\n");
		fwrite(_debugBuffer, _bufferPos, 1, logfile);
		fprintf(logfile, "=== DEBUG LOG END ===\n");
		fprintf(logfile, "Total log size: %d bytes\n", _bufferPos);
		fclose(logfile);
	}

	printf("\n=== DEBUG LOG START ===\n");
	printf("%s", _debugBuffer);
	printf("=== DEBUG LOG END ===\n");
	printf("Total log size: %d bytes\n", _bufferPos);
}

static bool log_init() {
	if (!__DEBUG) return false;
	if (_initialized) return true;

	_debugBuffer = (char*)malloc(DEBUG_BUFFER_SIZE);
	if (!_debugBuffer) return 0;

	memset(_debugBuffer, 0, DEBUG_BUFFER_SIZE);
	_bufferPos = 0;
	_initialized = true;

	// cleanup at program exit
	atexit(log_cleanup);
	return _initialized;
}

static void log_clear() {
	if (_initialized && _debugBuffer) {
		memset(_debugBuffer, 0, DEBUG_BUFFER_SIZE);
		_bufferPos = 0;
	}
}

static void log_cleanup() {
	if (_initialized && _debugBuffer) {
		free(_debugBuffer);
		_debugBuffer = NULL;
		_bufferPos = 0;
		_initialized = false;
	}
}
