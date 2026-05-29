          .CPU        300HN

; Absolute symbol definitions for hardware-mapped addresses (0xF088, 0xF580)
_DAT_f088  .EQU        61576
_DAT_f580  .EQU        62848

          .EXPORT     _DAT_f088
          .EXPORT     _DAT_f580

          .EXPORT     _totalSteps
          .EXPORT     _totalSteps_1
          .EXPORT     _RamCache_STEP_COUNT_maybe
          .EXPORT     _DAT_f788
          .EXPORT     _DAT_f78c
          .EXPORT     _watts
          .EXPORT     _DAT_f790
          .EXPORT     _stepWattCounter
          .EXPORT     _DAT_f793
          .EXPORT     _RamCache_settingsByte
          .EXPORT     _buttonInputRaw
          .EXPORT     _prevButtonInputRaw
          .EXPORT     _buttonTrigger
          .EXPORT     _buttonHoldDuration
          .EXPORT     _sessionSteps
          .EXPORT     _DAT_f7a0
          .EXPORT     _DAT_f7a2
          .EXPORT     _DAT_f7a4
          .EXPORT     _DAT_f7a5
          .EXPORT     _DAT_f7a6
          .EXPORT     _DAT_f7a7
          .EXPORT     _DAT_f7a8
          .EXPORT     _DAT_f7a9
          .EXPORT     _menu_cursor
          .EXPORT     _DAT_f7ab
          .EXPORT     _DAT_f7ac
          .EXPORT     _DAT_f7ad
          .EXPORT     _accelSampleCount
          .EXPORT     _activityTimer
          .EXPORT     _stepTimer
          .EXPORT     _currentlyActiveView
          .EXPORT     _stepBatchSize
          .EXPORT     _DAT_f7b3
          .EXPORT     _DAT_f7b4
          .EXPORT     _statusFlags
          .EXPORT     _walker_status_flags
          .EXPORT     _lastCommandTime
          .EXPORT     _commandPos
          .EXPORT     _wakeupFlagMaybe
          .EXPORT     _heapPointer
          .EXPORT     _nextRandom
          .EXPORT     _soundData
          .EXPORT     _volume
          .EXPORT     _noteDuration
          .EXPORT     _isSeparateNote
          .EXPORT     _soundHeader
          .EXPORT     _gCurSubstateY
          .EXPORT     _gCurSubstateZ
          .EXPORT     _gCurSubstateA
          .EXPORT     _DAT_f7d1
          .EXPORT     _accelXPos
          .EXPORT     _dowsing_item_pos
          .EXPORT     _accelYPos
          .EXPORT     _DAT_f7d5
          .EXPORT     _accelZPos
          .EXPORT     _DAT_f7d8
          .EXPORT     _DAT_f7d8_1
          .EXPORT     _DAT_f7da
          .EXPORT     _DAT_f7dc
          .EXPORT     _DAT_f7de
          .EXPORT     _currentEventLoopFunc
          .EXPORT     _DAT_f7e2
          .EXPORT     _DAT_f7e4
          .EXPORT     _fft_results
          .EXPORT     _DAT_f7e6
          .EXPORT     _DAT_f7ea
          .EXPORT     _DAT_f7ee
          .EXPORT     _DAT_f7f0
          .EXPORT     _DAT_f7f2
          .EXPORT     _ACCEL_SAMPLES_X
          .EXPORT     _accelXSamples
          .EXPORT     _accelYSamples
          .EXPORT     _accelZSamples
          .EXPORT     _DAT_f82e
          .EXPORT     _DAT_f840
          .EXPORT     _DAT_f841
          .EXPORT     _DAT_f842
          .EXPORT     _DAT_f843
          .EXPORT     _DAT_f844
          .EXPORT     _DAT_f846
          .EXPORT     _DAT_f84e
          .EXPORT     _DAT_f856
          .EXPORT     _ACCEL_SAMPLES_Y
          .EXPORT     _L_F886
          .EXPORT     _DAT_f886
          .EXPORT     _DAT_f88e
          .EXPORT     _DAT_f896
          .EXPORT     _DAT_f897
          .EXPORT     _ACCEL_SAMPLES_Z
          .EXPORT     _DAT_f8a9
          .EXPORT     _DAT_f8b6
          .EXPORT     _DAT_f8ba
          .EXPORT     _DAT_f8be
          .EXPORT     _DAT_f8bf
          .EXPORT     _DAT_f8c1
          .EXPORT     _DAT_f8c2
          .EXPORT     _DAT_f8c3
          .EXPORT     _REQUESTED_POKEMON_ACTION_TYPE
          .EXPORT     _DAT_f8c5
          .EXPORT     _DAT_f8c6
          .EXPORT     _DAT_f8c8
          .EXPORT     _DAT_f8ca
          .EXPORT     _DAT_f8cc
          .EXPORT     _rdr_data
          .EXPORT     _commandType
          .EXPORT     _DAT_f8cf
          .EXPORT     _DAT_f8d0
          .EXPORT     _DAT_f8d1
          .EXPORT     _DAT_f8d2
          .EXPORT     _TX_PACKET_payload
          .EXPORT     _DAT_f8d7
          .EXPORT     _DAT_f8d8
          .EXPORT     _DAT_f8e6
          .EXPORT     _DAT_f8ea
          .EXPORT     _DAT_f8ee
          .EXPORT     _isNotWalking
          .EXPORT     _DAT_f956
          .EXPORT     _DAT_f957
          .EXPORT     _DAT_f963
          .EXPORT     _DAT_f964
          .EXPORT     _DAT_fef0
          .EXPORT     _initialStackPosition

          .SECTION    B,DATA,ALIGN=2

_totalSteps:
          .RES.B      1
_totalSteps_1:
          .RES.B      3
