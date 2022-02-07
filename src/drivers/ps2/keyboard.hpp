#pragma once

class Keyboard {

};

__attribute__((interrupt)) void keyboardInterruptHandler(struct interrupt_frame* frame);