#pragma once

class Keyboard {

};

void initializeKeyboard();

__attribute__((interrupt)) void keyboardInterruptHandler(struct interrupt_frame* frame);