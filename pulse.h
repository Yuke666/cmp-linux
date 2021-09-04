#ifndef PULSE_DEF
#define PULSE_DEF

void Pulse_Close();
int Pulse_Open();
void Pulse_Update();
void Pulse_PlayData(char *data, int len);
void Pulse_SetSampleRate(int sr);
void Pulse_SetChannels(int num);
short *Pulse_GetCurrentSamples(int *num);
void Pulse_StopPlaying();
void Pulse_StartPlaying();
void Pulse_Update();
void Pulse_SetVolume(float vol);
int Pulse_GetVolume();
void Pulse_Seek(float amount);
float Pulse_GetSeek();
void Pulse_Pause();
void Pulse_Unpause();
int Pulse_GetDurationInSecs();
int Pulse_GetSampleIndex();
void Pulse_AddSample(char sample);
void Pulse_SetSampleIndex(int index);
void Pulse_ClearSamples();
int Pulse_GetWritableSize();
void Pulse_Write(short *samples, int len);
int Pulse_GetSampleRate();
int Pulse_GetSamplesSize();
int Pulse_GetSampleOffset();
#endif