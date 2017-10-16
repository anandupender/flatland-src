<<< "in Chuck" >>>;

if(true){
    adc => Gain g => dac;
    SinOsc foo2 => g;
    3 => g.op;
    440 =>  foo2.freq;
} 

while(true)1::second => now;