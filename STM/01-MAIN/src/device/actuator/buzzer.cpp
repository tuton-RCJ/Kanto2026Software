#include "buzzer.h"

#define C3 131
#define D3 147
#define E3 165
#define F3 175
#define G3 196
#define A3 220
#define B3 247
#define C4 262
#define D4 294
#define E4 330
#define F4 349
#define G4 392
#define A4 440
#define B4 494
#define C5 523
#define D5 587
#define E5 659
#define F5 698
#define G5 784
#define A5 880
#define B5 988
#define C6 1047
#define Bb5 932

Buzzer::Buzzer(int pin)
{
    _pin = pin;
    pinMode(_pin, OUTPUT);
    _currentMusic = nullptr;
    _currentMusicLength = 0;
}

void Buzzer::setFrequency(int freq)
{
    if (isDisabled)
    {
        return;
    }
    if (freq == 0)
    {
        analogWrite(_pin, 0);
        return;
    }
    analogWriteFrequency(freq);
    analogWrite(_pin, 64);
}

void Buzzer::mute()
{
    analogWrite(_pin, 0);
    if (_currentMusic != nullptr)
    {
        // 音楽再生中なら楽譜を削除
        _currentMusic = nullptr;
        _currentMusicLength = 0;
        nextNoteIndex = 0;
        nextNoteTime = 0;
    }
}

void Buzzer::beep(int note, double duration)
{
    int interbal = 10;
    long beepDuration = 60000.0 / _bpm * duration - interbal;
    setFrequency(note);
    delay(beepDuration);
    mute();
    delay(interbal);
}

//----------------------------------------------------------

void Buzzer::boot()
{
    Note notes[] = {
        {C5, 0.5},
        {E5, 0.5},
        {G5, 0.5},
    };

    PlayMusic(notes, 3, 400);
}

void Buzzer::kouka()
{
    Note notes[] = {
        {C4, 1},
        {E4, 1},
        {G4, 1.5},
        {F4, 0.5},
        {E4, 1},
        {C4, 1},
        {D4, 2},
        {E4, 1},
        {E4, 1},
        {D4, 1.5},
        {C4, 0.5},
        {G4, 4},
        {E4, 1},
        {E4, 1},
        {D4, 1.5},
        {C4, 0.5},
        {A3, 1},
        {C4, 1},
        {G3, 2},
        {C4, 1},
        {E4, 1},
        {D4, 1.5},
        {G3, 0.5},
        {C4, 4},
    };

    PlayMusic(notes, 24, 100);
}

// Noteの配列を受け取り、1つずつbeep()で音を鳴らす、Disabledなら即座にreturn
void Buzzer::PlayMusic(Note *notes, int length, int bpm)
{
    _bpm = bpm;
    for (int i = 0; i < length; i++)
    {
        if (isDisabled)
        {
            return;
        }
        beep(notes[i].note, notes[i].duration);
    }
}

void Buzzer::HappyBirthday()
{
    Note notes[] = {
        {C5, 0.75},
        {C5, 0.25},
        {D5, 1},
        {C5, 1},
        {F5, 1},
        {E5, 2},
        {C5, 0.75},
        {C5, 0.25},
        {D5, 1},
        {C5, 1},
        {G5, 1},
        {F5, 2},
        {C5, 0.75},
        {C5, 0.25},
        {C6, 1},
        {A5, 1},
        {F5, 1},
        {E5, 1},
        {D5, 1},
        {Bb5, 0.75},
        {Bb5, 0.25},
        {A5, 1},
        {F5, 1},
        {G5, 1},
        {F5, 2},
    };
    PlayMusic(notes, 25, 150);
}

void Buzzer::RegisterMusic(const NoteMillis *music, int length)
{
    if (music == nullptr || length <= 0)
    {
        mute();
        return;
    }

    if (length > MAX_MUSIC_LEN)
    {
        length = MAX_MUSIC_LEN;
    }

    for (int i = 0; i < length; i++)
    {
        _musicBuffer[i] = music[i];
    }

    _currentMusic = _musicBuffer;
    _currentMusicLength = length;
    nextNoteIndex = 0;
    nextNoteTime = 0;
}

void Buzzer::update()
{
    if (_currentMusic == nullptr || isDisabled)
    {
        return;
    }

    unsigned long currentTime = millis();
    if (currentTime >= nextNoteTime)
    {
        if (nextNoteIndex >= _currentMusicLength)
        {
            // 音楽の終了
            mute();
            return;
        }

        NoteMillis currentNote = _currentMusic[nextNoteIndex];
        setFrequency(currentNote.note);
        nextNoteTime = currentTime + currentNote.duration;
        nextNoteIndex++;
    }
}

void Buzzer::jingleBells()
{
    Note notes[] = {
        {E5, 0.5}, {E5, 0.5}, {E5, 1}, {E5, 0.5}, {E5, 0.5}, {E5, 1}, {E5, 0.5}, {G5, 0.5}, {C5, 0.75}, {D5, 0.25}, {E5, 2}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {E5, 0.5}, {E5, 0.5}, {E5, 0.5}, {E5, 0.5}, {D5, 0.5}, {D5, 0.5}, {E5, 0.5}, {D5, 1}, {G5, 1}, {E5, 0.5}, {E5, 0.5}, {E5, 1}, {E5, 0.5}, {E5, 0.5}, {E5, 1}, {E5, 0.5}, {G5, 0.5}, {C5, 0.75}, {D5, 0.25}, {E5, 2}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {F5, 0.5}, {E5, 0.5}, {E5, 0.5}, {E5, 0.5}, {G5, 0.5}, {G5, 0.5}, {F5, 0.5}, {D5, 0.5}, {C5, 2}};
    PlayMusic(notes, sizeof(notes) / sizeof(notes[0]), 150);
}
