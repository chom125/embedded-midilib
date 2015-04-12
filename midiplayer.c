/*
* mididump.c - A complete textual dump of a MIDI file.
*				Requires Steevs MIDI Library & Utilities
*				as it demonstrates the text name resolution code.
* Version 1.4
*
*  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
*			Copyright 2010, Steven Goodwin.
*
*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License as
*  published by the Free Software Foundation; either version 2 of
*  the License,or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*
* TODO: - rename midi events to standard names
*       - avoid floating point operations on playback
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "midifile.h"
#include "midiutil.h"
#include "midiplayer.h"
#include "hal/hal_misc.h"

static void dispatchMidiMsg(MIDI_PLAYER* pMidiPlayer, int32_t trackIndex) {
  MIDI_MSG* msg = &pMidiPlayer->msg[trackIndex];

  int32_t eventType = msg->bImpliedMsg ? msg->iImpliedMsg : msg->iType;
  switch (eventType) {
    case	msgNoteOff:
      if (pMidiPlayer->pOnNoteOffCb)
        pMidiPlayer->pOnNoteOffCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOff.iChannel, msg->MsgData.NoteOff.iNote);
      break;
    case	msgNoteOn:
      if (pMidiPlayer->pOnNoteOnCb)
        pMidiPlayer->pOnNoteOnCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteOn.iChannel, msg->MsgData.NoteOn.iNote, msg->MsgData.NoteOn.iVolume);
      break;
    case	msgNoteKeyPressure:
      if (pMidiPlayer->pOnNoteKeyPressureCb)
        pMidiPlayer->pOnNoteKeyPressureCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteKeyPressure.iChannel, msg->MsgData.NoteKeyPressure.iNote, msg->MsgData.NoteKeyPressure.iPressure);
      break;
    case	msgSetParameter:
      if (pMidiPlayer->pOnSetParameterCb)
        pMidiPlayer->pOnSetParameterCb(trackIndex, msg->dwAbsPos, msg->MsgData.NoteParameter.iChannel, msg->MsgData.NoteParameter.iControl, msg->MsgData.NoteParameter.iParam);
      break;
    case	msgSetProgram:
      if (pMidiPlayer->pOnSetProgramCb)
        pMidiPlayer->pOnSetProgramCb(trackIndex, msg->dwAbsPos, msg->MsgData.ChangeProgram.iChannel, msg->MsgData.ChangeProgram.iProgram);
      break;
    case	msgChangePressure:
      if (pMidiPlayer->pOnChangePressureCb)
        pMidiPlayer->pOnChangePressureCb(trackIndex, msg->dwAbsPos, msg->MsgData.ChangePressure.iChannel, msg->MsgData.ChangePressure.iPressure);
      break;
    case	msgSetPitchWheel:
      if (pMidiPlayer->pOnSetPitchWheelCb)
        pMidiPlayer->pOnSetPitchWheelCb(trackIndex, msg->dwAbsPos, msg->MsgData.PitchWheel.iChannel, msg->MsgData.PitchWheel.iPitch + 8192);
      break;
    case	msgMetaEvent:
      switch (msg->MsgData.MetaEvent.iType) {
      case	metaMIDIPort:
        if (pMidiPlayer->pOnMetaMIDIPortCb)
          pMidiPlayer->pOnMetaMIDIPortCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iMIDIPort);
        break;
      case	metaSequenceNumber:
        if (pMidiPlayer->pOnMetaSequenceNumberCb)
          pMidiPlayer->pOnMetaSequenceNumberCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.iSequenceNumber);
        break;
      case	metaTextEvent:
        if (pMidiPlayer->pOnMetaTextEventCb)
          pMidiPlayer->pOnMetaTextEventCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCopyright:
        if (pMidiPlayer->pOnMetaCopyrightCb)
          pMidiPlayer->pOnMetaCopyrightCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaTrackName:
        if (pMidiPlayer->pOnMetaTrackNameCb)
          pMidiPlayer->pOnMetaTrackNameCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaInstrument:
        if (pMidiPlayer->pOnMetaInstrumentCb)
          pMidiPlayer->pOnMetaInstrumentCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaLyric:
        if (pMidiPlayer->pOnMetaLyricCb)
          pMidiPlayer->pOnMetaLyricCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaMarker:
        if (pMidiPlayer->pOnMetaMarkerCb)
          pMidiPlayer->pOnMetaMarkerCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaCuePoint:
        if (pMidiPlayer->pOnMetaCuePointCb)
          pMidiPlayer->pOnMetaCuePointCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Text.pData);
        break;
      case	metaEndSequence:
        if (pMidiPlayer->pOnMetaEndSequenceCb)
          pMidiPlayer->pOnMetaEndSequenceCb(trackIndex, msg->dwAbsPos);
        break;
      case	metaSetTempo:
        setPlaybackTempo(pMidiPlayer->pMidiFile, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        if (pMidiPlayer->pOnMetaSetTempoCb)
          pMidiPlayer->pOnMetaSetTempoCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.Tempo.iBPM);
        break;
      case	metaSMPTEOffset:
        if (pMidiPlayer->pOnMetaSMPTEOffsetCb)
          pMidiPlayer->pOnMetaSMPTEOffsetCb(trackIndex, msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.SMPTE.iHours,
            msg->MsgData.MetaEvent.Data.SMPTE.iMins,
            msg->MsgData.MetaEvent.Data.SMPTE.iSecs,
            msg->MsgData.MetaEvent.Data.SMPTE.iFrames,
            msg->MsgData.MetaEvent.Data.SMPTE.iFF
          );
        break;
      case	metaTimeSig:
        // TODO: Metronome and thirtyseconds are missing!!!
        if (pMidiPlayer->pOnMetaTimeSigCb)
          pMidiPlayer->pOnMetaTimeSigCb(trackIndex,
            msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.TimeSig.iNom,
            msg->MsgData.MetaEvent.Data.TimeSig.iDenom / MIDI_NOTE_CROCHET,
            0, 0
          );
        break;
      case	metaKeySig: // TODO: scale is missing!!!
        if (pMidiPlayer->pOnMetaKeySigCb)
          pMidiPlayer->pOnMetaKeySigCb(trackIndex, msg->dwAbsPos, msg->MsgData.MetaEvent.Data.KeySig.iKey, 0);
        break;
      case	metaSequencerSpecific:
        if (pMidiPlayer->pOnMetaSequencerSpecificCb)
          pMidiPlayer->pOnMetaSequencerSpecificCb(trackIndex, msg->dwAbsPos,
            msg->MsgData.MetaEvent.Data.Sequencer.pData, msg->MsgData.MetaEvent.Data.Sequencer.iSize
          );
        break;
      }
      break;

    case	msgSysEx1:
    case	msgSysEx2:
      if (pMidiPlayer->pOnMetaSysExCb)
        pMidiPlayer->pOnMetaSysExCb(trackIndex, msg->dwAbsPos, msg->MsgData.SysEx.pData, msg->MsgData.SysEx.iSize);
      break;
    }
}

void midiplayer_init(MIDI_PLAYER* mpl, OnNoteOffCallback_t pOnNoteOffCb, OnNoteOnCallback_t pOnNoteOnCb,
  OnNoteKeyPressureCallback_t pOnNoteKeyPressureCb, OnSetParameterCallback_t pOnSetParameterCb,
  OnSetProgramCallback_t pOnSetProgramCb, OnChangePressureCallback_t pOnChangePressureCb, 
  OnSetPitchWheelCallback_t pOnSetPitchWheelCb, OnMetaMIDIPortCallback_t pOnMetaMIDIPortCb,
  OnMetaSequenceNumberCallback_t pOnMetaSequenceNumberCb, OnMetaTextEventCallback_t pOnMetaTextEventCb,
  OnMetaCopyrightCallback_t pOnMetaCopyrightCb, OnMetaTrackNameCallback_t pOnMetaTrackNameCb,
  OnMetaInstrumentCallback_t pOnMetaInstrumentCb, OnMetaLyricCallback_t pOnMetaLyricCb,
  OnMetaMarkerCallback_t pOnMetaMarkerCb, OnMetaCuePointCallback_t pOnMetaCuePointCb,
  OnMetaEndSequenceCallback_t pOnMetaEndSequenceCb, OnMetaSetTempoCallback_t pOnMetaSetTempoCb,
  OnMetaSMPTEOffsetCallback_t pOnMetaSMPTEOffsetCb, OnMetaTimeSigCallback_t pOnMetaTimeSigCb,
  OnMetaKeySigCallback_t pOnMetaKeySigCb, OnMetaSequencerSpecificCallback_t pOnMetaSequencerSpecificCb,
  OnMetaSysExCallback_t pOnMetaSysExCb
) {
  memset(mpl, 0, sizeof(MIDI_PLAYER));

  mpl->pOnNoteOffCb = pOnNoteOffCb;
  mpl->pOnNoteOnCb = pOnNoteOnCb;
  mpl->pOnNoteKeyPressureCb = pOnNoteKeyPressureCb;
  mpl->pOnSetParameterCb = pOnSetParameterCb;
  mpl->pOnSetProgramCb = pOnSetProgramCb;
  mpl->pOnChangePressureCb = pOnChangePressureCb;
  mpl->pOnSetPitchWheelCb = pOnSetPitchWheelCb;
  mpl->pOnMetaMIDIPortCb = pOnMetaMIDIPortCb;
  mpl->pOnMetaSequenceNumberCb = pOnMetaSequenceNumberCb;
  mpl->pOnMetaTextEventCb = pOnMetaTextEventCb;
  mpl->pOnMetaCopyrightCb = pOnMetaCopyrightCb;
  mpl->pOnMetaTrackNameCb = pOnMetaTrackNameCb;
  mpl->pOnMetaInstrumentCb = pOnMetaInstrumentCb;
  mpl->pOnMetaLyricCb = pOnMetaLyricCb;
  mpl->pOnMetaMarkerCb = pOnMetaMarkerCb;
  mpl->pOnMetaCuePointCb = pOnMetaCuePointCb;
  mpl->pOnMetaEndSequenceCb = pOnMetaEndSequenceCb;
  mpl->pOnMetaSetTempoCb = pOnMetaSetTempoCb;
  mpl->pOnMetaSMPTEOffsetCb = pOnMetaSMPTEOffsetCb;
  mpl->pOnMetaTimeSigCb = pOnMetaTimeSigCb;
  mpl->pOnMetaKeySigCb = pOnMetaKeySigCb;
  mpl->pOnMetaSequencerSpecificCb = pOnMetaSequencerSpecificCb;
  mpl->pOnMetaSysExCb = pOnMetaSysExCb;
}

bool midiPlayerOpenFile(MIDI_PLAYER* pMidiPlayer, const char* pFileName) {
  pMidiPlayer->pMidiFile = midiFileOpen(pFileName);
  if (!pMidiPlayer->pMidiFile)
    return false;
  
  // Load initial midi events
  for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMidiPlayer->pMidiFile); iTrack++) {
    midiReadGetNextMessage(pMidiPlayer->pMidiFile, iTrack, &pMidiPlayer->msg[iTrack]);
    pMidiPlayer->pMidiFile->Track[iTrack].deltaTime = pMidiPlayer->msg[iTrack].dt;
  }

  pMidiPlayer->startTime = hal_clock();
  pMidiPlayer->currentTick = 0;
  pMidiPlayer->lastTick = 0;
  pMidiPlayer->deltaTick = 0; // Must NEVER be negative!!!
  pMidiPlayer->eventsNeedToBeFetched = false;
  pMidiPlayer->trackIsFinished = true;
  pMidiPlayer->allTracksAreFinished = false;
  pMidiPlayer->lastMsPerTick = pMidiPlayer->pMidiFile->msPerTick;
  pMidiPlayer->timeScaleFactor = 1.0f;

  return true;
}

bool playMidiFile(MIDI_PLAYER* pMidiPlayer, const char *pFilename) {
  if (!midiPlayerOpenFile(pMidiPlayer, pFilename))
    return false;

  hal_printfInfo("Midi Format: %d", pMidiPlayer->pMidiFile->Header.iVersion);
  hal_printfInfo("Number of tracks: %d", midiReadGetNumTracks(pMidiPlayer->pMidiFile));
  hal_printfSuccess("Start playing...");
  return true;
}

bool midiPlayerTick(MIDI_PLAYER* pMidiPlayer) {
  MIDI_PLAYER* pMp = pMidiPlayer;

  if (pMp->pMidiFile == NULL)
    return false;

  if (fabs(pMp->lastMsPerTick - pMp->pMidiFile->msPerTick) > 0.001f) { // TODO: avoid floating point operation here!
    // On a tempo change we need to transform the old absolute time scale to the new scale.
    pMp->timeScaleFactor = pMp->lastMsPerTick / pMp->pMidiFile->msPerTick;
    pMp->lastTick *= pMp->timeScaleFactor;
  }
  pMp->lastMsPerTick = pMp->pMidiFile->msPerTick;
  pMp->currentTick = (hal_clock() - pMp->startTime) / pMp->pMidiFile->msPerTick;
  pMp->eventsNeedToBeFetched = true;

  while (pMp->eventsNeedToBeFetched) { // This loop keeps all tracks synchronized in case of a lag
    pMp->eventsNeedToBeFetched = false;
    pMp->allTracksAreFinished = true;
    pMp->deltaTick = pMp->currentTick - pMp->lastTick;
    if (pMp->deltaTick < 0) {
      hal_printfWarning("Warning: deltaTick is negative! Fast forward? deltaTick=%d", pMp->deltaTick);
      // TODO: correct time here, to prevent skips on following tempo changes!
      pMp->deltaTick = 0;
    }

    for (int iTrack = 0; iTrack < midiReadGetNumTracks(pMp->pMidiFile); iTrack++) {
      pMp->pMidiFile->Track[iTrack].deltaTime -= pMp->deltaTick;
      pMp->trackIsFinished = pMp->pMidiFile->Track[iTrack].ptrNew == pMp->pMidiFile->Track[iTrack].pEndNew;

      if (!pMp->trackIsFinished) {
        if (pMp->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMp->trackIsFinished) { // Is it time to play this event?
          dispatchMidiMsg(pMp, iTrack); // shoot

          // Debug 1/2
          int32_t expectedWaitTime = pMp->pMidiFile->Track[iTrack].debugLastMsgDt * pMp->lastMsPerTick;
          int32_t realWaitTime = hal_clock() - pMp->pMidiFile->Track[iTrack].debugLastClock;
          int32_t diff = realWaitTime - expectedWaitTime;

          if (abs(diff > 10))
            hal_printfWarning("Expected: %d ms, real: %d ms, diff: %d ms", 
              expectedWaitTime, realWaitTime, diff);
          // ---

          midiReadGetNextMessage(pMp->pMidiFile, iTrack, &pMp->msg[iTrack]); // reload
          pMp->pMidiFile->Track[iTrack].deltaTime += pMp->msg[iTrack].dt;

          // Debug 2/2
          pMp->pMidiFile->Track[iTrack].debugLastClock = hal_clock();
          pMp->pMidiFile->Track[iTrack].debugLastMsgDt = pMp->msg[iTrack].dt;
          // ---
        }

        if (pMp->pMidiFile->Track[iTrack].deltaTime <= 0 && !pMp->trackIsFinished)
          pMp->eventsNeedToBeFetched = true;

        pMp->allTracksAreFinished = false;
      }
      pMp->lastTick = pMp->currentTick;
    }
  }

  return !pMp->allTracksAreFinished; // TODO: close file
}
