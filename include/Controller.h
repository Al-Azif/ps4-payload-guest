// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <orbis/_types/pad.h>

class Controller {
public:
  Controller();
  ~Controller();

  void Update();

  unsigned int GetCurrentButtonsPressed();

  void SetCurrentButtonsPressed(unsigned int p_Buttons);
  unsigned int GetCurrentButtonsReleased();
  void SetCurrentButtonsReleased(unsigned int p_Buttons);

  bool GetButtonHold(unsigned int p_Filter);
  bool GetButtonPressed(unsigned int p_Filter);
  bool GetButtonReleased(unsigned int p_Filter);

private:
  int m_UserId;
  int m_PadId;

  unsigned int m_ButtonsPressed;
  unsigned int m_ButtonsReleased;
  unsigned int m_CurrentButtons;
  unsigned int m_ButtonsHold;

  OrbisPadData m_PadData;
};

#endif
