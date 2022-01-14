// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

// Based on https://github.com/psxdev/liborbis/blob/master/liborbisPad/source/orbisPad.c

#include "Controller.h"

#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/_types/user.h>

#include <cstdio>

#include "libLog.h"

// Initialize the controller
Controller::Controller() {
  // Initialize the Pad library
  if (scePadInit() != 0) {
    logKernel(LL_Debug, "%s", "Failed to initialize pad library!");
    return;
  }

  // Get the user ID
  OrbisUserServiceInitializeParams s_Param;
  s_Param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
  sceUserServiceInitialize(&s_Param);
  sceUserServiceGetInitialUser(&m_UserId);

  // Open a handle for the controller
  m_PadId = scePadOpen(m_UserId, 0, 0, NULL);

  if (m_PadId < 0) {
    logKernel(LL_Debug, "%s", "Failed to open pad!");
    return;
  }
}

// Uninitialize controller
Controller::~Controller() {
  scePadClose(m_PadId);
}

// Controller : Get current button pressed
unsigned int Controller::GetCurrentButtonsPressed() {
  return m_ButtonsPressed;
}

// Controller : Set current pressed button
void Controller::SetCurrentButtonsPressed(unsigned int p_Buttons) {
  m_ButtonsPressed = p_Buttons;
}

// Controller : Get current released button
unsigned int Controller::GetCurrentButtonsReleased() {
  return m_ButtonsReleased;
}

// Controller : Set released button
void Controller::SetCurrentButtonsReleased(unsigned int p_Buttons) {
  m_ButtonsReleased = p_Buttons;
}

// Controller : Hold button
bool Controller::GetButtonHold(unsigned int p_Filter) {
  if ((m_ButtonsHold & p_Filter) == p_Filter) {
    return 1;
  }
  return 0;
}

// Controller : Pressed button
bool Controller::GetButtonPressed(unsigned int p_Filter) {
  if ((m_ButtonsPressed & p_Filter) == p_Filter) {
    return 1;
  }
  return 0;
}

// Controller : Released button
bool Controller::GetButtonReleased(unsigned int p_Filter) {
  if ((m_ButtonsReleased & p_Filter) == p_Filter) {
    if (~(m_PadData.buttons) & p_Filter) {
      return 0;
    }
    return 1;
  }
  return 0;
}

// Update controllers value
void Controller::Update() {

  unsigned int s_ActualButtons = 0;
  unsigned int s_LastButtons = 0;

  s_LastButtons = m_PadData.buttons;
  scePadReadState(m_PadId, &m_PadData);
  s_ActualButtons = m_PadData.buttons;

  m_ButtonsPressed = (s_ActualButtons) & (~s_LastButtons);
  if (s_ActualButtons != s_LastButtons) {
    m_ButtonsReleased = (~s_ActualButtons) & (s_LastButtons);
  } else {
    m_ButtonsReleased = 0;
  }

  m_CurrentButtons = m_PadData.buttons;
  m_ButtonsHold = s_ActualButtons & s_LastButtons;
}
