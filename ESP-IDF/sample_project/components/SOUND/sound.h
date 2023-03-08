#ifndef sound
#define sound

static void sound_init(void);
static void sendAudioCommand(uint8_t command, uint16_t parameter);
static void sound_task(void *arg);

#endif