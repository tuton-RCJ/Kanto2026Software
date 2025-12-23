#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

struct Note
{
    int note;        // frequency in Hz
    double duration; // in beats
};

struct NoteMillis
{
    int note;     // frequency in Hz
    int duration; // milliseconds
};



class Buzzer
{
public:
    Buzzer(int pin);
    void beep(int note, double duration);
    void mute();
    void kouka();
    void boot();
    void EnterEvacuationZone();
    void PlayMusic(Note *notes, int length, int bpm);
    volatile bool isDisabled = false;
    void HappyBirthday();
    void RegisterMusic(NoteMillis *music);
    void update();

private:
    int _pin;
    int _bpm;
    void setFrequency(int freq);
    NoteMillis *_currentMusic;
    int nextNoteIndex = 0;
    unsigned long nextNoteTime = 0;
};

#endif