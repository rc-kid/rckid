#pragma once

#include "note.h"
#include "tone.h"
namespace rckid {

    /** A very simple single channel tone generator. 

        Takes music file (array of NoteInfo)
     
     */
    template<typename TONE_GENERATOR>
    class Music : public TONE_GENERATOR {
    public:
        using TONE_GENERATOR::setFrequency;

        template<size_t SIZE>
        Music(NoteInfo const (&music)[SIZE]):
            notes_{music}, 
            numNotes_{SIZE} {
            // start playing the first note
            setFrequency(frequencyOf(notes_[i_].note), durationOf(notes_[i_].length, tempo_));
        }

    protected:

        void onDone() override {
            if (++i_ == numNotes_)
                i_ = 0;
            setFrequency(frequencyOf(notes_[i_].note), durationOf(notes_[i_].length, tempo_));
        }

    private:

        Tempo tempo_ = Tempo::Allegro;
        NoteInfo const * notes_ = nullptr; 
        size_t numNotes_ = 0;
        size_t i_ = 0;

    }; // rckid::Music
}