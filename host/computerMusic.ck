SinOsc dup => dac;
SinOsc trip => dac;

fun void triple(dur d){
	while(true){
		Std.mtof( 60 ) => trip.freq;
	    d => now;
	    Std.mtof( 63 ) => trip.freq;
	    d => now;
	    Std.mtof( 67 ) => trip.freq;
	    d => now;
	}
}

fun void duple(dur d){
	while(true){
		Std.mtof( 48 ) => dup.freq;
	    d => now;
	    Std.mtof( 51 ) => dup.freq;
	    d => now;
	}
}

SinOsc s1 => dac;
SinOsc s2 => dac;
SinOsc s3 => dac;
fun void compMusic(int dimension){

	100 => float max;
	1 => float min;
	150::ms => dur speed;
	while(true){
		Std.rand2f(min, max) => float random;
		random => s1.freq;
		if(dimension == 2){
			random * 1.4142 => s2.freq;
		}
		if(dimension == 3){
			random * 1.25 => s2.freq;
			random * 1.5 => s3.freq;

		}
		speed => now;
		max + 7 => max;
		min + 2 => min;
		speed - .05::ms => speed;
	}
}

fun void done(){
	SinOsc done1 => dac;
	SinOsc done2 => dac;
	SinOsc done3 => dac;
	Std.mtof( 100 ) => done1.freq;
    200::ms => now;
    Std.mtof( 103 ) => done2.freq;
    200::ms => now;
    Std.mtof( 107 ) => done3.freq;
    200::ms => now;
    Std.mtof( 112 ) => done2.freq;
    750::ms => now;
}

external Event lineEnd;

spork ~ triple(1::second/(4)) @=> Shred killTrip;
3::second => now;


spork ~ compMusic(1) @=> Shred killLine;
1::second => now;

//LINE OVER
lineEnd=>now;
//killDup.exit();
killTrip.exit();
killLine.exit();
0 => s1.freq;
500::ms => now;
spork ~ done();
spork ~ compMusic(2) @=> Shred killSquare;

//SQUARE OVER
external Event squareEnd;
squareEnd=>now;
killSquare.exit();
0 => s1.freq;
0 => s2.freq;
0 => s3.freq;
500::ms => now;
spork ~ done();
spork ~ compMusic(2) @=> Shred killCube;

//CUBE OVER
external Event cubeEnd;
cubeEnd=>now;
killCube.exit();
SinOsc final => dac;
200 => final.freq;
3::second => now;