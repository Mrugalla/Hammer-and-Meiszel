/*
-----------------------------------------------------------------------------------------
~~~ concepts ~~~

~~~ todo ~~~

KeySelector
    Buttons updaten sich nocht nicht passend zum Toggle State
    Key Selector Enabled macht noch nichts sinnvolles

write new sleepy state mechanism for modal resonator filter.
problem was: it turned off the modal filter when it got too quiet
but quietness can also come from low sustain values on the envelope
or from low audio input values, but it should only trigger when nothing's playing anymore

rescue overlaps funzt noch nicht gut genug, manchmal overlappen sie nicht aber sind nicht einzeln ansprechbar

hold down note, then press other notes to trigger the dry through bug

generated saw and sqr acts weird with keytrack+ when you drag partials away from harmonic series

remap frequencies of partials with a MSEG curve, like mtransformer

map velocity to certain parameters

toast wrong place when hovering modal material editor

buttons ohne bg colour

preset browser

drag last created sound to DAW as audio

modal > comb > post sequence thing

smoothing-zeiten von modal voice sind viel zu lang. sichtbar wenn man n normalisierten param auf 0 dreht mit DBG

when sample imported user defines which range should be taken
    if user defines 2 ranges, it's one for each modal material

keytrack
    way to change freq of partials

optimize CPU with SIMD filters

all tooltips in english

everything from breite to width

Material
    import von samples erzeugt oft keine geilen magnitudes
    material editor solve instrument imports where fundamental not highest peak

Param db valToStr can be -inf (gain wet [-inf, 0])
    but it can also be not (gain out [-12,12])
GUI
    reintroduce custom mousecursor
    SharedPluginState (customizable state)
        fonts f�r verschiedene funktionen
            (zB knob-labels, titel, etc)
        sensitivity
            drag, wheel, key
        defaultvalue
            doubleclick, key
    all parameters currently lack visible lock feature
AUDIO
    FFT
        replace by 3rd party lib for performance (kiss fft or pffft)
    Oversampler
        make convolver FFT-based
        implement 4x
    sidechain
        envfol
            should appear to be added to macro
            actually sample-accurate
    MixProcessor
        code readability
        add latency compensation delay
        write cpp

~~~ video releases ~~~

Fixed My Auto-Suspend Voice Bug With Breakpoints
https://youtu.be/ycLhqMkX4lk?si=RMXjk8sqLYwptULJ

VST Coding: Harmonize and Saturate Modal Filters
https://www.youtube.com/watch?v=PP3Gg9-KQiU

VST Coding: Transposing Synth Voices
https://youtu.be/u7dhEzGlFyI?si=_dWUi8-w0Bz-gynf

Navigating Atomic State Using Call Graphs (Coding Tutorial)
https://www.youtube.com/watch?v=deVicZAaI44

Creative DSP Coding
https://www.youtube.com/watch?v=u39isqUp_KU

Coding Tutorial: Save and Load Non-Parameter Instance State
https://youtu.be/PQGN1Nf49uI?si=DwMHiBX-nBvyixq4

Handling Project Constants With My Axiom Namespace
https://youtu.be/v9d4Kc-c2Wg?si=6tSkd0CNFbgE_8-H

Coding and UI/UX: How Long Is Your Envelope?
https://youtu.be/Uo2hoLZDCSs?si=T_hQcCRwcd9ko4ul

~~~ awesome beta team ~~~

mleuc weil geilste sounddesign f�r neurozeug und botanica
dash glitch, because https://www.youtube.com/watch?v=0i1LCrC7dZw
dr. laguna weil er hat dieses geile allpass plugin gemacht
ewan bristow weil er n geiler botanica & plugdata-typ ist
alex reid, weil bitwig community
peazy
nimand

open: street shaman, zayne, xantux, felix
liraxity, signalsmith, eyalamir

-----------------------------------------------------------------------------------------

*/