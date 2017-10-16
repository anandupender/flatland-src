SinOsc foo2 => dac;
SinOsc foo3 => dac;
SinOsc foo4 => dac;
SinOsc snare => dac;


while(true){
	0 => snare.freq;
	120 => foo2.freq;
	80 => foo3.freq;
	100 => foo4.freq;

	100::ms => now;

	0 => snare.freq;
	0 => foo2.freq;
	0 => foo3.freq;
	0 => foo4.freq;

	250::ms => now;
	250::ms => now;
}