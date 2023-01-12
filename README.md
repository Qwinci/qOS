# qOS

A personal X86_64 OS project

## Todo
### General
- [ ] Scheduler
- [ ] Elf loading
- [ ] Usermode
- [ ] Console
### Drivers
- [ ] Usb support
	- [-] Uhci (partially done)
	- [ ] Ohci
	- [ ] Ehci
	- [ ] Xhci
	- [ ] Mass storage
	- [ ] Hid keyboard
	- [ ] Hid mouse
- [ ] Timers support
	- [ ] Hpet
		- [x] Poll support
		- [ ] Interrupt driven support
	- [ ] Apic timer
	- [ ] Tsc
- [ ] Ps1
	- [-] Keyboard (partially done)
	- [ ] Mouse
- [ ] Graphics
	- [ ] Generic interface to graphics devices
	- [ ] Cpu framebuffer device
		- [ ] OpenGL
		- [ ] Vulkan