_RamCache_STEP_COUNT_maybe:
          .RES.B      4
_DAT_f788:
          .RES.B      4
_DAT_f78c:
          .RES.B      2
_watts:
          .RES.B      2
_DAT_f790:
          .RES.B      2
_stepWattCounter:
          .RES.B      1
_DAT_f793:
          .RES.B      4
_RamCache_settingsByte:
          .RES.B      1
_buttonInputRaw:
          .RES.B      1
_prevButtonInputRaw:
          .RES.B      1
_buttonTrigger:
          .RES.B      1
_buttonHoldDuration:
          .RES.B      1
_sessionSteps:
          .RES.B      4
_DAT_f7a0:
          .RES.B      2
_DAT_f7a2:
          .RES.B      2
_DAT_f7a4:
          .RES.B      1
_DAT_f7a5:
          .RES.B      1
_DAT_f7a6:
          .RES.B      1
_DAT_f7a7:
          .RES.B      1
_DAT_f7a8:
          .RES.B      1
_DAT_f7a9:
          .RES.B      1
_menu_cursor:
          .RES.B      1
_DAT_f7ab:
          .RES.B      1
_DAT_f7ac:
          .RES.B      1
_DAT_f7ad:
          .RES.B      1
_accelSampleCount:
          .RES.B      1
_activityTimer:
          .RES.B      1
_stepTimer:
          .RES.B      1
_currentlyActiveView:
          .RES.B      1
_stepBatchSize:
          .RES.B      1
_DAT_f7b3:
          .RES.B      1
_DAT_f7b4:
          .RES.B      1
_statusFlags:
          .RES.B      1
_walker_status_flags:
          .RES.B      2
_lastCommandTime:
          .RES.B      2
_commandPos:
          .RES.B      1
_wakeupFlagMaybe:
          .RES.B      3
_heapPointer:
          .RES.B      2
_nextRandom:
          .RES.B      4
_soundData:
          .RES.B      2
_volume:
          .RES.B      2
_noteDuration:
          .RES.B      2
_isSeparateNote:
          .RES.B      2
_soundHeader:
          .RES.B      2
_gCurSubstateY:
          .RES.B      1
_gCurSubstateZ:
          .RES.B      1
_gCurSubstateA:
          .RES.B      1
_DAT_f7d1:
          .RES.B      1
_accelXPos:
          .RES.B      1
_dowsing_item_pos:
          .RES.B      1
_accelYPos:
          .RES.B      1
_DAT_f7d5:
          .RES.B      1
_accelZPos:
          .RES.B      2
_DAT_f7d8:
          .RES.B      1
_DAT_f7d8_1:
          .RES.B      1
_DAT_f7da:
          .RES.B      2
_DAT_f7dc:
          .RES.B      2
_DAT_f7de:
          .RES.B      2
_currentEventLoopFunc:
          .RES.B      2
_DAT_f7e2:
          .RES.B      2
_DAT_f7e4:
          .RES.B      2
_fft_results:
_DAT_f7e6:
          .RES.B      4
_DAT_f7ea:
          .RES.B      4
_DAT_f7ee:
          .RES.B      2
_DAT_f7f0:
          .RES.B      2
_DAT_f7f2:
          .RES.B      52
_accelXSamples:
_ACCEL_SAMPLES_X:
          .RES.B      8
_DAT_f82e:
          .RES.B      18
_DAT_f840:
          .RES.B      1
_DAT_f841:
          .RES.B      1
_DAT_f842:
          .RES.B      1
_DAT_f843:
          .RES.B      1
_DAT_f844:
          .RES.B      2
_DAT_f846:
          .RES.B      8
_DAT_f84e:
          .RES.B      8
_DAT_f856:
          .RES.B      16
_accelYSamples:
_ACCEL_SAMPLES_Y:
          .RES.B      32
_L_F886:
_DAT_f886:
          .RES.B      8
_DAT_f88e:
          .RES.B      8
_DAT_f896:
          .RES.B      1
_DAT_f897:
          .RES.B      15
_accelZSamples:
_ACCEL_SAMPLES_Z:
          .RES.B      3
_DAT_f8a9:
          .RES.B      13
_DAT_f8b6:
          .RES.B      4
_DAT_f8ba:
          .RES.B      4
_DAT_f8be:
          .RES.B      1
_DAT_f8bf:
          .RES.B      2
_DAT_f8c1:
          .RES.B      1
_DAT_f8c2:
          .RES.B      1
_DAT_f8c3:
          .RES.B      1
_REQUESTED_POKEMON_ACTION_TYPE:
          .RES.B      1
_DAT_f8c5:
          .RES.B      1
_DAT_f8c6:
          .RES.B      2
_DAT_f8c8:
          .RES.B      2
_DAT_f8ca:
          .RES.B      2
_DAT_f8cc:
          .RES.B      1
_rdr_data:
          .RES.B      1
_commandType:
          .RES.B      1
_DAT_f8cf:
          .RES.B      1
_DAT_f8d0:
          .RES.B      1
_DAT_f8d1:
          .RES.B      1
_DAT_f8d2:
          .RES.B      4
_TX_PACKET_payload:
          .RES.B      1
_DAT_f8d7:
          .RES.B      1
_DAT_f8d8:
          .RES.B      14
_DAT_f8e6:
          .RES.B      4
_DAT_f8ea:
          .RES.B      4
_DAT_f8ee:
          .RES.B      1
_isNotWalking:
          .RES.B      103
_DAT_f956:
          .RES.B      1
_DAT_f957:
          .RES.B      12
_DAT_f963:
          .RES.B      1
_DAT_f964:
          .RES.B      1420
_DAT_fef0:
          .RES.B      136
_initialStackPosition:
          .RES.B      8

          .END