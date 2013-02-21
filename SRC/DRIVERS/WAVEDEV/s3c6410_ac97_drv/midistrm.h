//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
#pragma once
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------

// Enable to turn on linear interpolation between samples in sine table
// #instructions in inner loop: ~7 added
#define MIDI_OPTIMIZE_LINEAR_INTERPOLATE 0

#define NUMCHANNELS  (16)
#define NUMENVELOPES (4)
#define NUMNOTES     (32)

// Special channel reserved for playing arbitrary tones
#define FREQCHANNEL (NUMCHANNELS)

class CMidiNote;
class CMidiStream;


typedef struct _ENVELOPE
{
    UINT32 Slope;
    UINT32 Count;
} ENVELOPE;


class CMidiNote
{
public:
    UINT32 NoteVal()      {return m_Note;}
    UINT32 NoteChannel()  {return m_Channel;}
    HRESULT NoteOn(CMidiStream *pMidiStream, UINT32 dwChannel, UINT32 dwNote, UINT32 dwVelocity);
    HRESULT NoteOff(UINT32 dwVelocity);
    PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast);
    PBYTE Render2(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast);

    void GainChange();

    void SetVelocity(UINT32 Velocity)
    {
        // Reset the bytes left value here. This ensures that if a note is going away we bring it back.
        m_dwBytesLeft = (DWORD)-1;

        m_Velocity = Velocity;
        m_dwGain   = Velocity<<9;
        GainChange();
    }

    LIST_ENTRY m_Link;

private:
    static const UINT32 PitchTable[12];                 // Pitch table
    static const ENVELOPE EnvelopeTable[NUMENVELOPES];  // Envelope table
    static const INT16 SineTable[0x101];                // Sine table

    CMidiStream *m_pMidiStream;
    UINT32 m_Note;
    UINT32 m_Velocity;
    UINT32 m_Channel;

    UINT32 m_Index;          // Current index into wavetable
    UINT32 m_IndexDelta;     // Amount to increment index on each sample

    DWORD   m_dwGain;        // gain as set through SetVolume, and reported through GetVolume
    DWORD   m_fxpGain;       // effective gain, after composing with device and class gains.

    DWORD   m_dwBytesLeft;
};


class CMidiStream : public StreamContext
{
public:
    HRESULT Open(DeviceContext *pDeviceContext, LPWAVEOPENDESC lpWOD, DWORD dwFlags);
    DWORD Close();

    PBYTE Render(PBYTE pBuffer, PBYTE pBufferEnd, PBYTE pBufferLast);

    UINT32 DeltaTicksToByteCount(UINT32 DeltaTicks);


    void NoteMoveToFreeList(CMidiNote *pCMidiNote)
    {
        PLIST_ENTRY pListEntry = &pCMidiNote->m_Link;
        RemoveEntryList(pListEntry);
        InsertTailList(&m_FreeList,pListEntry);
    }


    void NoteMoveToNoteList(CMidiNote *pCMidiNote)
    {
        PLIST_ENTRY pListEntry = &pCMidiNote->m_Link;
        RemoveEntryList(pListEntry);
        InsertTailList(&m_NoteList,pListEntry);
    }


    void NoteDone(CMidiNote *pCMidiNote)
    {
        NoteMoveToFreeList(pCMidiNote);
        Release();
    }


    HRESULT NoteOn(UINT32 dwNote, UINT32 dwVelocity, UINT32 dwChannel);
    HRESULT NoteOff(UINT32 dwNote, UINT32 dwVelocity, UINT32 dwChannel);
    HRESULT AllNotesOff(UINT32 dwVelocity);

    DWORD MidiMessage(UINT32 dwMsg);
    HRESULT InternalMidiMessage(UINT32 dwData);
    HRESULT MidiData(UINT32 dwData);

    CMidiNote *FindNote(UINT32 dwNote, UINT32 dwChannel);
    CMidiNote *AllocNote(UINT32 dwNote, UINT32 dwChannel);


    UINT32 DeltaTicksToSamples(UINT32 DeltaTicks)
    {
        return (DeltaTicks * m_SamplesPerTick);
    }


    HRESULT UpdateTempo();

    UINT32 ProcessMidiStream();

    DWORD MapNoteGain(DWORD NoteGain);
    void GainChange();

    DWORD Reset();


protected:
    CMidiNote       m_MidiNote[NUMNOTES];
    LIST_ENTRY      m_NoteList;
    LIST_ENTRY      m_FreeList;
    UINT32          m_ByteCount;
    BYTE            m_RunningStatus;
    UINT32          m_USecPerQuarterNote;     // Tempo, from midi stream
    UINT32          m_TicksPerQuarterNote;    // PPQN, from midi header
    UINT32          m_SamplesPerTick;         // Calculated as (SampleRate * Tempo)/(PPQN * 1000000)
    UINT32          m_DeltaSampleCount;       // # of samples since last midi event (init to 0)
};

