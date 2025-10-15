#ifndef SCROLLER_H
#define SCROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

void Roll_Credits(char* text_file, char* sample_file, byte (*font)[8]);
void Text_Scroller(char *filename, byte font[256][8]);

#ifdef __cplusplus
}
#endif

#endif
