# Heisser Draht

software originally for http://www.kreativekiste.de/elektro/dem-heissen-draht-auf-der-spur
developed by Timo Denk (Nov 2014)
modified by Sebastian Sproesser (2015)

We use a Arduino Mega, a seven segment display and a LCD

Wire-wiring: 
- 22: start
- 24: stop
- 26: mistake
- 28: buzzer
- A0: penalty time

In this implementation start and stop are interchangeable, we can
begin from either side. We trigger the start/stop with our game loop

Connections for seven segment display:
45, 46, 47, 48, 49, 50, 51, 52

Connections LCD:
- rs (LCD pin 4) to Arduino pin 12
- rw (LCD pin 5) to Arduino pin 11
- enable (LCD pin 6) to Arduino pin 10
- LCD pin 15 to Arduino pin 13
- LCD pins d4, d5, d6, d7 to Arduino pins 5, 4, 3, 2

## Serial communication of game state

When the game runs, we send the current state of the game via serial
connection to use a bigger display powered by a Raspberry Pi

Our messages have the format

"mm:ss:xxx;mmm;status"

	mm	Minutes
	ss	Seconds
	xxx	Milliseconds
	mmm	Current count of mistakes
	status can be:
		Start	The start of a game
		Reset	The player has gone back to the start
		Mistake	A mistake was made
		Stop	The player has completed a run