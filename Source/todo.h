/*
-----------------------------------------------------------------------------------------
~~~ concepts ~~~

~~~ todo ~~~

make default presets for common usecases

remap freq ratios of partials with a MSEG curve, like mtransformer

map velocity to certain parameters
    or maybe just to gain the amp/mod envelopes

when sample imported user defines which range should be taken
    if user defines 2 ranges, it's one for each modal material

optimize modal filter CPU
    SIMD
    FFT

Material
    import von samples erzeugt nicht immer geile magnitudes
    material editor solve instrument imports where fundamental not highest peak

Param db valToStr can be -inf (gain wet [-inf, 0])
    but it can also be not (gain out [-12,12])
GUI
    SharedPluginState (customizable state)
        fonts für verschiedene funktionen
            (zB knob-labels, titel, etc)
        sensitivity
            drag, wheel, key
        defaultvalue
            doubleclick, key
AUDIO
    FFT
        replace by 3rd party lib for performance (kiss fft or pffft)
    Oversampler
        make convolver FFT-based
        implement 4x
    sidechain
        envfol
            should appear to be added to macro
            sample-accurate
    MixProcessor
        code readability
        add latency compensation delay
        write cpp

~~~ video releases ~~~

JUCE Tutorial: Drag And Drop Audio Files
https://www.youtube.com/watch?v=JNU8KDuLR78

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

-----------------------------------------------------------------------------------------

C:\Program Files\Image-Line\FL Studio 2024\FL64.exe
C:\Program Files\Steinberg\Cubase 9.5\Cubase9.5.exe

-----------------------------------------------------------------------------------------

*/