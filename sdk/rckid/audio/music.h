#pragma once

#include "tone.h"

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

    /** Single channel music controller for a tone generator. 
     
        Music contains notes for a single tone generator where it taps to the onDone callback to issue more notes. 

        TODO add the music commands, figure out encoding, etc.   
     */
    class Music {
    public:

        /** Single note information. 
         */
        PACKED(class NoteInfo {
        public:
            Note note;
            NoteLength length;
        }); 

        void attachTo(Tone & tone) {
            tone.setOnDone([this](Tone & tone) { next(tone); });
            next(tone);
        }

        void detach(Tone & tone) {
            tone.clearOnDone();
            tone.off();
        }

        void setSheet(uint8_t const * sheet, uint32_t sheetSize) {
            sheet_ = sheet;
            sheetSize_ = sheetSize;
            tempo_ = Tempo::Allegro;
        }

        template<uint32_t SIZE>
        void setSheet(uint8_t const (&sheet)[SIZE]) { setSheet(sheet, SIZE); }

        static constexpr uint32_t frequencyOf(Note n) {
            switch (n) {
        #define NOTE(NAME, FREQUENCY, ID) case Note::NAME: return (FREQUENCY + 5) / 10;
        #include "notes.inc.h"
                default:
                    return 0;
            }
        }

        /** Converts note length to microseconds given the duration for a quarter note beat.  
         */
        static constexpr uint32_t durationOf(NoteLength l, uint32_t quarterBeat) {
            return quarterBeat * static_cast<unsigned>(l) / 40; // quarter note length in 40
        }

        /** Converts note length to microseconds given a tempo. 
         */
        static constexpr uint32_t durationOf(NoteLength l, Tempo t) {
            return durationOf(l, static_cast<unsigned>(t) * 10); // tempo values are 10ms intervals
        }


    private:

        void next(Tone & tone) {
            NoteInfo const * ni = reinterpret_cast<NoteInfo const *>(sheet_ + i_);
            tone.setFrequency(frequencyOf(ni->note), durationOf(ni->length, tempo_));
            i_ += 2;
            if (i_ >= sheetSize_)
                i_ = 0;
        }

        uint8_t const * sheet_ = nullptr; 
        uint32_t sheetSize_ = 0;
        uint32_t i_ = 0;
        Tempo tempo_;

    }; 


    /** Some macros for simplifying writing notes directly in header files. 
     */
    #define NOTE(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::Full)
    #define NOTE_2(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::Half)
    #define NOTE_4(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::Quarter)
    #define NOTE_8(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::Eighth)
    #define NOTE_16(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::Sixteenth)
    #define NOTE_L(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::FullLong)
    #define NOTE_2L(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::HalfLong)
    #define NOTE_4L(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::QuarterLong)
    #define NOTE_8L(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::EighthLong)
    #define NOTE_16L(N) static_cast<uint8_t>(Note::N), static_cast<uint8_t>(NoteLength::SixteenthLong)
    #define REST static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::Full)
    #define REST_2 static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::Half)
    #define REST_4 static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::Quarter)
    #define REST_8 static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::Eighth)
    #define REST_16 static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::Sixteenth)
    #define REST_L static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::FullLong)
    #define REST_2L static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::HalfLong)
    #define REST_4L static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::QuarterLong)
    #define REST_8L static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::EighthLong)
    #define REST_16L static_cast<uint8_t>(Note::Off), static_cast<uint8_t>(NoteLength::SixteenthLong)

} // namespace rckid