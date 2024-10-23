#pragma once

#include <cstdint>

namespace rckid {

    /** Musical tempo shortcuts with the numeric values corresponding to single beat length in 10ms intervals. 
     */
    enum class Tempo {
        Adagio = 109, // 55,  44 - 66, 
        Andante = 73, // 82,  56 - 108
        Moderato = 53, // 114,  108- 120, 
        Allegro = 44, // 138,  120 - 156
        Vivace = 36, // 166,  156 - 176
        Presto = 33, // 184, 168 - 200
    }; // rckid::Tempo

    /** Musical length of a note. 
     */
    enum class NoteLength : uint8_t {
        Full = 160, 
        Half = 80, 
        Quarter = 40,
        Eighth = 20,
        Sixteenth = 10,
        FullLong = 240, 
        HalfLong = 120, 
        QuarterLong = 60,
        EighthLong = 30, 
        SixteenthLong = 15,
    }; // rckid::NoteLength

    /** Musical note, identified by its MIDI id to save space. 
     */
    enum class Note : uint8_t {
        Off = 0, 
    #define NOTE(NAME, FREQUENCY, ID) NAME = ID, 
    #include "notes.inc.h"
    }; // rckid::Note

    /** Single note information. 
     */
    PACKED(struct NoteInfo {
        Note note;
        NoteLength length;

        constexpr NoteInfo(Note n, NoteLength l): note{n}, length{l} {}
    });

    constexpr inline uint16_t frequencyOf(Note n) {
        switch (n) {
    #define NOTE(NAME, FREQUENCY, ID) case Note::NAME: return FREQUENCY;
    #include "notes.inc.h"
            default:
                return 0;
        }
    }

    /** Converts note length to microseconds given the duration for a quarter note beat.  
     */
    constexpr inline unsigned durationOf(NoteLength l, unsigned quarterBeat) {
        return quarterBeat * static_cast<unsigned>(l) / 40; // quarter note length in 40
    }

    /** Converts note length to microseconds given a tempo. 
     */
    constexpr inline unsigned durationOf(NoteLength l, Tempo t) {
        return durationOf(l, static_cast<unsigned>(t) * 10000); // tempo values are ms / 10
    }

    /** Some macros for simplifying writing notes directly in header files. 
     */
    #define NOTE(N) NoteInfo{Note::N, NoteLength::Full}
    #define NOTE_2(N) NoteInfo{Note::N, NoteLength::Half}
    #define NOTE_4(N) NoteInfo{Note::N, NoteLength::Quarter}
    #define NOTE_8(N) NoteInfo{Note::N, NoteLength::Eighth}
    #define NOTE_16(N) NoteInfo{Note::N, NoteLength::Sixteenth}
    #define NOTE_L(N) NoteInfo{Note::N, NoteLength::FullLong}
    #define NOTE_2L(N) NoteInfo{Note::N, NoteLength::HalfLong}
    #define NOTE_4L(N) NoteInfo{Note::N, NoteLength::QuarterLong}
    #define NOTE_8L(N) NoteInfo{Note::N, NoteLength::EighthLong}
    #define NOTE_16L(N) NoteInfo{Note::N, NoteLength::SixteenthLong}
    #define REST NoteInfo{Note::Off, NoteLength::Full}
    #define REST_2 NoteInfo{Note::Off, NoteLength::Half}
    #define REST_4 NoteInfo{Note::Off, NoteLength::Quarter}
    #define REST_8 NoteInfo{Note::Off, NoteLength::Eighth}
    #define REST_16 NoteInfo{Note::Off, NoteLength::Sixteenth}
    #define REST_L NoteInfo{Note::Off, NoteLength::FullLong}
    #define REST_2L NoteInfo{Note::Off, NoteLength::HalfLong}
    #define REST_4L NoteInfo{Note::Off, NoteLength::QuarterLong}
    #define REST_8L NoteInfo{Note::Off, NoteLength::EighthLong}
    #define REST_16L NoteInfo{Note::Off, NoteLength::SixteenthLong}

} // namespace rckid